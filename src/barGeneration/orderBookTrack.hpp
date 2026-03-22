#pragma once
#include "commons.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <deque>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std::chrono;

constexpr double   TICKSIZE = 0.1;
constexpr double   MINPRICE = 30000;
constexpr double   MAXPRICE = 100000;
constexpr unsigned MAXTICKS = (MAXPRICE - MINPRICE) / TICKSIZE;

class orderBook {
private:
  std::vector<double> bidBook;
  std::vector<double> askBook;
  std::vector<trade>  trades;
  topOfBook           latestToB;
  std::int32_t        maxBidIdx = -1;
  std::int32_t        minAskIdx = static_cast<std::int32_t>(MAXTICKS);
  curr_state          current_state;
  medianHeap          avgBidSizeHeap;
  medianHeap          avgAskSizeHeap;
  medianHeap          avgBidPriceHeap;
  medianHeap          avgAskPriceHeap;

  std::int32_t priceToIdx(double price)
  {
    return static_cast<std::int32_t>((price - MINPRICE) / TICKSIZE);
  }

public:
  // orderBook definition
  orderBook() : bidBook(MAXTICKS, 0.0), askBook(MAXTICKS, 0.0) {};
  std::chrono::steady_clock::time_point next_bar_close;
  std::chrono::seconds                  bar_interval{1};

