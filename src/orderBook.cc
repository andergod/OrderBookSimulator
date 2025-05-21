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

/**
 * @brief Prints out hello world and tests the JSON Lib.
 *
 */

// Order book definition
orderBook::orderBook() : idCounter(1) {}

std::int32_t  orderBook::priceToIdx(const double price) {
    return std::int32_t (price - MINPRICE)/TICKSIZE;
}

// method for pushing a price into the book in case we don't find any match for it
void orderBook::pushOrder(const order& cleanRec, const std::int32_t priceIdx, const bool side) {
    // TODO: ADD to this function its addition to the lookupMap
    std::array<std::deque<order>, MAXTICKS> &desiredBook = (side) ? askBook : bidBook;
    std::int32_t &bestPxIdx = (side) ? bestAskIdx : bestBidIdx;
    // updating bestAsk/bestBid in case new order is better and not matching
    if (side) {
        bestPxIdx=(priceIdx < bestPxIdx) ? priceIdx : bestPxIdx;
    } else {
        bestPxIdx=(priceIdx > bestPxIdx) ? priceIdx : bestPxIdx;
    }
    desiredBook[priceIdx].push_back(cleanRec);   
}

void orderBook::updateNextWorstPxIdx(bool side) {
    std::int32_t* bestPxIdx = side ? &bestBidIdx : &bestAskIdx;
    // here it suggest me to use auto, mmm i am doubtul about using it except for "for" statemetns
    auto& book = side ? bidBook : askBook;
    std::int32_t px = *bestPxIdx;
    while (px >= 0 && px < MAXTICKS && book[px].empty()) {
        px += (side ? -1 : 1);
    }
    *bestPxIdx = px;
}

void orderBook::matchOrder(order &cleanRec, std::int32_t priceIdx, bool side, std::int32_t &bestPxIdx) {
    std::array<std::deque<order>, MAXTICKS> &book = (side) ? bidBook : askBook;

    if (side) {
        // sell order → match against bids (high to low)
        for (int px = bestPxIdx; px >= priceIdx && cleanRec.quantity > 0; --px) {
            matchAtPriceLevel(book[px], cleanRec);
            if (book[px].empty() && px == bestPxIdx) {
                updateNextWorstPxIdx(!side);
            }
        }
    } else {
        // buy order → match against asks (low to high)
        for (int px = bestPxIdx; px <= priceIdx && cleanRec.quantity > 0; ++px) {
            matchAtPriceLevel(book[px], cleanRec);
            if (book[px].empty() && px == bestPxIdx) {
                updateNextWorstPxIdx(!side);
            }
        }
    }
    // push leftover if not fully filled
    if (cleanRec.quantity > 0) {
        pushOrder(cleanRec, priceIdx, side);
    }
}

void orderBook::matchAtPriceLevel(std::deque<order> &level, order &cleanRec) {
    while (!level.empty() && cleanRec.quantity > 0) {
        order &matchingOrder = level.front();
        std::int32_t trades = std::min(cleanRec.quantity, matchingOrder.quantity);
        matchingOrder.quantity -= trades;
        cleanRec.quantity -= trades;

        if (matchingOrder.quantity == 0) {
        // TODO check that if we pop the order, we delete it from the lookUpMap
            level.pop_front();
        }
    }
}

void orderBook::addLimitOrder(orderReceived &received) {

    order cleanRec(this->idCounter++, received.quantity, std::chrono::system_clock::now());
    
    std::int32_t priceIdx = (received.price - MINPRICE)/TICKSIZE;
    std::int32_t bestPxIdx = (received.side) ? bestBidIdx : bestAskIdx;
    if (received.side) {
        // if sells
        (priceIdx <= bestPxIdx) ? matchOrder(cleanRec, priceIdx, received.side, bestPxIdx) : pushOrder(cleanRec, priceIdx, received.side);
    } else {
        // if buys
        (priceIdx >= bestPxIdx) ? matchOrder(cleanRec, priceIdx, received.side, bestPxIdx) : pushOrder(cleanRec, priceIdx, received.side);
    }

}

void orderBook::showBook() {
    // iterate over each price of the order book
    using BookLevel = std::array<std::deque<order>, MAXTICKS>;
    // Because i am making a array of references, I can't put them in a container
    // So, when references aren't enough, I use pointers which difficult the verbosity but are more flexibles
    std::array<BookLevel*, 2> orderBook = {&askBook, &bidBook};
    std::array<std::string_view, 2> bookName = {"Ask Book: ", "Bid Book"};
    for (std::int32_t j=0; j<orderBook.size(); ++j) {
        std::cout << bookName[j] << std::endl;
        BookLevel& book = *orderBook[j];
        for (std::int32_t i=0; i<book.size(); ++i) {
            double price = (i*TICKSIZE) + MINPRICE;
            const std::deque<order>&ordersAtPrice= book[i]; // The vector of orders at this price
            if (ordersAtPrice.empty()) {
                continue;
            }
            // Print the price level
            std::cout << "Price: " << price << std::endl;

            // iterate over each side (buy and sell) in a determined price
            for (std::int32_t k=0; k<std::min<std::size_t>(2, ordersAtPrice.size()); ++k) {
                const auto& o = ordersAtPrice[k];
                std::time_t orderTime = std::chrono::system_clock::to_time_t(o.timestamp);
                std::tm tm_order = *std::localtime(&orderTime);

                // show the relevant values
                std::cout << "  Order ID: " << o.order_id
                        << ", Quantity: " << o.quantity
                        << ", timestamp: " << std::put_time(&tm_order, "%Y-%m-%d %H:%M:%S")  << std::endl;
            }
            
        }   
    }
    std::cout << "Best Ask: " << (bestAskIdx*TICKSIZE) + MINPRICE << std::endl;
    std::cout << "Best Bid: " << (bestBidIdx*TICKSIZE) + MINPRICE << std::endl;
}
 
double generateRandomPrice(double min, double max) {
    // generate random price for orders
    // or thread local also heps
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(min, max);
    return std::round(dist(rng)*100)/100;
}

bool generateSide() {
    // generate random side for orders
    std::vector<bool> sides = {0, 1};
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0,sides.size() - 1);
    return sides[dist(rng)];
}

// define class order Generator
orderGenerator::orderGenerator() : idGenerated(1) {}

orderReceived orderGenerator::generateOrder(){
    // we define an order from random variables (side and price)
    orderReceived o;
    // a price cannot be equal to maxPrice or it will overflow the index
    o.price = generateRandomPrice(MINPRICE, MAXPRICE - 1);
    o.quantity = static_cast<std::int32_t>(generateRandomPrice(1,10));
    o.side = generateSide();
    o.timestamp = std::chrono::system_clock::now();
    return o;
}
