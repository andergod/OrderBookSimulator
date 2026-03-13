#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <deque>
#include <iostream>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std::chrono;

constexpr double   TICKSIZE = 0.1;
constexpr double   MINPRICE = 30000;
constexpr double   MAXPRICE = 100000;
constexpr unsigned MAXTICKS = (MAXPRICE - MINPRICE) / TICKSIZE;
using json                  = nlohmann::json;

enum class Side : bool { a = true, b = false };

struct trade {
  double                                                  price;
  time_point<system_clock, duration<uint64_t, std::nano>> time;
  double                                                  size;
  std::string                                             side;
};

struct topOfBook {
  double                                                  price_bid;
  double                                                  price_ask;
  double                                                  bid_quantity;
  double                                                  ask_quantity;
  time_point<system_clock, duration<uint64_t, std::nano>> time;
  time_point<system_clock, duration<uint64_t, std::nano>> last_update_time;
};

struct bar {
  double                                                  startBid;
  double                                                  endBid;
  double                                                  highBid;
  double                                                  lowBid;
  double                                                  avgBid;
  double                                                  avgBidSize;
  double                                                  startAsk;
  double                                                  endAsk;
  double                                                  highAsk;
  double                                                  lowAsk;
  double                                                  avgAsk;
  double                                                  avgAskSize;
  std::int32_t                                            bid_quote_count;
  std::int32_t                                            ask_quote_count;
  std::int32_t                                            tradeQuantity;
  time_point<system_clock, duration<uint64_t, std::nano>> time;
};

struct curr_state {
  double   curr_start_bid;
  double   curr_end_bid;
  double   curr_high_bid;
  double   curr_low_bid;
  double   curr_sum_bid;
  double   curr_sum_bid_size;
  double   curr_start_ask;
  double   curr_end_ask;
  double   curr_high_ask;
  double   curr_low_ask;
  double   curr_sum_ask;
  double   curr_sum_ask_size;
  uint64_t cum_time;
  uint64_t curr_bid_update_count;
  uint64_t curr_ask_update_count;

  void reset()
  {
    curr_start_bid        = 0.0;
    curr_end_bid          = 0.0;
    curr_high_bid         = 0.0;
    curr_low_bid          = std::numeric_limits<double>::max();
    curr_sum_bid          = 0.0;
    curr_sum_bid_size     = 0.0;
    curr_bid_update_count = 0;
    curr_start_ask        = 0.0;
    curr_end_ask          = 0.0;
    curr_high_ask         = 0.0;
    curr_low_ask          = std::numeric_limits<double>::max();
    curr_sum_ask          = 0.0;
    curr_sum_ask_size     = 0.0;
    curr_ask_update_count = 0;
    cum_time              = 0;
  }
};

struct update {
  double       quantity;
  std::int32_t price;
};

struct message {
  std::string                                             ticket;
  std::string                                             message_type;
  std::vector<update>                                     askBook;
  std::vector<update>                                     bidBook;
  time_point<system_clock, duration<uint64_t, std::nano>> time;
};

inline time_point<system_clock, duration<uint64_t, std::nano>> parse_time(
  std::string& time_str)
{
  time_point<system_clock, duration<uint64_t, std::nano>> tp{};
  // i should make this a function because decompressing times is gonna become a
  // thing everywhere
  int y, m, d, h, min, s;
  if (
    sscanf(time_str.c_str(), "%d-%d-%dT%d:%d:%d", &y, &m, &d, &h, &min, &s)
    == 6) {
    std::tm tm = {};
    tm.tm_year = y - 1900;
    tm.tm_mon  = m - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min  = min;
    tm.tm_sec  = s;
    // timegm is a non-standard but common extension on Linux/BSD for UTC
    // parsing
    time_t tt = timegm(&tm);
    tp        = std::chrono::time_point_cast<duration<uint64_t, std::nano>>(
      std::chrono::system_clock::from_time_t(tt));

    // Parse nanoseconds if present
    size_t dot_pos = time_str.find('.');
    if (dot_pos != std::string::npos) {
      size_t z_pos = time_str.find('Z');
      if (z_pos == std::string::npos)
        z_pos = time_str.length();

      std::string ns_str = time_str.substr(dot_pos + 1, z_pos - dot_pos - 1);
      // Normalize to 9 digits
      if (ns_str.length() > 9)
        ns_str = ns_str.substr(0, 9);
      else
        while (ns_str.length() < 9)
          ns_str += '0';

      uint64_t ns = std::stoull(ns_str);
      tp += std::chrono::duration<uint64_t, std::nano>(ns);
    }
  }
  else {
    std::cerr << "Invalid time string: " << time_str << std::endl;
  }
  return tp;
}

