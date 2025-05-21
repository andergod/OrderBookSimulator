#pragma once

#include <chrono>
#include <string> 
#include <map> 
#include <vector>
#include <array>
#include <deque>
#include <unordered_map>
#include "config.hpp"

// represent the order we receive raw
struct orderReceived
{
    double price;
    std::int32_t quantity;
    bool side;  // 0 means buy, and 1 sell
    std::chrono::system_clock::time_point timestamp;
};

// Making a mimic as what info do we actually stores for our L2 book
struct order
{
    const std::int32_t order_id;
    std::int32_t quantity;
    std::chrono::system_clock::time_point timestamp;
    order(const std::int32_t id, const std::int32_t q, const std::chrono::system_clock::time_point t)
    : order_id(id), quantity(q), timestamp(t) {}
};

struct trade
{
    std::string trade_id;
    double price;
    int quantity;
    std::chrono::system_clock::time_point timestamp;
    trade(const std::string& id,const double& p,const int& q, const std::chrono::system_clock::time_point& t)
    : trade_id(id), price(p), quantity(q), timestamp(t) {}
};

struct OrderLocation {
    bool is_bid;                   // Side
    std::int32_t price_index;               // Price level index in your array
    std::deque<order>::iterator order_it;  // Iterator pointing to the order inside the deque
};

struct tradeRecord
{
    // tradeRecord has two orders (buy and sell, and the actual transactions (trade struct))
    order order1;
    order order2;
    trade tradeDone;
};

class orderBook{
    private:
        void pushOrder (const order &received, const std::int32_t priceIdx, const bool side);
        void matchOrder(order &cleanRec, std::int32_t priceIdx, bool side, std::int32_t &bestPxIdx);
        void updateNextWorstPxIdx(const bool side);
        void matchAtPriceLevel(std::deque<order> &level, order &cleanRec);
        std::int32_t priceToIdx(const double price);
        std::unordered_map<std::string, OrderLocation> lookUpMap;
        std::array<std::deque<order>, MAXTICKS> bidBook;
        std::array<std::deque<order>, MAXTICKS> askBook;
        std::int32_t bestBidIdx = -1;
        std::int32_t bestAskIdx = MAXTICKS;
        std::int32_t idCounter;
    public:
        //orderBook definition    
        orderBook();
        // method for adding a limit order into the order book and match it if necessary
        void addLimitOrder(orderReceived &received);
        // show the contect of the book
        void showBook();
        // vector that holds the trades
        std::vector<tradeRecord> trades;
}; 

class orderGenerator{
    private:
        // keep count of the Ids for each order generated
        std::int32_t idGenerated;
    public:
        // class definition
        orderGenerator();
        // method to generate random orders
        orderReceived generateOrder();
};