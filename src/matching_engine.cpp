#include "matching_engine.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace crypto_matching_engine {

MatchingEngine::MatchingEngine() {
    startOrderProcessing();
}

MatchingEngine::~MatchingEngine() {
    stopOrderProcessing();
}

bool MatchingEngine::submitOrder(const std::string& symbol, Order order) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    order_queue_.push(OrderEvent{
        .type = OrderEvent::Type::SUBMIT,
        .symbol = symbol,
        .order = order
    });
    queue_cv_.notify_one();
    return true;
}

bool MatchingEngine::cancelOrder(const std::string& symbol, OrderId order_id) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    order_queue_.push(OrderEvent{
        .type = OrderEvent::Type::CANCEL,
        .symbol = symbol,
        .order_id = order_id
    });
    queue_cv_.notify_one();
    return true;
}

bool MatchingEngine::modifyOrder(const std::string& symbol, OrderId order_id, Quantity new_quantity) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    order_queue_.push(OrderEvent{
        .type = OrderEvent::Type::MODIFY,
        .symbol = symbol,
        .order_id = order_id,
        .new_quantity = new_quantity
    });
    queue_cv_.notify_one();
    return true;
}

BestBidOffer MatchingEngine::getBBO(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(books_mutex_);
    auto it = order_books_.find(symbol);
    if (it != order_books_.end()) {
        return it->second->getBBO();
    }
    return BestBidOffer{};
}

std::vector<std::pair<Price, Quantity>> MatchingEngine::getOrderBookDepth(
    const std::string& symbol, size_t levels) const {
    std::lock_guard<std::mutex> lock(books_mutex_);
    auto it = order_books_.find(symbol);
    if (it != order_books_.end()) {
        return it->second->getOrderBookDepth(levels);
    }
    return {};
}

void MatchingEngine::processOrders() {
    while (true) {
        OrderEvent event;
        try {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this]() { 
                return !order_queue_.empty() || !running_; 
            });
            
            if (!running_ && order_queue_.empty()) {
                break;
            }
            
            event = order_queue_.front();
            order_queue_.pop();
            
            handleOrderEvent(event);
        } catch (const std::exception& e) {
            std::cerr << "Matching Engine Processing Error: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown Matching Engine Processing Error" << std::endl;
        }
    }
}

void MatchingEngine::handleOrderEvent(const OrderEvent& event) {
    switch (event.type) {
        case OrderEvent::Type::SUBMIT: {
            auto book = getOrCreateOrderBook(event.symbol);
            if (book) {
                book->addOrder(event.order);
            }
            break;
        }
        case OrderEvent::Type::CANCEL: {
            std::lock_guard<std::mutex> lock(books_mutex_);
            auto it = order_books_.find(event.symbol);
            if (it != order_books_.end()) {
                it->second->cancelOrder(event.order_id);
            }
            break;
        }
        case OrderEvent::Type::MODIFY: {
            std::lock_guard<std::mutex> lock(books_mutex_);
            auto it = order_books_.find(event.symbol);
            if (it != order_books_.end()) {
                it->second->modifyOrder(event.order_id, event.new_quantity);
            }
            break;
        }
    }
}

OrderBook* MatchingEngine::getOrCreateOrderBook(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(books_mutex_);
    auto it = order_books_.find(symbol);
    if (it == order_books_.end()) {
        auto book = std::make_unique<OrderBook>(symbol);
        
        // Set up callbacks for trade and BBO updates
        book->setTradeCallback([this, symbol](const Trade& trade) {
            // TODO: Implement trade notification via WebSocket
            // std::cout << "Trade: " << symbol << " @ " << trade.price 
            //          << " qty: " << trade.quantity << std::endl;
        });
        
        book->setBBOUpdateCallback([this, symbol](const std::string& sym, const BestBidOffer& bbo) {
            // TODO: Implement BBO update notification via WebSocket
            // std::cout << "BBO Update: " << symbol << std::endl;
        });
        
        it = order_books_.emplace(symbol, std::move(book)).first;
    }
    return it->second.get();
}

void MatchingEngine::startOrderProcessing() {
    running_ = true;
    processing_thread_ = std::thread(&MatchingEngine::processOrders, this);
}

void MatchingEngine::stopOrderProcessing() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        running_ = false;
    }
    queue_cv_.notify_one();
    
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }
}

} // namespace crypto_matching_engine 