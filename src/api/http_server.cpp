#include "http_server.hpp"
#include "matching_engine.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace crypto_matching_engine {

HttpServer::HttpServer(MatchingEngine& engine) : engine_(engine) {}

void HttpServer::start(int port) {
    server_.Post("/order", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            Order order;
            order.id = j["id"].get<OrderId>();
            order.symbol = j["symbol"].get<std::string>();
            order.side = j["side"].get<std::string>() == "buy" ? OrderSide::BUY : OrderSide::SELL;
            order.type = [&]() {
                std::string type = j["type"].get<std::string>();
                if (type == "market") return OrderType::MARKET;
                if (type == "limit") return OrderType::LIMIT;
                if (type == "ioc") return OrderType::IOC;
                if (type == "fok") return OrderType::FOK;
                throw std::runtime_error("Invalid order type");
            }();
            order.quantity = j["quantity"].get<Quantity>();
            if (j.contains("price")) {
                order.price = j["price"].get<Price>();
            }
            order.timestamp = std::chrono::system_clock::now();

            engine_.submitOrder(order.symbol, order);
            res.status = 200;
            res.set_content("Order submitted successfully", "text/plain");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(e.what(), "text/plain");
        }
    });

    server_.Get("/orderbook/:symbol", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string symbol = req.path_params.at("symbol");
            auto depth = engine_.getOrderBookDepth(symbol, 10);
            json j = depth;
            res.status = 200;
            res.set_content(j.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(e.what(), "text/plain");
        }
    });

    server_.Delete("/order/:symbol/:id", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            std::string symbol = req.path_params.at("symbol");
            OrderId order_id = std::stoull(req.path_params.at("id"));
            engine_.cancelOrder(symbol, order_id);
            res.status = 200;
            res.set_content("Order cancelled successfully", "text/plain");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(e.what(), "text/plain");
        }
    });

    std::cout << "Starting HTTP server on port " << port << std::endl;
    server_.listen("0.0.0.0", port);
}

} // namespace crypto_matching_engine 