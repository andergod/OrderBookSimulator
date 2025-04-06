#include <iostream>
#include "config.hpp"
#include "orderBook.h"
#include <map>
#include <vector>

int main()
{
    std::cout << project_name << '\n';
    std::cout << project_version << '\n';

    orderBook Book;
    orderGenerator generator;
    
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    while (true) {
        order new_order =  generator.generateOrder();
        Book.addOrder(new_order);

        // Break after 5 secs of compiling
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        std::chrono::seconds elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);
        if (elapsed.count() > 1) {
            break;
        }
    }
    
    // Show results
    Book.showBook();

    return 0;
}

