#include "matching_engine.hpp"
#include "http_server.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace crypto_matching_engine;

// Helper function to generate random orders
Order generateRandomOrder(const std::string& symbol) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> side_dist(0, 1);
    static std::uniform_int_distribution<> type_dist(0, 3);
    static std::uniform_real_distribution<> price_dist(100.0, 1000.0);
    static std::uniform_real_distribution<> quantity_dist(0.1, 10.0);
    static OrderId next_id = 1;

    Order order;
    order.id = next_id++;
    order.symbol = symbol;
    order.side = side_dist(gen) == 0 ? OrderSide::BUY : OrderSide::SELL;
    order.type = static_cast<OrderType>(type_dist(gen));
    order.price = price_dist(gen);
    order.quantity = quantity_dist(gen);
    order.timestamp = std::chrono::system_clock::now();

    return order;
}

// Helper function to convert order to JSON
json orderToJson(const Order& order) {
    json j;
    j["id"] = order.id;
    j["symbol"] = order.symbol;
    j["side"] = order.side == OrderSide::BUY ? "buy" : "sell";
    j["type"] = [&]() {
        switch (order.type) {
            case OrderType::MARKET: return "market";
            case OrderType::LIMIT: return "limit";
            case OrderType::IOC: return "ioc";
            case OrderType::FOK: return "fok";
            default: return "unknown";
        }
    }();
    j["price"] = order.price;
    j["quantity"] = order.quantity;
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        order.timestamp.time_since_epoch()).count();
    return j;
}

// Helper function to convert trade to JSON
json tradeToJson(const Trade& trade) {
    json j;
    j["maker_order_id"] = trade.maker_order_id;
    j["taker_order_id"] = trade.taker_order_id;
    j["symbol"] = trade.symbol;
    j["price"] = trade.price;
    j["quantity"] = trade.quantity;
    j["aggressor_side"] = (trade.aggressor_side == OrderSide::BUY) ? "buy" : "sell";
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        trade.timestamp.time_since_epoch()).count();
    return j;
}

int main() {
    try {
        // Create and start the matching engine
        MatchingEngine engine;

        // Create and start the HTTP server
        HttpServer server(engine);
        std::thread server_thread([&server]() {
            try {
                server.start(8081);
            } catch (const std::exception& e) {
                std::cerr << "HTTP Server Error: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown HTTP Server Error" << std::endl;
            }
        });

        std::this_thread::sleep_for(std::chrono::seconds(1)); // Add a small delay to ensure server starts

        // Generate and submit some test orders
        std::string symbol = "BTC/USD";
        for (int i = 0; i < 10; ++i) {
            Order order = generateRandomOrder(symbol);
            std::cout << "Submitting order: " << orderToJson(order).dump() << std::endl;
            engine.submitOrder(symbol, order);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Keep the main thread alive
        server_thread.join();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 