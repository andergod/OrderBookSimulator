#include <cstdint>
#include <iostream>
#include <map>
#include "orderBook.h"
#include <random>
#include <cmath>
#include <vector>

/**
 * @brief Prints out hello world and tests the JSON Lib.
 *
 */

void orderBook::addOrder(const order &received) {
    order_book[received.price].push_back(received);
};

void orderBook::showBook() {
    for (auto& pair : order_book) {
        const double &price = pair.first; // The price is the key
        const std::vector<order>& ordersAtPrice = pair.second; // The vector of orders at this price

        // Print the price level
        std::cout << "Price: " << price << std::endl;

        // Iterating over the orders at this price
        for (const auto& o : ordersAtPrice) {
            std::cout << "  Order ID: " << o.order_id
                    << ", Quantity: " << o.quantity
                    << ", Side: " << o.side << std::endl;
        }
    }   
}
 
std::map<double, std::vector<order>> initiate_order_book() {
    return std::map<double, std::vector<order>>{};
}

double generateRandomPrice(double min, double max) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(min, max);
    return std::round(dist(rng)*100)/100;
}

std::string generateSide() {
    std::vector<std::string> sides = {"buy", "sell"};
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0,sides.size() - 1);
    return sides[dist(rng)];
}

orderGenerator::orderGenerator() : idCounter(1) {}

order orderGenerator::generateOrder(){
    order o;
    o.order_id = std::to_string(idCounter++);
    o.price = generateRandomPrice(150, 200);
    o.quantity = static_cast<int>(generateRandomPrice(1,10));
    o.side = generateSide();
    return o;
}
