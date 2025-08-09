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
    // For loggin all prints
    freopen("log.txt", "w", stdout);
    std::conditional_t<BOOKTYPE == type::deque, dequeOrderBook, intrusiveOrderBook> Book;

    orderGenerator generator;

    // timer initialized     
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::int32_t count = 1;
    
    while (true) {
        if (count%7==0) {
            std::vector<std::int32_t> cancelOrders = Book.recModOrders(generator.modifyOrders());
            for (auto&cancelIds : cancelOrders) {
                    generator.ackCancel(cancelIds);
            }
        }
        else if (count%9==0) {
            Book.recCancelOrders(generator.cancelOrders());
        }
        // add the order to the book and match it if there is a good match
        else {
            std::vector<std::int32_t> cancelOrders = Book.addLimitOrder(generator.generateOrder());
            for (auto&cancelIds : cancelOrders) {
                generator.ackCancel(cancelIds);
            }
        }     
        ++count;
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
    // Book.showLookUpMap();
    generator.showActiveId();

    return 0;
}

