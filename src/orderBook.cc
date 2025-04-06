#include <cstdint>
#include <iostream>
#include <map>
#include "orderBook.h"
#include <random>
#include <cmath>
#include <vector>
#include <ctime>
#include <iomanip>
#include <unordered_map>

/**
 * @brief Prints out hello world and tests the JSON Lib.
 *
 */

void orderBook::addToBook(order& received) {
    order_book[received.price][received.side].push_back(received);
}

void orderBook::addLimitOrder(order &received) {
    received.timestamp = std::chrono::system_clock::now();
    auto price_it = order_book.find(received.price);
    std::string opposing_order = (received.side == "Buy") ? "Sell" : "Buy";

    if (price_it == order_book.end()) {
        addToBook(received);
    } else {
        auto &side_map = price_it->second;
        auto side_it = side_map.find(opposing_order);
        if (side_it != side_map.end()) {
            if (side_it->second.empty()) {
                addToBook(received);
            } else {
                order &match = side_it->second.front();
                int quantityTraded = std::min(match.quantity, received.quantity);
    
                static int tradeId = 1;
                trade tradeDone(std::to_string(tradeId++), received.price, quantityTraded, received.timestamp);
                trades.push_back({received, match, tradeDone});
    
                match.quantity -= quantityTraded;
                received.quantity -= quantityTraded;
    
                if (match.quantity == 0) {
                    side_it->second.pop_front();
                }
                if (received.quantity > 0) {
                    addToBook(received);
                }          
            }
        } else {
            addToBook(received);    
        }
    }
}

void orderBook::showBook() {
    for (auto& pair : order_book) {
        const double &price = pair.first; // The price is the key
        const std::unordered_map<std::string, std::deque<order>>& ordersAtPrice = pair.second; // The vector of orders at this price

        // Print the price level
        std::cout << "Price: " << price << std::endl;

        for (const auto& map: ordersAtPrice) {
            const std::string &side = map.first;
            const std::deque<order> &orderAtSide = map.second;

            std::cout << "Side: " << side << std::endl;

            // Iterating over the orders at this price
            for (const auto& o : orderAtSide) {
                std::time_t time_now = std::chrono::system_clock::to_time_t(o.timestamp);
                std::tm tm_now = *std::localtime(&time_now);

                std::cout << "  Order ID: " << o.order_id
                        << ", Quantity: " << o.quantity
                        << ", Side: " << o.side
                        << ", timestamp: " << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S")  << std::endl;
            }
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
    std::vector<std::string> sides = {"Buy", "Sell"};
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
    o.timestamp = std::chrono::system_clock::now();
    return o;
}
