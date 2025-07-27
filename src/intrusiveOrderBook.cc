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
intrusiveOrderBook::intrusiveOrderBook() {}

std::vector<int32_t> intrusiveOrderBook::addLimitOrderImpl(orderReceived received) {
    // still is not so clear to me why is a pointer, 
    // usually a pointer is defined as 
    // BookLevel& book = *orderBook[j]; so a bit of a difference
    OrderIntrusive* cleanRecPt = orderPool.allocate(received.order_id, received.quantity, std::chrono::system_clock::now());
    std::int32_t priceIdx = priceToIdx(received.price);
    std::int32_t bestPxIdx = (received.side == Side::Sell) ? bestBidIdx : bestAskIdx;
    // If sell then make it priceIdx <= bestPxIdex
    bool condition = (received.side == Side::Sell) ? (priceIdx <= bestPxIdx) : (priceIdx >= bestPxIdx);
    if (DEBUGMODE) {
        printf("Adding limit order for Id %d \n", cleanRecPt->order_id);
        printf("Index Location: %d; ", priceIdx);
        printf("Side: %d; ", received.side);
        printf("Quantity: %d \n", cleanRecPt->quantity);
        fflush(stdout);
    }
    return (condition) ? matchOrder(cleanRecPt, priceIdx, received.side, bestPxIdx) : pushOrder(cleanRecPt, priceIdx, received.side);
}


std::vector<int32_t> intrusiveOrderBook::matchOrder(OrderIntrusive* cleanRec, std::int32_t priceIdx, Side side, std::int32_t &bestPxIdx) {
    // active orders
    std::array<OrderList, MAXTICKS> &book = (side == Side::Sell) ? bidBook : askBook;
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
        orderPool.deallocate(cleanRec);
    }
    return matchedOrder;
}

std::vector<int32_t> intrusiveOrderBook::modifyOrderImpl(amendOrder modOrder) {
    auto it = lookUpMap.find(modOrder.order_id);
    if (it == lookUpMap.end()) {
        printf("Order Not found, double check code");
        return {};
    } else {
        OrderLocationIntrusive &loc = it->second;
        std::array<OrderList, MAXTICKS> &book = (loc.side == Side::Sell) ? askBook : bidBook;
        // I will bet this will break something, as I am adding it to a deque, where needs to be copied
        OrderIntrusive oldOrder = *loc.order_pt;
        
        orderReceived newOrder = orderReceived(modOrder.price.value(), oldOrder.quantity, loc.side, std::chrono::system_clock::now(), oldOrder.order_id);
        if (DEBUGMODE) {
            printf("Order ID to Modify: %d \n", modOrder.order_id);
            printf("Old order values: order_id %d, and quantity %d \n", oldOrder.order_id, oldOrder.quantity);
            printf("Change price_index from %d to %d \n", loc.price_index, priceToIdx(modOrder.price.value()));
            fflush(stdout);
        }
        // Erase the original order from the deque and delete for LookupMap. It will be re-added later on addLimitOrder
        auto& orderList = book[loc.price_index];
        orderList.erase(loc.order_pt);
        lookUpMap.erase(modOrder.order_id);
        // Re-add as a new limit order
        if (DEBUGMODE) printf("Calling addLimitOrder for order %d\n", modOrder.order_id);
        std::vector<int32_t> canOrders = addLimitOrder(newOrder);
        if (DEBUGMODE) printf("Returned from addLimitOrder for order %d\n", modOrder.order_id);
        // Check that teh full lookUpMap is correct
        return canOrders;
    }
}

void intrusiveOrderBook::CheckLookUpMap(std::unordered_map<std::int32_t, OrderLocationIntrusive> &lookUpMap) {
    for (auto &pair: lookUpMap) {
        int32_t key = pair.first;
        OrderLocationIntrusive &loc = pair.second;
        if (key != loc.order_pt->order_id) {
            printf("Error during checking the lookUpMap is legit for order %d \n", key);
            fflush(stdout);
            printf("Stop");
        }
    }
}

void intrusiveOrderBook::cancelOrderImpl(amendOrder canOrder) {
    OrderLocationIntrusive loc = lookUpMap[canOrder.order_id];
    std::array<OrderList, MAXTICKS> &book = (loc.side == Side::Sell) ? askBook : bidBook;
    if (DEBUGMODE) printf("On the order Book: We'll cancel order ID %d \n", canOrder.order_id);
    // Erase the original order from the deque and delete for LookupMap
    auto& dq = book[loc.price_index];
    orderPool.deallocate(loc.price_index);
    dq.erase(loc.order_pt);
    lookUpMap.erase(canOrder.order_id);
}

std::vector<int32_t> intrusiveOrderBook::matchAtPriceLevel(OrderList &level, OrderIntrusive* cleanRec) {
    std::vector<int32_t> matchedId;
    while (!level.empty() && cleanRec->quantity > 0) {
        OrderIntrusive* matchingOrder = level.head;
        std::int32_t trades = std::min(cleanRec->quantity, matchingOrder->quantity);
        matchingOrder->quantity -= trades;
        cleanRec->quantity -= trades;


        if (matchingOrder->quantity == 0) {
            if (DEBUGMODE) printf("Matched Order %d \n", matchingOrder->order_id);
            // Saving element into matchedId to pass cancel order to the client
            matchedId.push_back(matchingOrder->order_id);
            lookUpMap.erase(matchingOrder->order_id);
            // Delete the matched order with q = 0 from the level
            level.erase(matchingOrder);
            orderPool.deallocate(matchingOrder);
        }
    }
    return matchedId;
}

void intrusiveOrderBook::updateNextWorstPxIdxImpl(const Side side) {
    std::int32_t &bestPxIdx = (side == Side::Sell) ? bestBidIdx : bestAskIdx;
    auto& book = (side==Side::Sell) ? bidBook : askBook;
    std::int32_t px = bestPxIdx;
    while (px >= 0 && px < MAXTICKS && book[px].empty()) {
        px += (side==Side::Sell ? -1 : 1);
    }
    bestPxIdx = px;
}

std::vector<int32_t> intrusiveOrderBook::pushOrder(OrderIntrusive* cleanRec, std::int32_t priceIdx, Side side) {
    std::array<OrderList, MAXTICKS> &desiredBook = (side == Side::Sell) ? askBook : bidBook;
    std::int32_t &bestPxIdx = (side == Side::Sell) ? bestAskIdx : bestBidIdx;
    // updating bestAsk/bestBid in case new order is better and not matching
    if (side == Side::Sell) {
        bestPxIdx=(priceIdx < bestPxIdx) ? priceIdx : bestPxIdx;
    } else {
        bestPxIdx=(priceIdx > bestPxIdx) ? priceIdx : bestPxIdx;
    }
    desiredBook[priceIdx].push_back(cleanRec);  
    lookUpMap.emplace(cleanRec->order_id, OrderLocationIntrusive(side, priceIdx, cleanRec));
    return {};
}

