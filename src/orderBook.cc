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

std::vector<int32_t> orderBook::addLimitOrder(orderReceived received) {
    order cleanRec(received.order_id, received.quantity, std::chrono::system_clock::now());
    std::int32_t priceIdx = priceToIdx(received.price);
    std::int32_t bestPxIdx = (received.side == Side::Sell) ? bestBidIdx : bestAskIdx;
    // If sell then make it priceIdx <= bestPxIdex
    bool condition = (received.side == Side::Sell) ? (priceIdx <= bestPxIdx) : (priceIdx >= bestPxIdx);
    if (DEBUGMODE) {
        printf("Adding limit order for Id %d \n", cleanRec.order_id);
        printf("Index Location: %d; ", priceIdx);
        printf("Side: %d; ", received.side);
        printf("Quantity: %d \n", cleanRec.quantity);
        fflush(stdout);
    }
    return (condition) ? matchOrder(cleanRec, priceIdx, received.side, bestPxIdx) : pushOrder(cleanRec, priceIdx, received.side);
}

std::vector<int32_t> orderBook::recModOrders(amendOrder modOrder) {
    auto it = lookUpMap.find(modOrder.order_id);
    if (it == lookUpMap.end()) {
        printf("Order Not found, double check code");
        return {};
    } else {
        OrderLocation &loc = it->second;
        std::array<std::deque<order>, MAXTICKS> &book = (loc.side == Side::Sell) ? askBook : bidBook;
        // I will bet this will break something, as I am adding it to a deque, where needs to be copied
        order oldOrder = *loc.order_it;
        
        orderReceived newOrder = orderReceived(modOrder.price.value(), oldOrder.quantity, loc.side, std::chrono::system_clock::now(), oldOrder.order_id);
        if (DEBUGMODE) {
            printf("Order ID to Modify: %d \n", modOrder.order_id);
            printf("Old order values: order_id %d, and quantity %d \n", oldOrder.order_id, oldOrder.quantity);
            printf("Change price_index from %d to %d \n", loc.price_index, priceToIdx(modOrder.price.value()));
            fflush(stdout);
        }
        CheckLookUpMap(lookUpMap);
        // Erase the original order from the deque and delete for LookupMap. It will be re-added later on addLimitOrder
        book[loc.price_index].erase(loc.order_it); 
        lookUpMap.erase(modOrder.order_id);
        // Re-add as a new limit order
        if (DEBUGMODE) printf("Calling addLimitOrder for order %d\n", modOrder.order_id);
        std::vector<int32_t> canOrders = addLimitOrder(newOrder);
        if (DEBUGMODE) printf("Returned from addLimitOrder for order %d\n", modOrder.order_id);
        // Check that teh full lookUpMap is correct
        CheckLookUpMap(lookUpMap);
        return canOrders;
    }
}

void orderBook::CheckLookUpMap(std::unordered_map<std::int32_t, OrderLocation> lookUpMap) {
    for (auto pair: lookUpMap) {
        int32_t key = pair.first;
        OrderLocation &loc = pair.second;
        if (key != loc.order_it->order_id) {
            printf("Error during checking the lookUpMap is legit for order %d \n", key);
            fflush(stdout);
            printf("Stop");
        }

    std::unordered_map<const void*, std::vector<int32_t>> pointerToKeys;
    for (const auto& [key, loc] : lookUpMap) {
        const void* ptr = static_cast<const void*>(&(*loc.order_it));
        pointerToKeys[ptr].push_back(key);
    }

    for (const auto& [ptr, keys] : pointerToKeys) {
        if (keys.size() > 1) {
            printf("💣 Multiple keys share the same iterator address: ");
            for (int id : keys) printf("%d ", id);
            fflush(stdout);
            printf("\n");
        }
    }
    }
}

