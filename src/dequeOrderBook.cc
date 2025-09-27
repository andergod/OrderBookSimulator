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

// order Book definition
dequeOrderBook::dequeOrderBook() {}

std::vector<int32_t> dequeOrderBook::addLimitOrderImpl(orderReceived received) {
    std::shared_ptr<order> cleanRecPt = std::make_shared<order>(received.order_id, received.quantity, std::chrono::system_clock::now());
    std::int32_t priceIdx = priceToIdx(received.price);
    std::int32_t bestPxIdx = (received.side == Side::Sell) ? bestBidIdx : bestAskIdx;
    // If sell then make it priceIdx <= bestPxIdex
    bool condition = (received.side == Side::Sell) ? (priceIdx <= bestPxIdx) : (priceIdx >= bestPxIdx);
    if (DEBUGMODE) {
        printf("Adding limit order for Id %d \n", cleanRecPt->order_id);
        printf("Index Location: %d; ", priceIdx);
        printf("Side: %s; ", received.side== Side::Sell ? "Sell" : "Buy");
        printf("Quantity: %d \n", cleanRecPt->quantity);
        fflush(stdout);
    }
    return (condition) ? matchOrder(cleanRecPt, priceIdx, received.side, bestPxIdx) : pushOrder(cleanRecPt, priceIdx, received.side);
}

// // class orderBookWithLogs::{
//             base dequeOrderBook;
// //     private:
// //         virtual void addLimitOrderImpl(const Side side)override;

//                 print("I am doing addLimitOrderImpl")


std::vector<int32_t> dequeOrderBook::modifyOrderImpl(amendOrder modOrder) {
    auto it = lookUpMap.find(modOrder.order_id);
    if (it == lookUpMap.end()) {
        printf("Order Not found, double check code");
        return {};
    } else {
        OrderLocation &loc = it->second;
        std::array<std::deque<std::shared_ptr<order>>, MAXTICKS> &book = (loc.side == Side::Sell) ? askBook : bidBook;
        // I will bet this will break something, as I am adding it to a deque, where needs to be copied
        order oldOrder = *loc.order_pt;
        orderReceived newOrder = orderReceived(modOrder.price.value(), oldOrder.quantity, loc.side, std::chrono::system_clock::now(), oldOrder.order_id);
        if (DEBUGMODE) {
            printf("Order ID to Modify: %d \n", modOrder.order_id);
            printf("Old order values: order_id %d, and quantity %d \n", oldOrder.order_id, oldOrder.quantity);
            printf("Change price_index from %d to %d \n", loc.price_index, priceToIdx(modOrder.price.value()));
            fflush(stdout);
        }
        // Erase the original order from the deque and delete for LookupMap. It will be re-added later on addLimitOrder
        auto& dq = book[loc.price_index];
        auto it = std::find(dq.begin(), dq.end(), loc.order_pt);
        dq.erase(it);
        lookUpMap.erase(modOrder.order_id);
        // Re-add as a new limit order
        if (DEBUGMODE) printf("Calling addLimitOrder for order %d\n", modOrder.order_id);
        std::vector<int32_t> canOrders = addLimitOrder(newOrder);
        if (DEBUGMODE) printf("Returned from addLimitOrder for order %d\n", modOrder.order_id);
        // Check that teh full lookUpMap is correct
        return canOrders;
    }
}

void dequeOrderBook::CheckLookUpMap(std::unordered_map<std::int32_t, OrderLocation> &lookUpMap) {
    for (auto &pair: lookUpMap) {
        int32_t key = pair.first;
        OrderLocation &loc = pair.second;
        if (key != loc.order_pt->order_id) {
            printf("Error during checking the lookUpMap is legit for order %d \n", key);
            fflush(stdout);
            printf("Stop");
        }
    }
}

void dequeOrderBook::cancelOrderImpl(amendOrder canOrder) {
    OrderLocation loc = lookUpMap[canOrder.order_id];
    std::array<std::deque<std::shared_ptr<order>>, MAXTICKS> &book = (loc.side == Side::Sell) ? askBook : bidBook;
    if (DEBUGMODE) printf("On the order Book: We'll cancel order ID %d \n", canOrder.order_id);
    // Erase the original order from the deque and delete for LookupMap
    auto& dq = book[loc.price_index];
    auto it = std::find(dq.begin(), dq.end(), loc.order_pt);
    dq.erase(it);
    lookUpMap.erase(canOrder.order_id);
}

std::vector<int32_t> dequeOrderBook::matchOrder(std::shared_ptr<order> cleanRec, std::int32_t priceIdx, Side side, std::int32_t &bestPxIdx) {
    // active orders
    std::array<std::deque<std::shared_ptr<order>>, MAXTICKS> &book = (side == Side::Sell) ? bidBook : askBook;
    std::vector<int32_t> matchedOrder;
    if (side == Side::Sell) {
        // sell order → match against bids (high to low)
        for (int px = bestPxIdx; px >= priceIdx && cleanRec->quantity > 0; --px) {
            std::vector<int32_t> atPriceMatched = matchAtPriceLevel(book[px], cleanRec);
            matchedOrder.insert(matchedOrder.end(), atPriceMatched.begin(), atPriceMatched.end());
            if (book[px].empty() && px == bestPxIdx) {
                updateNextWorstPxIdxImpl(oppositeSide(side));
            }
        }
    } else {
        // buy order → match against asks (low to high)
        for (int px = bestPxIdx; px <= priceIdx && cleanRec->quantity > 0; ++px) {
            std::vector<int32_t> atPriceMatched = matchAtPriceLevel(book[px], cleanRec);
            matchedOrder.insert(matchedOrder.end(), atPriceMatched.begin(), atPriceMatched.end());
            if (book[px].empty() && px == bestPxIdx) {
                updateNextWorstPxIdxImpl(oppositeSide(side));
            }
        }
    }
    if (cleanRec->quantity > 0) {
        // push leftover if not fully filled
        pushOrder(cleanRec, priceIdx, side);
    } else {
        if (DEBUGMODE) printf("Filled at origin %d \n", cleanRec->order_id);
        // Add to the vector of cancel orders if the quantity is zero
        matchedOrder.push_back(cleanRec->order_id);
    }
    return matchedOrder;
}

