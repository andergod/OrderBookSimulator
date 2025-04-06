#include <iostream>
#include "config.hpp"
#include "orderBook.h"
#include <map>
#include <vector>

int main()
{
    std::cout << project_name << '\n';
    std::cout << project_version << '\n';

    std::map<double, std::vector<order>> order_book = initiate_order_book();
    orderGenerator generator;
    
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    while (true) {
        order new_order = generator.generateOrder();
        order_book[new_order.price].push_back(new_order);

        // Break after 5 secs of compiling
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::chrono::seconds elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);
        if (elapsed.count() > 5) {
            break;
        }
    }
    
    // Show results
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

    return 0;
}
