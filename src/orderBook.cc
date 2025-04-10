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

 // method for pushing a price into the book in case we don't find any match for it
void orderBook::addToBook(order& received) {
    order_book[received.price][received.side].push_back(received);
}

void orderBook::addLimitOrder(order &received) {
    received.timestamp = std::chrono::system_clock::now();
    auto price_it = order_book.find(received.price);
    // we'll match it to an opposing order
    std::string opposing_order = (received.side == "Buy") ? "Sell" : "Buy";

    // If no price found in the order book
    if (price_it == order_book.end()) {
        // push it into the book
        addToBook(received);
    } else {
        // If we find a price in the order book 
        auto &side_map = price_it->second;
        auto side_it = side_map.find(opposing_order);
        // if we find a opposing side. If the receiveds is buy, the match should live on sell
        if (side_it != side_map.end()) {
            // in the opposide is empty, then there is nothing to match
            if (side_it->second.empty()) {
                addToBook(received);
            } else {
                // if the opposite side is not empty, then we can found a match for our current order
                // taking the order at the front of the deque
                order &match = side_it->second.front();

                // the quantity traded will be the minimum between the buy and sell order
                int quantityTraded = std::min(match.quantity, received.quantity);
    
                // this is a way i keep count of the tradeId
                static int tradeId = 1;

                // defining trade from the buy and sell order
                trade tradeDone(std::to_string(tradeId++), received.price, quantityTraded, received.timestamp);
                
                // pusing it into the vector trades
                trades.push_back({received, match, tradeDone});
    
                // updating the quantities for trading for the received order and matcher order                
                match.quantity -= quantityTraded;
                received.quantity -= quantityTraded;
    
                // if the quantity matched == 0, means the matched orders was fulfilled and should be kick out the orderBook
                if (match.quantity == 0) {
                    side_it->second.pop_front();
                }

                // if there is any quantity remaining from the received order, we should add it into the order book
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
    // method for showing the content of the order book
    
    // iterate over each price of the order book
    for (auto& pair : order_book) {
        const double &price = pair.first; // The price is the key
        const std::unordered_map<std::string, std::deque<order>>& ordersAtPrice = pair.second; // The vector of orders at this price

        // Print the price level
        std::cout << "Price: " << price << std::endl;

        // iterate over each side (buy and sell) in a determined price
        for (const auto& map: ordersAtPrice) {
            const std::string &side = map.first;
            const std::deque<order> &orderAtSide = map.second;

            std::cout << "Side: " << side << std::endl;

            // Iterating over the orders at a determined side
            for (const auto& o : orderAtSide) {
                std::time_t time_now = std::chrono::system_clock::to_time_t(o.timestamp);
                std::tm tm_now = *std::localtime(&time_now);

                // show the relevant values
                std::cout << "  Order ID: " << o.order_id
                        << ", Quantity: " << o.quantity
                        << ", Side: " << o.side
                        << ", timestamp: " << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S")  << std::endl;
            }
        }
    }   
}
 
double generateRandomPrice(double min, double max) {
    // generate random price for orders
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(min, max);
    return std::round(dist(rng)*100)/100;
}

std::string generateSide() {
    // generate random side for orders
    std::vector<std::string> sides = {"Buy", "Sell"};
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0,sides.size() - 1);
    return sides[dist(rng)];
}

// define class order Generator
orderGenerator::orderGenerator() : idCounter(1) {}

order orderGenerator::generateOrder(){
    // we define an order from random variables (side and price)
    order o;
    o.order_id = std::to_string(idCounter++);
    o.price = generateRandomPrice(150, 200);
    o.quantity = static_cast<int>(generateRandomPrice(1,10));
    o.side = generateSide();
    o.timestamp = std::chrono::system_clock::now();
    return o;
}