std::vector<int32_t> dequeOrderBook::pushOrder(std::shared_ptr<order> cleanRec, std::int32_t priceIdx, Side side) {
    std::array<std::deque<std::shared_ptr<order>>, MAXTICKS> &desiredBook = (side == Side::Sell) ? askBook : bidBook;
    std::int32_t &bestPxIdx = (side == Side::Sell) ? bestAskIdx : bestBidIdx;
    // updating bestAsk/bestBid in case new order is better and not matching
    if (side == Side::Sell) {
        bestPxIdx=(priceIdx < bestPxIdx) ? priceIdx : bestPxIdx;
    } else {
        bestPxIdx=(priceIdx > bestPxIdx) ? priceIdx : bestPxIdx;
    }
    desiredBook[priceIdx].push_back(cleanRec);  
    lookUpMap.emplace(cleanRec->order_id, OrderLocation(side, priceIdx, cleanRec));
    return {};
}

void dequeOrderBook::updateNextWorstPxIdxImpl(const Side side) {
    std::int32_t &bestPxIdx = (side == Side::Sell) ? bestBidIdx : bestAskIdx;
    auto& book = (side==Side::Sell) ? bidBook : askBook;
    std::int32_t px = bestPxIdx;
    while (px >= 0 && px < MAXTICKS && book[px].empty()) {
        px += (side==Side::Sell ? -1 : 1);
    }
    bestPxIdx = px;
}

std::vector<int32_t> dequeOrderBook::matchAtPriceLevel(std::deque<std::shared_ptr<order>> &level, std::shared_ptr<order> &cleanRec) {
    std::vector<int32_t> matchedId;
    while (!level.empty() && cleanRec->quantity > 0) {
        std::shared_ptr<order> &matchingOrder = level.front();
        std::int32_t trades = std::min(cleanRec->quantity, matchingOrder->quantity);
        matchingOrder->quantity -= trades;
        cleanRec->quantity -= trades;


        if (matchingOrder->quantity == 0) {
            if (DEBUGMODE) printf("Matched Order %d \n", matchingOrder->order_id);
            // Saving element into matchedId to pass cancel order to the client
            matchedId.push_back(matchingOrder->order_id);
            lookUpMap.erase(matchingOrder->order_id);
            // Delete the matched order with q = 0 from the level
            level.pop_front();
        }
    }
    return matchedId;
}

void dequeOrderBook::showBookImpl() {
    // iterate over each price of the order book
    using BookLevel = std::array<std::deque<std::shared_ptr<order>>, MAXTICKS>;
    // Because i am making a array of references, I can't put them in a container
    // So, when references aren't enough, I use pointers which difficult the verbosity but are more flexibles
    std::array<BookLevel*, 2> orderBook = {&askBook, &bidBook};
    std::array<std::string_view, 2> bookName = {"Ask Book: ", "Bid Book"};
    for (std::int32_t j=0; j<orderBook.size(); ++j) {
        std::cout << bookName[j] << std::endl;
        BookLevel& book = *orderBook[j];
        for (std::int32_t i=0; i<book.size(); ++i) {
            double price = (i*TICKSIZE) + MINPRICE;
            const std::deque<std::shared_ptr<order> >&ordersAtPrice= book[i]; // The vector of orders at this price
            if (ordersAtPrice.empty()) {
                continue;
            }
            // Print the price level
            std::cout << "Price: " << price << std::endl;
            // iterate over each side (buy and sell) in a determined price
            for (std::int32_t k=0; k<std::min<std::size_t>(2, ordersAtPrice.size()); ++k) {
                const auto& o = ordersAtPrice[k];
                std::time_t orderTime = std::chrono::system_clock::to_time_t(o->timestamp);
                std::tm tm_order = *std::localtime(&orderTime);

                // show the relevant values
                std::cout << "  Order ID: " << o->order_id
                        << ", Quantity: " << o->quantity
                        << ", timestamp: " << std::put_time(&tm_order, "%Y-%m-%d %H:%M:%S")  << std::endl;
            }
            
        }   
    }
    std::cout << "Best Ask: " << (bestAskIdx*TICKSIZE) + MINPRICE << std::endl;
    std::cout << "Best Bid: " << (bestBidIdx*TICKSIZE) + MINPRICE << std::endl;
}

void dequeOrderBook::showLookUpMapImpl () {
    int count = 0;
    for (const auto& entry : lookUpMap) {
        std::cout << "Order ID: " << entry.first << std::endl;
        std::cout << "Side: (Is Sell?) " << static_cast<bool>(entry.second.side) << std::endl;
        std::cout << "Price Index: " << (entry.second.price_index * TICKSIZE) + MINPRICE << std::endl;
        std::cout << "--------------------------" << std::endl;

        if (++count >= 10) break;  // stop after 10 entries
    }
}
