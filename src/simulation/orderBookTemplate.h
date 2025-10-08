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
    template <typename book, typename cleanRecType>
    std::vector<int32_t> def_pushOrder(book &desiredBook, cleanRecType cleanRec, std::int32_t priceIdx, Side side)
    {
        std::int32_t &bestPxIdx = (side == Side::Sell) ? static_cast<Derived *>(this)->bestAskIdx : static_cast<Derived *>(this)->bestBidIdx;
        // updating bestAsk/bestBid in case new order is better and not matching
        if (side == Side::Sell)
        {
            bestPxIdx = (priceIdx < bestPxIdx) ? priceIdx : bestPxIdx;
        }
        else
        {
            bestPxIdx = (priceIdx > bestPxIdx) ? priceIdx : bestPxIdx;
        }
        desiredBook[priceIdx].push_back(cleanRec);
        return {};
    }
    template <typename BookList>
    void def_showBookImpl(BookList orderBook)
    {
        std::array<std::string_view, 2> bookName = {"Ask Book: ", "Bid Book: "};
        for (std::int32_t j = 0; j < orderBook.size(); ++j)
        {
            std::cout << bookName[j] << std::endl;
            auto &book = *orderBook[j];
            for (std::int32_t i = 0; i < book.size(); ++i)
            {
                double price = (i * TICKSIZE) + MINPRICE;
                auto &ordersAtPrice = book[i]; // The vector of orders at this price
                if (ordersAtPrice.empty())
                {
                    continue;
                }
                // Print the price level
                std::cout << "Price: " << price << std::endl;
                // iterate over each side (buy and sell) in a determined price
                static_cast<Derived *>(this)->iterativePrint(ordersAtPrice);
            }
        }
        std::cout << "Best Ask: " << (static_cast<Derived *>(this)->bestAskIdx * TICKSIZE) + MINPRICE << std::endl;
        std::cout << "Best Bid: " << (static_cast<Derived *>(this)->bestBidIdx * TICKSIZE) + MINPRICE << std::endl;
    }
    void def_showLookUpMapImpl()
    {
        int count = 0;
        for (const auto &entry : static_cast<Derived *>(this)->lookUpMap)
        {
            std::cout << "Order ID: " << entry.first << std::endl;
            std::cout << "Side: (Is Sell?) " << static_cast<bool>(entry.second.side) << std::endl;
            std::cout << "Price Index: " << (entry.second.price_index * TICKSIZE) + MINPRICE << std::endl;
            std::cout << "--------------------------" << std::endl;

            if (++count >= 10)
                break; // stop after 10 entries
        }
    }
};