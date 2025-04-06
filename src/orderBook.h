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

std::map<double, std::vector<order>> initiate_order_book();

class orderGenerator{
    private:
        long idCounter;
    public:
        orderGenerator();

        order generateOrder();
};