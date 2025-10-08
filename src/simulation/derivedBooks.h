#pragma once

#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <array>
#include <deque>
#include <unordered_map>
#include "config.hpp"
#include "orderBook.h"
#include "orderBookTemplate.h"
#include "common.h"
#include <memory_resource>
#include <optional>
#include <memory>

class dequeOrderBook : public orderBook<dequeOrderBook>
{
private:
    friend class orderBook<dequeOrderBook>;
    std::vector<int32_t> pushOrder(std::shared_ptr<order> cleanRec, std::int32_t priceIdx, Side side);
    std::vector<int32_t> matchOrder(std::shared_ptr<order> cleanRec, std::int32_t priceIdx, Side side, std::int32_t &bestPxIdx);
    std::vector<int32_t> matchAtPriceLevel(std::deque<std::shared_ptr<order>> &level, std::shared_ptr<order> &cleanRec);
    std::unordered_map<std::int32_t, OrderLocation> lookUpMap;
    std::array<std::deque<std::shared_ptr<order>>, MAXTICKS> bidBook;
    std::array<std::deque<std::shared_ptr<order>>, MAXTICKS> askBook;
    std::int32_t bestBidIdx = -1;
    std::int32_t bestAskIdx = MAXTICKS;
    void CheckLookUpMap(std::unordered_map<std::int32_t, OrderLocation> &lookUpMap);

public:
    // orderBook definition
    dequeOrderBook();
    // method for adding a limit order into the order book and match it if necessary
    std::vector<int32_t> addLimitOrderImpl(orderReceived received);
    void updateNextWorstPxIdxImpl(const Side side);
    std::vector<int32_t> modifyOrderImpl(amendOrder modOrder);
    void cancelOrderImpl(amendOrder modOrder);
    // show the contect of the book
    void showBookImpl();
    void showLookUpMapImpl();
    // vector that holds the trades
    std::vector<tradeRecord> trades;
};

class intrusiveOrderBook : public orderBook<intrusiveOrderBook>
{
private:
    // worth making it friends or should i manage and do everything need it public, make UpdateNextImpl public
    // and should I make on the interphace prive but the implementation private
    friend class orderBook<intrusiveOrderBook>;
    OrderPool orderPool;
    std::vector<int32_t> pushOrder(OrderIntrusive *cleanRec, std::int32_t priceIdx, Side side);
    std::vector<int32_t> matchOrder(OrderIntrusive *cleanRec, std::int32_t priceIdx, Side side, std::int32_t &bestPxIdx);
    void updateNextWorstPxIdxImpl(const Side side);
    std::vector<int32_t> matchAtPriceLevel(OrderList &level, OrderIntrusive *cleanRec);
    std::unordered_map<std::int32_t, OrderLocationIntrusive> lookUpMap;
    std::array<OrderList, MAXTICKS> bidBook;
    std::array<OrderList, MAXTICKS> askBook;
    std::int32_t bestBidIdx = -1;
    std::int32_t bestAskIdx = MAXTICKS;
    void CheckLookUpMap(std::unordered_map<std::int32_t, OrderLocationIntrusive> &lookUpMap);

public:
    // orderBook definition
    intrusiveOrderBook() : orderPool(1000 * MAXTICKS) {};
    // method for adding a limit order into the order book and match it if necessary
    std::vector<int32_t> addLimitOrderImpl(orderReceived received);
    std::vector<int32_t> modifyOrderImpl(amendOrder modOrder);
    void cancelOrderImpl(amendOrder modOrder);
    // show the contect of the book
    void showBookImpl();
    void showLookUpMapImpl();
    // vector that holds the trades
    std::vector<tradeRecord> trades;
};

class pmrBook : public orderBook<pmrBook>
{
private:
    // worth making it friends or should i manage and do everything need it public, make UpdateNextImpl public
    // and should I make on the interphace prive but the implementation private
    friend class orderBook<pmrBook>;
    std::vector<int32_t> pushOrder(OrderIntrusive *cleanRec, std::int32_t priceIdx, Side side);
    std::vector<int32_t> matchOrder(OrderIntrusive *cleanRec, std::int32_t priceIdx, Side side, std::int32_t &bestPxIdx);
    void updateNextWorstPxIdxImpl(const Side side);
    std::vector<int32_t> matchAtPriceLevel(OrderList &level, OrderIntrusive *cleanRec);
    std::unordered_map<std::int32_t, OrderLocationIntrusive> lookUpMap;
    pmrPool makeOrderPool(size_t capacity);
    std::array<OrderList, MAXTICKS> bidBook;
    std::array<OrderList, MAXTICKS> askBook;
    std::int32_t bestBidIdx = -1;
    std::int32_t bestAskIdx = MAXTICKS;
    pmrPool orderPool;
    void CheckLookUpMap(std::unordered_map<std::int32_t, OrderLocationIntrusive> &lookUpMap);

public:
    // orderBook definition
    pmrBook();
    // method for adding a limit order into the order book and match it if necessary
    std::vector<int32_t> addLimitOrderImpl(orderReceived received);
    std::vector<int32_t> modifyOrderImpl(amendOrder modOrder);
    void cancelOrderImpl(amendOrder modOrder);
    // show the contect of the book
    void showBookImpl();
    void showLookUpMapImpl();
    // vector that holds the trades
    std::vector<tradeRecord> trades;
};