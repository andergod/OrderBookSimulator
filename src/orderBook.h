#pragma once

#include <chrono>
#include <string> 
#include <map> 
#include <vector>
#include <array>
#include <deque>
#include <unordered_map>
#include "config.hpp"
#include <optional>

enum class Side:bool {
    Buy = false,
    Sell = true
};

// represent the order we receive raw
struct orderReceived
{
    double price;
    std::int32_t quantity;
    Side side;  // 0 means buy, and 1 sell
    std::chrono::system_clock::time_point timestamp;
    std::int32_t order_id;
    orderReceived(double p, std::int32_t q, Side s, std::chrono::system_clock::time_point  t, std::int32_t oId)
    : price(p), quantity(q), side(s), timestamp(t), order_id(oId) {}
};

// Making a mimic as what info do we actually stores for our L2 book
struct order
{
    std::int32_t order_id;
    std::int32_t quantity;
    std::chrono::system_clock::time_point timestamp;
    order(const std::int32_t id, const std::int32_t q, const std::chrono::system_clock::time_point t)
    : order_id(id), quantity(q), timestamp(t) {}
};
// Not using yet
struct trade
{
    std::string trade_id;
    double price;
    int quantity;
    std::chrono::system_clock::time_point timestamp;
    // is this always gonna be const? i am not making any change while defining
    trade(const std::string id, const double p, const int q, const std::chrono::system_clock::time_point t)
    : trade_id(id), price(p), quantity(q), timestamp(t) {}
};

struct OrderLocation {
    Side side;                   
    std::int32_t price_index;      
    std::deque<order>::iterator order_it; // Iterator pointing to the order inside the deque
    OrderLocation() = default;
    OrderLocation(const Side s, std::int32_t pxIdx, std::deque<order>::iterator it)
    : side(s), price_index(pxIdx), order_it(it) {}
};

struct tradeRecord
{
    // tradeRecord has two orders (buy and sell, and the actual transactions (trade struct))
    order order1;
    order order2;
    trade tradeDone;
};

enum class action:bool {
    modify = true,
    cancel = false
};

struct amendOrder {
    const std::int32_t order_id;
    const action act;
    const std::optional<double> price;
    amendOrder(const std::int32_t id, const action a, const std::optional<double> p = std::nullopt)
    : order_id(id), act(a), price(p) {}
};

class orderBook{
    private:
        std::vector<int32_t> pushOrder (const order &received, const std::int32_t priceIdx, const Side side);
        std::vector<int32_t> matchOrder(order &cleanRec, std::int32_t priceIdx, Side side, std::int32_t &bestPxIdx);
        Side oppositeSide(const Side side);
        void updateNextWorstPxIdx(const Side side);
        std::vector<int32_t> matchAtPriceLevel(std::deque<order> &level, order &cleanRec);
        std::int32_t priceToIdx(const double price);
        void recModOrders(amendOrder modOrder);
        std::unordered_map<std::int32_t, OrderLocation> lookUpMap;
        std::array<std::deque<order>, MAXTICKS> bidBook;
        std::array<std::deque<order>, MAXTICKS> askBook;
        std::int32_t bestBidIdx = -1;
        std::int32_t bestAskIdx = MAXTICKS;
        std::int32_t idCounter;
    public:
        //orderBook definition    
        orderBook();
        // method for adding a limit order into the order book and match it if necessary
        std::vector<int32_t> addLimitOrder(orderReceived &received);
        // show the contect of the book
        void showBook();
        void showLookUpMap();
        // vector that holds the trades
        std::vector<tradeRecord> trades;
}; 

class orderGenerator{
    private:
        // keep count of the Ids for each order generated
        std::int32_t idGenerated;
        std::vector<std::int32_t> activeIds;
        std::unordered_map<std::int32_t, std::size_t> idToIdx;
    public:
        // class definition
        orderGenerator();
        amendOrder cancelOrders();
        amendOrder modifyOrders();
        void ackCancel(std::int32_t orderId);
        void showActiveId();
        // method to generate random orders
        orderReceived generateOrder();
};