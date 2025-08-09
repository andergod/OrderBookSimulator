#include <cstdint>
#include <iostream>
#include <map>
#include "orderBook.h"
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

/**
 * @brief Prints out hello world and tests the JSON Lib.
 *
 */
double generateRandomPrice(double min, double max) {
    // generate random price for orders
    // or thread local also heps
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(min, max);
    return std::round(dist(rng)/TICKSIZE)*TICKSIZE;
}

std::int32_t generateRandomInt(std::int32_t min, std::int32_t max) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int32_t> dist(min, max);
    return dist(rng);
}

Side generateSide() {
    // generate random side for orders
    std::mt19937 rng(std::random_device{}());
    std::bernoulli_distribution dist(0.5);
    return static_cast<Side>(dist(rng));
}

std::int32_t  priceToIdx(const double price) {
    return static_cast<std::int32_t>((price - MINPRICE)/TICKSIZE);
}

Side oppositeSide(const Side side) {
    return static_cast<Side>(!static_cast<bool>(side));
}

OrderPool::OrderPool(std::int32_t size) {
    pool.resize(size);
    for (auto& order : pool) {
        freeList.push_back(&order);
    }
}

OrderIntrusive* OrderPool::allocate(std::int32_t id, std::int32_t quantity, std::chrono::system_clock::time_point timestamp) {
    if (freeList.empty()) {
        throw std::bad_alloc();
    }
    OrderIntrusive* order = freeList.back();
    freeList.pop_back();
    // another case where we create a pointer but not sure why or where? 
    *order = OrderIntrusive(id, quantity, timestamp);
    return order;
}

void OrderPool::deallocate(OrderIntrusive* order) {
    freeList.push_back(order);
}
