#pragma once

#include <chrono>
#include <string> 
#include <map> 
#include <vector>

struct order
{
    std::string order_id;
    double price;
    int quantity;
    std::string side; 
};

class orderBook{
    private:
        std::map<double, std::vector<order>> order_book;
    public:
        void addOrder(const order &received);
        void showBook();
}; 

std::map<double, std::vector<order>> initiate_order_book();

class orderGenerator{
    private:
        long idCounter;
    public:
        orderGenerator();

        order generateOrder();
};