  void firstBarCameIn()
  {
    auto now = std::chrono::steady_clock::now();
    if (next_bar_close.time_since_epoch().count() == 0)
      next_bar_close = now + bar_interval;
  };
  void updateToB(time_point<system_clock, duration<uint64_t, std::nano>> time)
  {
    if (maxBidIdx >= 0) {
      latestToB.price_bid    = MINPRICE + maxBidIdx * TICKSIZE;
      latestToB.bid_quantity = bidBook[maxBidIdx];
    }
    else {
      latestToB.price_bid    = 0.0;
      latestToB.bid_quantity = 0.0;
    }

    if (minAskIdx < static_cast<std::int32_t>(MAXTICKS)) {
      latestToB.price_ask    = MINPRICE + minAskIdx * TICKSIZE;
      latestToB.ask_quantity = askBook[minAskIdx];
    }
    else {
      latestToB.price_ask    = 0.0;
      latestToB.ask_quantity = 0.0;
    }
    latestToB.time = time;
  };
  void barUpdate()
  {
    if (latestToB.price_bid > 0) {
      if (current_state.curr_start_bid == 0.0) { // First bid update for this
        current_state.curr_start_bid = latestToB.price_bid;
      }
      if (latestToB.price_bid < current_state.curr_low_bid)
        current_state.curr_low_bid = latestToB.price_bid;
      if (latestToB.price_bid > current_state.curr_high_bid)
        current_state.curr_high_bid = latestToB.price_bid;
      current_state.curr_end_bid = latestToB.price_bid;
      avgBidPriceHeap.push(latestToB.price_bid);
      avgBidSizeHeap.push(latestToB.bid_quantity);
      current_state.curr_bid_update_count++;
    }

    if (latestToB.price_ask > 0) {
      if (current_state.curr_start_ask == 0.0) { // First ask update for this
        current_state.curr_start_ask = latestToB.price_ask;
      }
      if (latestToB.price_ask < current_state.curr_low_ask)
        current_state.curr_low_ask = latestToB.price_ask;
      if (latestToB.price_ask > current_state.curr_high_ask)
        current_state.curr_high_ask = latestToB.price_ask;
      current_state.curr_end_ask = latestToB.price_ask;
      avgAskPriceHeap.push(latestToB.price_ask);
      avgAskSizeHeap.push(latestToB.ask_quantity);
      current_state.curr_ask_update_count++;
    }
  };
  void receiveQuoteUpdate(
    std::int32_t                                            price,
    Side                                                    side,
    double                                                  quantity,
    time_point<system_clock, duration<uint64_t, std::nano>> time,
    std::ostream&                                           os = std::cout)
  {
    addUpdateBook(price, side, quantity, os);
    updateToB(time);
    barUpdate();
  }
  void addUpdateBook(
    std::int32_t  price,
    Side          side,
    double        quantity,
    std::ostream& os = std::cout)
  {
    if (price < MINPRICE || price > MAXPRICE) {
      os << "Invalid price: " << price << std::endl;
      return;
    }
    std::int32_t priceIdx = priceToIdx(price);
    if (side == Side::b) {
      bidBook[priceIdx] = quantity;
      if (quantity > 0) {
        if (priceIdx > maxBidIdx)
          maxBidIdx = priceIdx;
      }
      else if (priceIdx == maxBidIdx) {
        while (maxBidIdx >= 0 && bidBook[maxBidIdx] <= 0) {
          maxBidIdx--;
        }
      }
    }
    else {
      askBook[priceIdx] = quantity;
      if (quantity > 0) {
        if (priceIdx < minAskIdx)
          minAskIdx = priceIdx;
      }
      else if (priceIdx == minAskIdx) {
        while (minAskIdx < static_cast<std::int32_t>(MAXTICKS)
               && askBook[minAskIdx] <= 0) {
          minAskIdx++;
        }
      }
    }
  };
  bar createBar(std::ostream& os = std::cout)
  {
    bar newBar{}; // zero initialize

    if (
      current_state.curr_bid_update_count > 0
      || current_state.curr_ask_update_count > 0) {
      newBar.time = now_timepoint();
    }

    if (current_state.curr_bid_update_count > 0) {
      newBar.startBid        = current_state.curr_start_bid;
      newBar.endBid          = current_state.curr_end_bid;
      newBar.highBid         = current_state.curr_high_bid;
      newBar.lowBid          = current_state.curr_low_bid;
      newBar.avgBid          = avgBidPriceHeap.getMedian();
      newBar.avgBidSize      = avgBidSizeHeap.getMedian();
      newBar.ask_quote_count = current_state.curr_ask_update_count;
    }

    if (current_state.curr_ask_update_count > 0) {
      newBar.startAsk        = current_state.curr_start_ask;
      newBar.endAsk          = current_state.curr_end_ask;
      newBar.highAsk         = current_state.curr_high_ask;
      newBar.lowAsk          = current_state.curr_low_ask;
      newBar.avgAsk          = avgAskPriceHeap.getMedian();
      newBar.avgAskSize      = avgAskSizeHeap.getMedian();
      newBar.bid_quote_count = current_state.curr_bid_update_count;
    }

    auto        now = std::chrono::system_clock::now();
    std::time_t t   = std::chrono::system_clock::to_time_t(now);

    os << "Bar at time: " << std::put_time(std::localtime(&t), "%F %T")
       << ", startBid:" << newBar.startBid << ", endBid:" << newBar.endBid
       << ", highBid:" << newBar.highBid << ", lowBid:" << newBar.lowBid
       << ", avgBid:" << newBar.avgBid << ", avgBidSize:" << newBar.avgBidSize
       << ", startAsk:" << newBar.startAsk << ", endAsk:" << newBar.endAsk
       << ", highAsk:" << newBar.highAsk << ", lowAsk:" << newBar.lowAsk
       << ", avgAsk:" << newBar.avgAsk << ", avgAskSize:" << newBar.avgAskSize
       << ", bid_quote_count:" << newBar.bid_quote_count
       << ", ask_quote_count:" << newBar.ask_quote_count << std::endl;

    // Reset for next bar
    current_state.reset();
    avgBidPriceHeap.reset();
    avgBidSizeHeap.reset();
    avgAskPriceHeap.reset();
    avgAskSizeHeap.reset();
    // Updating times so the start of ToB goes from now
    updateToB(now);
    return newBar;
  };
  void addUpdateTrades(
    double                                                  price,
    std::string                                             side,
    double                                                  size,
    time_point<system_clock, duration<uint64_t, std::nano>> time,
    std::ostream&                                           os = std::cout)
  {
    os << "Trade at :" << price;
    trades.push_back({price, time, size, side});
  };
  void showBook(std::ostream& os = std::cout)
  {
    os << "bidBook: ";
    for (int i = 0; i < MAXTICKS; i++) {
      if (bidBook[i] == 0)
        continue;
      os << "Price " << MINPRICE + i * TICKSIZE << ": ";
      os << bidBook[i] << " ";
      os << std::endl;
    }
    os << std::endl;
    os << "askBook: ";
    for (int i = 0; i < MAXTICKS; i++) {
      if (askBook[i] == 0)
        continue;
      os << "Price " << MINPRICE + i * TICKSIZE << ": ";
      os << askBook[i] << " ";
      os << std::endl;
    }
    os << std::endl;
  };
  void showTrades(std::ostream& os = std::cout)
  {
    for (auto t : trades) {
      auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                  t.time.time_since_epoch())
                  .count();
      os << "Trade: " << t.price << " " << t.size << " " << ns << " " << t.side
         << '\n';
    }
  }
};
