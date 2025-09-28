#include <cstdint>
#include <iostream>
#include <map>
#include "orderBook.h"
#include "common.h"
#include "config.hpp"
#include <random>
#include <cmath>
#include <vector>
#include <ctime>
#include <iomanip>
#include <unordered_map>
#include <array>
#include <memory>  
#include <algorithm>

void orderGenerator::ackCancel(std::int32_t orderId) {
    auto idx = idToIdx[orderId];
    // Algorithm so you can change the elements efficiently and deletes from record
    std::swap(activeIds[idx], activeIds.back());
    idToIdx[activeIds[idx]] = idx;   // update index of swapped element
    if (DEBUGMODE) printf("Deleting %d from Active Ids in OrderGen \n", orderId);
    activeIds.pop_back();
    idToIdx.erase(orderId);
}

amendOrder orderGenerator::cancelOrders() {
    std::int32_t randomId = activeIds[generateRandomInt(0, activeIds.size() - 1)];
    if (DEBUGMODE) printf("On the gen Book: We'll cancel order ID %d \n", randomId);
    fflush(stdout);
    ackCancel(randomId);
    return amendOrder(randomId, action::cancel);
}

amendOrder orderGenerator::modifyOrders() {
    std::int32_t randomId = activeIds[generateRandomInt(0, activeIds.size() - 1)];
    return amendOrder(randomId, action::modify, generateRandomPrice(MINPRICE, MAXPRICE - TICKSIZE));
}

// define class order Generator
orderGenerator::orderGenerator() : idGenerated(1) {}

orderReceived orderGenerator::generateOrder(){
    // we define an order from random variables (side and price)
    orderReceived o(generateRandomPrice(MINPRICE, MAXPRICE - TICKSIZE), generateRandomInt(1,10),
        generateSide(), std::chrono::system_clock::now(), idGenerated++);
    // Add active ID to the activeIds and idToIdx
    activeIds.push_back(o.order_id);
    idToIdx[o.order_id] = activeIds.size() - 1;
    return o;
}

void orderGenerator::showActiveId() {
    std::cout << "Active IDs: " << std::endl;
    std::int32_t count= 0;
    for (auto& mem: activeIds) {
        std::cout << mem << std::endl;
        ++count;
        if (count>10) {
            break;
        }
    }
}