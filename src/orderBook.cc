#pragma once

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
