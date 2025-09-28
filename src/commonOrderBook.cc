#include <cstdint>
#include <iostream>
#include <map>
#include "orderBook.h"
#include "common.cc"
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

// template<typename Derived>
// void orderBook<Derived>::updateNextWorstPxIdxImpl(const Side side) {
//     auto& derived = *static_cast<Derived*>(this);
//     std::int32_t &bestPxIdx = (side == Side::Sell) ? derived.bestBidIdx : derived.bestAskIdx;
//     auto& book = (side == Side::Sell) ? derived.bidBook : derived.askBook;
    
//     std::int32_t px = bestPxIdx;
//     while (px >= 0 && px < MAXTICKS && book[px].empty()) {
//         px += (side == Side::Sell ? -1 : 1);
//     }
//     bestPxIdx = px;
//     if (DEBUGMODE) {
//         printf("Using default updateNextWorstPxIdx implementation\n");
//     }
// }