inline void from_json(const json& j, update& u)
{
  j.at("p").get_to(u.price);
  j.at("s").get_to(u.quantity);
};

inline void from_json(const json& j, message& m)
{
  j.at("S").get_to(m.ticket);
  j.at("T").get_to(m.message_type);
  j.at("a").get_to(m.askBook);
  j.at("b").get_to(m.bidBook);
  std::string time_str;
  j.at("t").get_to(time_str);
  m.time = parse_time(time_str);
};

inline void from_json(const json& j, trade& t)
{
  j.at("p").get_to(t.price);
  std::string time_str;
  j.at("t").get_to(time_str);
  // Parse timestamp string "2026-02-14T13:53:27.738353203Z"
  t.time = parse_time(time_str);
  j.at("s").get_to(t.size);
  j.at("tks").get_to(t.side);
};

class orderBook {
private:
  std::vector<double> bidBook;
  std::vector<double> askBook;
  std::vector<trade>  trades;
  topOfBook           latestToB;
  std::int32_t        maxBidIdx = -1;
  std::int32_t        minAskIdx = static_cast<std::int32_t>(MAXTICKS);
  curr_state          current_state;
  std::int32_t        priceToIdx(double price)
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
    latestToB.last_update_time = latestToB.time;
    latestToB.time             = time;
  };
  void barUpdate()
  {
    auto delta_time = (latestToB.time - latestToB.last_update_time).count();
    if (latestToB.price_bid > 0 || latestToB.price_ask > 0) {
      current_state.cum_time += delta_time;
    }

    if (latestToB.price_bid > 0) {
      if (current_state.curr_start_bid == 0.0) { // First bid update for this
                                                 // bar
        current_state.curr_start_bid = latestToB.price_bid;
      }
      if (latestToB.price_bid < current_state.curr_low_bid)
        current_state.curr_low_bid = latestToB.price_bid;
      if (latestToB.price_bid > current_state.curr_high_bid)
        current_state.curr_high_bid = latestToB.price_bid;
      current_state.curr_end_bid = latestToB.price_bid;
      // wrong microstructure, priceBid is valid from now, so this should be
      // using the last price but its more complicated logic that the aim here
      current_state.curr_sum_bid += latestToB.price_bid * delta_time;
      current_state.curr_sum_bid_size += latestToB.bid_quantity * delta_time;
      current_state.curr_bid_update_count++;
    }

    if (latestToB.price_ask > 0) {
      if (current_state.curr_start_ask == 0.0) { // First ask update for this
                                                 // bar
        current_state.curr_start_ask = latestToB.price_ask;
      }
      if (latestToB.price_ask < current_state.curr_low_ask)
        current_state.curr_low_ask = latestToB.price_ask;
      if (latestToB.price_ask > current_state.curr_high_ask)
        current_state.curr_high_ask = latestToB.price_ask;
      current_state.curr_end_ask = latestToB.price_ask;
      current_state.curr_sum_ask += latestToB.price_ask * delta_time;
      current_state.curr_sum_ask_size += latestToB.ask_quantity * delta_time;
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
  void createBar(std::ostream& os = std::cout)
  {
    bar newBar{}; // zero initialize

    if (current_state.curr_bid_update_count > 0) {
      newBar.startBid = current_state.curr_start_bid;
      newBar.endBid   = current_state.curr_end_bid;
      newBar.highBid  = current_state.curr_high_bid;
      newBar.lowBid   = current_state.curr_low_bid;
      // need to fix this avgBid can't be obs weighted, need to be time weighted
      newBar.avgBid = current_state.curr_sum_bid / current_state.cum_time;
      newBar.avgBidSize
        = current_state.curr_sum_bid_size / current_state.cum_time;
      newBar.ask_quote_count = current_state.curr_ask_update_count;
    }

    if (current_state.curr_ask_update_count > 0) {
      newBar.startAsk = current_state.curr_start_ask;
      newBar.endAsk   = current_state.curr_end_ask;
      newBar.highAsk  = current_state.curr_high_ask;
      newBar.lowAsk   = current_state.curr_low_ask;
      // need to fix this avgBid can't be obs weighted, need to be time weighted
      newBar.avgAsk = current_state.curr_sum_ask / current_state.cum_time;
      newBar.avgAskSize
        = current_state.curr_sum_ask_size / current_state.cum_time;
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
    // Updating times so the start of ToB goes from now
    updateToB(now);
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
