#include "order_book.hpp"
#include <algorithm>
#include <stdexcept>

namespace crypto_matching_engine {

OrderBook::OrderBook(const std::string& symbol) : symbol_(symbol) {}

bool OrderBook::addOrder(Order order) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Validate order
    if (order.type == OrderType::LIMIT && !order.price) {
        return false;
    }
    
    // Try to match the order first
    if (matchOrder(order)) {
        // If it's an IOC or FOK order and we couldn't fill it completely, return false
        if ((order.type == OrderType::IOC || order.type == OrderType::FOK) && 
            order.quantity > 0) {
            return false;
        }
    }
    
    // If there's remaining quantity and it's a limit order, add to book
    if (order.quantity > 0 && order.type == OrderType::LIMIT) {
        addToBook(order);
    }
    
    return true;
}

bool OrderBook::matchOrder(Order& order) {
    if (order.quantity <= 0) return false;
    
    if (order.side == OrderSide::BUY) {
        matchAgainstSide<std::less<Price>>(order, asks_, true);
    } else {
        matchAgainstSide<std::greater<Price>>(order, bids_, false);
    }
    
    return order.quantity == 0;
}

template<typename Compare>
void OrderBook::matchAgainstSide(Order& order, 
                               std::map<Price, OrderBookLevel, Compare>& opposite_side,
                               bool is_buy) {
    while (order.quantity > 0 && !opposite_side.empty()) {
        auto& [price, level] = *opposite_side.begin();
        
        // Check if we can match at this price
        if ((is_buy && price > order.price.value_or(std::numeric_limits<Price>::max())) ||
            (!is_buy && price < order.price.value_or(0))) {
            break;
        }
        
        // Match against orders at this price level
        for (auto it = level.orders.begin(); it != level.orders.end();) {
            if (order.quantity <= 0) break;
            
            Quantity match_quantity = std::min(order.quantity, it->quantity);
            
            // Create and notify trade
            Trade trade{
                .maker_order_id = it->id,
                .taker_order_id = order.id,
                .symbol = symbol_,
                .price = price,
                .quantity = match_quantity,
                .aggressor_side = order.side,
                .timestamp = std::chrono::system_clock::now()
            };
            notifyTrade(trade);
            
            // Update quantities
            order.quantity -= match_quantity;
            it->quantity -= match_quantity;
            level.total_quantity -= match_quantity;
            
            // Remove filled orders
            if (it->quantity == 0) {
                order_lookup_.erase(it->id);
                it = level.orders.erase(it);
            } else {
                ++it;
            }
        }
        
        // Remove empty price levels
        if (level.orders.empty()) {
            opposite_side.erase(opposite_side.begin());
        }
    }
    
    updateBBO();
}

void OrderBook::addToBook(Order& order) {
    if (order.side == OrderSide::BUY) {
        auto& level = bids_[*order.price];
        level.price = *order.price;
        level.orders.push_back(order);
        level.total_quantity += order.quantity;
    } else {
        auto& level = asks_[*order.price];
        level.price = *order.price;
        level.orders.push_back(order);
        level.total_quantity += order.quantity;
    }
    
    order_lookup_[order.id] = {*order.price, order.side};
    updateBBO();
}

bool OrderBook::cancelOrder(OrderId order_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = order_lookup_.find(order_id);
    if (it == order_lookup_.end()) {
        return false;
    }
    
    removeFromBook(order_id);
    return true;
}

void OrderBook::removeFromBook(OrderId order_id) {
    auto [price, side] = order_lookup_.at(order_id);
    
    if (side == OrderSide::BUY) {
        auto level_it = bids_.find(price);
        if (level_it != bids_.end()) {
            auto& level = level_it->second;
            auto order_it = std::find_if(level.orders.begin(), level.orders.end(),
                                       [order_id](const Order& o) { return o.id == order_id; });
            
            if (order_it != level.orders.end()) {
                level.total_quantity -= order_it->quantity;
                level.orders.erase(order_it);
                
                if (level.orders.empty()) {
                    bids_.erase(level_it);
                }
            }
        }
    } else {
        auto level_it = asks_.find(price);
        if (level_it != asks_.end()) {
            auto& level = level_it->second;
            auto order_it = std::find_if(level.orders.begin(), level.orders.end(),
                                       [order_id](const Order& o) { return o.id == order_id; });
            
            if (order_it != level.orders.end()) {
                level.total_quantity -= order_it->quantity;
                level.orders.erase(order_it);
                
                if (level.orders.empty()) {
                    asks_.erase(level_it);
                }
            }
        }
    }
    
    order_lookup_.erase(order_id);
    updateBBO();
}

bool OrderBook::modifyOrder(OrderId order_id, Quantity new_quantity) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = order_lookup_.find(order_id);
    if (it == order_lookup_.end()) {
        return false;
    }
    
    auto [price, side] = order_lookup_.at(order_id);
    
    if (side == OrderSide::BUY) {
        auto level_it = bids_.find(price);
        if (level_it != bids_.end()) {
            auto& level = level_it->second;
            auto order_it = std::find_if(level.orders.begin(), level.orders.end(),
                                       [order_id](const Order& o) { return o.id == order_id; });
            
            if (order_it != level.orders.end()) {
                level.total_quantity -= order_it->quantity;
                order_it->quantity = new_quantity;
                level.total_quantity += new_quantity;
                updateBBO();
                return true;
            }
        }
    } else {
        auto level_it = asks_.find(price);
        if (level_it != asks_.end()) {
            auto& level = level_it->second;
            auto order_it = std::find_if(level.orders.begin(), level.orders.end(),
                                       [order_id](const Order& o) { return o.id == order_id; });
            
            if (order_it != level.orders.end()) {
                level.total_quantity -= order_it->quantity;
                order_it->quantity = new_quantity;
                level.total_quantity += new_quantity;
                updateBBO();
                return true;
            }
        }
    }
    
    return false;
}

BestBidOffer OrderBook::getBBO() const {
    BestBidOffer bbo;
    
    if (!bids_.empty()) {
        const auto& [price, level] = *bids_.begin();
        bbo.best_bid = price;
        bbo.best_bid_quantity = level.total_quantity;
    }
    
    if (!asks_.empty()) {
        const auto& [price, level] = *asks_.begin();
        bbo.best_offer = price;
        bbo.best_offer_quantity = level.total_quantity;
    }
    
    return bbo;
}

std::vector<std::pair<Price, Quantity>> OrderBook::getOrderBookDepth(size_t levels) const {
    std::vector<std::pair<Price, Quantity>> depth;
    
    // Add bids
    size_t count = 0;
    for (const auto& [price, level] : bids_) {
        if (count++ >= levels) break;
        depth.emplace_back(price, level.total_quantity);
    }
    
    // Add asks
    count = 0;
    for (const auto& [price, level] : asks_) {
        if (count++ >= levels) break;
        depth.emplace_back(price, level.total_quantity);
    }
    
    return depth;
}

void OrderBook::setTradeCallback(TradeCallback callback) {
    trade_callback_ = std::move(callback);
}

void OrderBook::setBBOUpdateCallback(BBOUpdateCallback callback) {
    bbo_update_callback_ = std::move(callback);
}

void OrderBook::notifyTrade(const Trade& trade) {
    if (trade_callback_) {
        trade_callback_(trade);
    }
}

void OrderBook::updateBBO() {
    if (bbo_update_callback_) {
        bbo_update_callback_(symbol_, getBBO());
    }
}

} // namespace crypto_matching_engine 