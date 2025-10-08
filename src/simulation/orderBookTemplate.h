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
#include "common.h"
#include <memory_resource>
#include <optional>
#include <memory>

template <typename Derived>
class orderBook
{
public:
    void updateNextWorstPxIdx(const Side side)
    {
        static_cast<Derived *>(this)->updateNextWorstPxIdxImpl(side);
    }
    std::vector<int32_t> addLimitOrder(orderReceived received)
    {
        return static_cast<Derived *>(this)->addLimitOrderImpl(received);
    }
    std::vector<int32_t> recModOrders(amendOrder modOrder)
    {
        return static_cast<Derived *>(this)->modifyOrderImpl(modOrder);
    }
    void recCancelOrders(amendOrder modOrder)
    {
        static_cast<Derived *>(this)->cancelOrderImpl(modOrder);
    }
    void showBook()
    {
        static_cast<Derived *>(this)->showBookImpl();
    }
    void showLookUpMap()
    {
        static_cast<Derived *>(this)->showLookUpMapImpl();
    }

protected:
    void def_updateNextWorstPxId(const Side side)
    {
        std::int32_t &bestPxIdx = (side == Side::Sell) ? static_cast<Derived *>(this)->bestBidIdx : static_cast<Derived *>(this)->bestAskIdx;
        auto &book = (side == Side::Sell) ? static_cast<Derived *>(this)->bidBook : static_cast<Derived *>(this)->askBook;
        std::int32_t px = bestPxIdx;
        while (px >= 0 && px < MAXTICKS && book[px].empty())
        {
            px += (side == Side::Sell ? -1 : 1);
        }
        bestPxIdx = px;
    }
    template <typename OrderPtr>
    std::vector<int32_t> def_addLimitOrder(orderReceived received, OrderPtr cleanRecPt)
    {
        std::int32_t priceIdx = priceToIdx(received.price);
        std::int32_t &bestPxIdx = (received.side == Side::Sell) ? static_cast<Derived *>(this)->bestBidIdx : static_cast<Derived *>(this)->bestAskIdx;
        // If sell then make it priceIdx <= bestPxIdex
        bool condition = (received.side == Side::Sell) ? (priceIdx <= bestPxIdx) : (priceIdx >= bestPxIdx);
        if (DEBUGMODE)
        {
            printf("Adding limit order for Id %d \n", cleanRecPt->order_id);
            printf("Index Location: %d; ", priceIdx);
            printf("Side: %s; ", received.side == Side::Sell ? "Sell" : "Buy");
            printf("Quantity: %d \n", cleanRecPt->quantity);
            fflush(stdout);
        }
        return (condition) ? static_cast<Derived *>(this)->matchOrder(cleanRecPt, priceIdx, received.side, bestPxIdx) : static_cast<Derived *>(this)->pushOrder(cleanRecPt, priceIdx, received.side);
    }
    template <typename OrderLocation>
    void def_CheckLookUpMap(std::unordered_map<std::int32_t, OrderLocation> &lookUpMap)
    {
        for (auto &pair : lookUpMap)
        {
            int32_t key = pair.first;
            OrderLocation &loc = pair.second;
            if (key != loc.order_pt->order_id)
            {
                printf("Error during checking the lookUpMap is legit for order %d \n", key);
                fflush(stdout);
                printf("Stop");
            }
        }
    }
};