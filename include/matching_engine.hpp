#pragma once

#include "order_book.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace crypto_matching_engine {

class MatchingEngine {
public:
    MatchingEngine();
    ~MatchingEngine();

    // Order management
    bool submitOrder(const std::string& symbol, Order order);
    bool cancelOrder(const std::string& symbol, OrderId order_id);
    bool modifyOrder(const std::string& symbol, OrderId order_id, Quantity new_quantity);

    // Market data
    BestBidOffer getBBO(const std::string& symbol) const;
    std::vector<std::pair<Price, Quantity>> getOrderBookDepth(const std::string& symbol, size_t levels) const;

    // API endpoints
    // void startServer(uint16_t port);
    // void stopServer();

private:
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> order_books_;
    mutable std::mutex books_mutex_;

    // Server thread
    std::thread server_thread_;
    std::atomic<bool> running_{false};

    // Order processing queue
    struct OrderEvent {
        enum class Type { SUBMIT, CANCEL, MODIFY } type;
        std::string symbol;
        Order order;
        OrderId order_id;
        Quantity new_quantity;
    };

    std::queue<OrderEvent> order_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread processing_thread_;

    // Internal methods
    void processOrders();
    void handleOrderEvent(const OrderEvent& event);
    OrderBook* getOrCreateOrderBook(const std::string& symbol);
    void startOrderProcessing();
    void stopOrderProcessing();
};

} // namespace crypto_matching_engine 