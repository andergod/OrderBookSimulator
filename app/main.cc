#include <iostream>
#include "config.hpp"
#include "orderBook.h"
#include <cstdint>
#include <map>
#include <vector>

int main()
{
    std::cout << project_name << '\n';
    std::cout << project_version << '\n';
  
    orderBook Book;
    orderGenerator generator;

    // timer initialized     
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    
    while (true) {
        // create an order to add to the book
        orderReceived new_order =  generator.generateOrder();
        // add the order to the book and match it if there is a good match
        Book.addLimitOrder(new_order);

        // keep track of the time passed
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::chrono::seconds elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);

        // if it has passed more than 1 sec, break the loop
        if (elapsed.count() > 1) {
            break;
        }
    }
    
    // Show results
    Book.showBook();

    return 0;
}