void orderBook::recCancelOrders(amendOrder canOrder) {
    // TODO: return a number and then make that orderGenerator receive that and ackCancel with that ID
    CheckLookUpMap(lookUpMap);
    OrderLocation loc = lookUpMap[canOrder.order_id];
    std::array<std::deque<order>, MAXTICKS> &book = (loc.side == Side::Sell) ? askBook : bidBook;
    if (DEBUGMODE) printf("On the order Book: We'll cancel order ID %d \n", canOrder.order_id);
    // Erase the original order from the deque and delete for LookupMap
    CheckLookUpMap(lookUpMap);
    book[loc.price_index].erase(loc.order_it); 
    CheckLookUpMap(lookUpMap);
    lookUpMap.erase(canOrder.order_id);
    CheckLookUpMap(lookUpMap);
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
    if (cleanRec.quantity > 0) {
        // push leftover if not fully filled
        pushOrder(cleanRec, priceIdx, side);
    } else {
        if (DEBUGMODE) printf("Filled at origin %d \n", cleanRec.order_id);
        // Add to the vector of cancel orders if the quantity is zero
        matchedOrder.push_back(cleanRec.order_id);
    }
    CheckLookUpMap(lookUpMap);
    return matchedOrder;
}

// method for pushing a price into the book in case we don't find any match for it
std::vector<int32_t> orderBook::pushOrder(order cleanRec, std::int32_t priceIdx, Side side) {
    std::array<std::deque<order>, MAXTICKS> &desiredBook = (side == Side::Sell) ? askBook : bidBook;
    std::int32_t &bestPxIdx = (side == Side::Sell) ? bestAskIdx : bestBidIdx;
    CheckLookUpMap(lookUpMap);
    // updating bestAsk/bestBid in case new order is better and not matching
    if (side == Side::Sell) {
        bestPxIdx=(priceIdx < bestPxIdx) ? priceIdx : bestPxIdx;
    } else {
        bestPxIdx=(priceIdx > bestPxIdx) ? priceIdx : bestPxIdx;
    }
    CheckLookUpMap(lookUpMap); // pre-check
    // Check for multiple orderId
    for (const order& o : desiredBook[priceIdx]) {
        if (o.order_id == cleanRec.order_id) {
            printf("💣 Order ID %d already exists in desiredBook[%d]! Risk of aliasing!\n", cleanRec.order_id, priceIdx);
            exit(EXIT_FAILURE);
            }
    }
    desiredBook[priceIdx].push_back(cleanRec);  
    CheckLookUpMap(lookUpMap); // post-check
    auto it = std::prev(desiredBook[priceIdx].end());
    CheckLookUpMap(lookUpMap);
    auto [_, success] = lookUpMap.emplace(cleanRec.order_id, OrderLocation(side, priceIdx, it));
    // Check for duplicate orderId
    if (!success) {
        printf("❌ Attempt to insert duplicate order_id = %d into lookUpMap\n", cleanRec.order_id);
        exit(EXIT_FAILURE);
    }
    CheckLookUpMap(lookUpMap);
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
            if (DEBUGMODE) printf("Matched Order %d \n", matchingOrder.order_id);
            // Saving element into matchedId to pass cancel order to the client
            matchedId.push_back(matchingOrder.order_id);
            lookUpMap.erase(matchingOrder.order_id);
            CheckLookUpMap(lookUpMap);
            // Delete the matched order with q = 0 from the level
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

void orderGenerator::ackCancel(std::int32_t orderId) {
    auto idx = idToIdx[orderId];
    // Algorithm so you can change the elements efficiently and deletes from record
    std::swap(activeIds[idx], activeIds.back());
    idToIdx[activeIds[idx]] = idx;   // update index of swapped element
    printf("Deleting %d from Active Ids in OrderGen \n", orderId);
    fflush(stdout);
    activeIds.pop_back();
    idToIdx.erase(orderId);
}

amendOrder orderGenerator::cancelOrders() {
    std::int32_t randomId = activeIds[generateRandomInt(0, activeIds.size() - 1)];
    printf("On the gen Book: We'll cancel order ID %d \n", randomId);
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