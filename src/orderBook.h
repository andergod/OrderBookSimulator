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
    order order1;
    order order2;
    trade tradeDone;
};

class orderBook{
    private:
        void addToBook (order &received);
        std::map<double, std::unordered_map<std::string, std::deque<order>>> order_book;
    public:
        void addLimitOrder(order &received);
        void showBook();
        std::vector<tradeRecord> trades;
}; 

class orderGenerator{
    private:
        long idCounter;
    public:
        orderGenerator();

        order generateOrder();
};