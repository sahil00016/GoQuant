#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <chrono>
#include <variant>
#include <optional>

namespace crypto_matching_engine {

using OrderId = uint64_t;
using Price = double;
using Quantity = double;
using Timestamp = std::chrono::system_clock::time_point;

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderType {
    MARKET,
    LIMIT,
    IOC,  // Immediate-Or-Cancel
    FOK   // Fill-Or-Kill
};

struct Order {
    OrderId id;
    std::string symbol;
    OrderSide side;
    OrderType type;
    Quantity quantity;
    std::optional<Price> price;  // Required for LIMIT orders
    Timestamp timestamp;
    bool is_active{true};
};

struct Trade {
    OrderId maker_order_id;
    OrderId taker_order_id;
    std::string symbol;
    Price price;
    Quantity quantity;
    OrderSide aggressor_side;
    Timestamp timestamp;
};

struct OrderBookLevel {
    Price price;
    Quantity total_quantity;
    std::vector<Order> orders;  // Orders at this price level, sorted by time
};

struct BestBidOffer {
    std::optional<Price> best_bid;
    std::optional<Price> best_offer;
    std::optional<Quantity> best_bid_quantity;
    std::optional<Quantity> best_offer_quantity;
};

} // namespace crypto_matching_engine 