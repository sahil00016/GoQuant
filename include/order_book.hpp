#pragma once

#include "order_types.hpp"
#include <memory>
#include <mutex>
#include <functional>
#include <queue>

namespace crypto_matching_engine {

class OrderBook {
public:
    using TradeCallback = std::function<void(const Trade&)>;
    using BBOUpdateCallback = std::function<void(const std::string&, const BestBidOffer&)>;

    OrderBook(const std::string& symbol);
    
    // Order management
    bool addOrder(Order order);
    bool cancelOrder(OrderId order_id);
    bool modifyOrder(OrderId order_id, Quantity new_quantity);
    
    // Market data
    BestBidOffer getBBO() const;
    std::vector<std::pair<Price, Quantity>> getOrderBookDepth(size_t levels) const;
    
    // Callback registration
    void setTradeCallback(TradeCallback callback);
    void setBBOUpdateCallback(BBOUpdateCallback callback);

private:
    std::string symbol_;
    std::map<Price, OrderBookLevel, std::greater<Price>> bids_;
    std::map<Price, OrderBookLevel, std::less<Price>> asks_;
    std::unordered_map<OrderId, std::pair<Price, OrderSide>> order_lookup_;
    
    mutable std::mutex mutex_;
    TradeCallback trade_callback_;
    BBOUpdateCallback bbo_update_callback_;
    
    // Internal matching functions
    bool matchOrder(Order& order);
    template<typename Compare>
    void matchAgainstSide(Order& order, 
                         std::map<Price, OrderBookLevel, Compare>& opposite_side,
                         bool is_buy);
    void updateBBO();
    void notifyTrade(const Trade& trade);
    void notifyBBOUpdate();
    
    // Helper functions
    bool isPriceCrossing(const Order& order) const;
    void addToBook(Order& order);
    void removeFromBook(OrderId order_id);
    void updateOrderBookLevel(OrderBookLevel& level, const Order& order, bool is_add);
};

} // namespace crypto_matching_engine 