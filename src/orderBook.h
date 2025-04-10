#pragma once

#include <chrono>
#include <string> 
#include <map> 
#include <vector>
#include <array>
#include <deque>
#include <unordered_map>

struct order
{
    std::string order_id;
    double price;
    int quantity;
    std::string side; 
    std::chrono::system_clock::time_point timestamp;
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

struct tradeRecord
{
    // tradeRecord has two orders (buy and sell, and the actual transactions (trade struct))
    order order1;
    order order2;
    trade tradeDone;
};

class orderBook{
    private:
        void addToBook (order &received);
        // core of the class: maybe a bit over-engineered? 
        std::map<double, std::unordered_map<std::string, std::deque<order>>> order_book;
    public:
        // method for adding a limit order into the order book and match it if necessary
        void addLimitOrder(order &received);
        // show the contect of the book
        void showBook();
        // vector that holds the trades
        std::vector<tradeRecord> trades;
}; 

class orderGenerator{
    private:
        // keep count of the Ids for each order generated
        long idCounter;
    public:
        // class definition
        orderGenerator();
        // method to generate random orders
        order generateOrder();
};