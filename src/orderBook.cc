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

std::vector<int32_t> orderBook::addLimitOrder(orderReceived &received) {
    order cleanRec(received.order_id, received.quantity, std::chrono::system_clock::now());
    std::int32_t priceIdx = priceToIdx(received.price);
    std::int32_t bestPxIdx = (received.side == Side::Sell) ? bestBidIdx : bestAskIdx;
    // If sell then make it priceIdx <= bestPxIdex
    bool condition = (received.side == Side::Sell) ? (priceIdx <= bestPxIdx) : (priceIdx >= bestPxIdx);
    return (condition) ? matchOrder(cleanRec, priceIdx, received.side, bestPxIdx) : pushOrder(cleanRec, priceIdx, received.side);
}

void orderBook::recModOrders(amendOrder modOrder) {
    // TODO: Implement the recModOrder in main and recCancel as well
    OrderLocation &loc = lookUpMap[modOrder.order_id];
    std::array<std::deque<order>, MAXTICKS> &book = (loc.side == Side::Sell) ? askBook : bidBook;
    const order oldOrder = *loc.order_it;
    orderReceived newOrder(modOrder.price.value(), oldOrder.quantity, loc.side, std::chrono::system_clock::now(), oldOrder.order_id);
    // Erase the original order from the deque and delete for LookupMap. It will be re-added later on addLimitOrder
    book[loc.price_index].erase(loc.order_it);
    lookUpMap.erase(modOrder.order_id);
    // Re-add as a new limit order
    addLimitOrder(newOrder);
}

std::vector<int32_t> orderBook::matchOrder(order &cleanRec, std::int32_t priceIdx, Side side, std::int32_t &bestPxIdx) {
    // active orders
    std::array<std::deque<order>, MAXTICKS> &book = (side == Side::Sell) ? bidBook : askBook;
    std::vector<int32_t> matchedOrder;
    if (side == Side::Sell) {
        // sell order → match against bids (high to low)
        for (int px = bestPxIdx; px >= priceIdx && cleanRec.quantity > 0; --px) {
            std::vector<int32_t> atPriceMatched = matchAtPriceLevel(book[px], cleanRec);
            matchedOrder.insert(matchedOrder.end(), atPriceMatched.begin(), atPriceMatched.end());
            if (book[px].empty() && px == bestPxIdx) {
                updateNextWorstPxIdx(oppositeSide(side));
            }
        }
    } else {
        // buy order → match against asks (low to high)
        for (int px = bestPxIdx; px <= priceIdx && cleanRec.quantity > 0; ++px) {
            std::vector<int32_t> atPriceMatched = matchAtPriceLevel(book[px], cleanRec);
            matchedOrder.insert(matchedOrder.end(), atPriceMatched.begin(), atPriceMatched.end());
            if (book[px].empty() && px == bestPxIdx) {
                updateNextWorstPxIdx(oppositeSide(side));
            }
        }
    }
    // push leftover if not fully filled
    if (cleanRec.quantity > 0) {
        pushOrder(cleanRec, priceIdx, side);
    }
    return matchedOrder;
}

// method for pushing a price into the book in case we don't find any match for it
std::vector<int32_t> orderBook::pushOrder(const order& cleanRec, const std::int32_t priceIdx, const Side side) {
    std::array<std::deque<order>, MAXTICKS> &desiredBook = (side == Side::Sell) ? askBook : bidBook;
    std::int32_t &bestPxIdx = (side == Side::Sell) ? bestAskIdx : bestBidIdx;
    // updating bestAsk/bestBid in case new order is better and not matching
    if (side == Side::Sell) {
        bestPxIdx=(priceIdx < bestPxIdx) ? priceIdx : bestPxIdx;
    } else {
        bestPxIdx=(priceIdx > bestPxIdx) ? priceIdx : bestPxIdx;
    }
    desiredBook[priceIdx].push_back(cleanRec);  
    lookUpMap.emplace(cleanRec.order_id, OrderLocation(side, priceIdx, std::prev(desiredBook[priceIdx].end())));
    return {};
}

void orderBook::updateNextWorstPxIdx(const Side side) {
    std::int32_t &bestPxIdx = (side == Side::Sell) ? bestBidIdx : bestAskIdx;
    auto& book = (side==Side::Sell) ? bidBook : askBook;
    std::int32_t px = bestPxIdx;
    while (px >= 0 && px < MAXTICKS && book[px].empty()) {
        px += (side==Side::Sell ? -1 : 1);
    }
    bestPxIdx = px;
}

std::vector<int32_t> orderBook::matchAtPriceLevel(std::deque<order> &level, order &cleanRec) {
    std::vector<int32_t> matchedId;
    while (!level.empty() && cleanRec.quantity > 0) {
        order &matchingOrder = level.front();
        std::int32_t trades = std::min(cleanRec.quantity, matchingOrder.quantity);
        matchingOrder.quantity -= trades;
        cleanRec.quantity -= trades;


        if (matchingOrder.quantity == 0) {
            matchedId.push_back(matchingOrder.order_id);
            lookUpMap.erase(matchingOrder.order_id);
            level.pop_front();
        }
    }
    return matchedId;
}

std::int32_t  orderBook::priceToIdx(const double price) {
    return static_cast<std::int32_t>((price - MINPRICE)/TICKSIZE);
}

Side orderBook::oppositeSide(const Side side) {
    return static_cast<Side>(!static_cast<bool>(side));
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

void orderBook::showLookUpMap () {
    int count = 0;
    for (const auto& entry : lookUpMap) {
        std::cout << "Order ID: " << entry.first << std::endl;
        std::cout << "Side: (Is Sell?) " << static_cast<bool>(entry.second.side) << std::endl;
        std::cout << "Price Index: " << (entry.second.price_index * TICKSIZE) + MINPRICE << std::endl;
        std::cout << "--------------------------" << std::endl;

        if (++count >= 10) break;  // stop after 10 entries
    }
}
 
double generateRandomPrice(double min, double max) {
    // generate random price for orders
    // or thread local also heps
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(min, max);
    return std::round(dist(rng)*100)/100;
}

Side generateSide() {
    // generate random side for orders
    std::mt19937 rng(std::random_device{}());
    std::bernoulli_distribution dist(0.5);
    return static_cast<Side>(dist(rng));
}

void orderGenerator::ackCancel(std::int32_t orderId) {
    auto idx = idToIdx[orderId];
    // Algorithm so you can change the elements efficiently and deletes from record
    std::swap(activeIds[idx], activeIds.back());
    idToIdx[activeIds[idx]] = idx;   // update index of swapped element
    activeIds.pop_back();
    idToIdx.erase(orderId);
}

amendOrder orderGenerator::cancelOrders() {
    std::int32_t randomId = activeIds[generateRandomPrice(0, static_cast<double>(activeIds.size()))];
    auto idx = idToIdx[randomId];
    // Algorithm so you can change the elements efficiently and deletes from record
    ackCancel(randomId);
    return amendOrder(randomId, action::cancel);
}

amendOrder orderGenerator::modifyOrders() {
    std::int32_t randomId = activeIds[generateRandomPrice(0, static_cast<double>(activeIds.size()))];
    return amendOrder(randomId, action::modify, generateRandomPrice(MINPRICE, MAXPRICE - TICKSIZE));
}

// define class order Generator
orderGenerator::orderGenerator() : idGenerated(1) {}

orderReceived orderGenerator::generateOrder(){
    // we define an order from random variables (side and price)
    orderReceived o(generateRandomPrice(MINPRICE, MAXPRICE - TICKSIZE), static_cast<std::int32_t>(generateRandomPrice(1,10)),
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