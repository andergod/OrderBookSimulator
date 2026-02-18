#pragma once
#include <array>
#include <chrono>
#include <deque>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <ctime>
#include <cstdio>

using namespace std::chrono;

constexpr double   TICKSIZE = 0.1;
constexpr double   MINPRICE = 30000;
constexpr double   MAXPRICE = 100000;
constexpr unsigned MAXTICKS = (MAXPRICE - MINPRICE) / TICKSIZE;
using json                  = nlohmann::json;

enum class Side : bool { a = true, b = false };

struct trade {
  double price;
  time_point<system_clock, duration<uint64_t, std::nano>> time;
  double size;
  std::string side;
  
};

struct bar {
  double avgBid;
  double avgAsk;
  double avgTrade;
  std::int32_t tradeQuantity;
  std::int32_t bidQuantity;
  std::int32_t askQuantity;
};

struct update {
  double quantity;
  std::int32_t price;
};

struct message {
  std::string         ticket;
  std::string         message_type;
  std::vector<update> askBook;
  std::vector<update> bidBook;
};

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
};

inline void from_json(const json& j, trade& t)
{
  j.at("p").get_to(t.price);
  
  // Parse timestamp string "2026-02-14T13:53:27.738353203Z"
  std::string time_str;
  j.at("t").get_to(time_str);
  
  // i should make this a function because decompressing times is gonna become a thing everywhere
  int y, m, d, h, min, s;
  if (sscanf(time_str.c_str(), "%d-%d-%dT%d:%d:%d", &y, &m, &d, &h, &min, &s) == 6) {
    std::tm tm = {};
    tm.tm_year = y - 1900;
    tm.tm_mon = m - 1;
    tm.tm_mday = d;
    tm.tm_hour = h;
    tm.tm_min = min;
    tm.tm_sec = s;
    // timegm is a non-standard but common extension on Linux/BSD for UTC parsing
    time_t tt = timegm(&tm);
    t.time = std::chrono::time_point_cast<duration<uint64_t, std::nano>>(
        std::chrono::system_clock::from_time_t(tt));
    
    // Parse nanoseconds if present
    size_t dot_pos = time_str.find('.');
    if (dot_pos != std::string::npos) {
      size_t z_pos = time_str.find('Z');
      if (z_pos == std::string::npos) z_pos = time_str.length();
      
      std::string ns_str = time_str.substr(dot_pos + 1, z_pos - dot_pos - 1);
      // Normalize to 9 digits
      if (ns_str.length() > 9) ns_str = ns_str.substr(0, 9);
      else while (ns_str.length() < 9) ns_str += '0';
      
      uint64_t ns = std::stoull(ns_str);
      t.time += std::chrono::duration<uint64_t, std::nano>(ns);
    }
  }

  j.at("s").get_to(t.size);
  j.at("tks").get_to(t.side);
};

class orderBook {
private:
  std::vector<double> bidBook;
  std::vector<double> askBook;
  std::vector<trade>   trades;
  std::int32_t        priceToIdx(double price)
  {
    return static_cast<std::int32_t>((price - MINPRICE) / TICKSIZE);
  }

public:
  // orderBook definition
  orderBook() : bidBook(MAXTICKS, 0.0), askBook(MAXTICKS, 0.0) {};
  std::chrono::steady_clock::time_point next_bar_close;
  std::chrono::seconds bar_interval{1};

  void firstBarCameIn() {
    auto now = std::chrono::steady_clock::now();
    if (next_bar_close.time_since_epoch().count() == 0)
      next_bar_close = now + bar_interval;
  };
  void addUpdateBook(
    std::int32_t price, Side side, double quantity, std::ostream& os = std::cout) {
    if (price < MINPRICE || price > MAXPRICE) {
      os << "Invalid price: " << price << std::endl;
      return;
    }
    std::int32_t priceIdx = priceToIdx(price);
    if (side == Side::b)
      bidBook[priceIdx] = quantity;
    else
      askBook[priceIdx] = quantity;
  };
  void createBar(std::ostream& os = std::cout) {
    // Missing implementation for the trade size, only publishing Bid-ask spred
    double avgBid = 0.0;
    double avgAsk = 0.0;
    double avgTrade = 0.0;
    std::int32_t tradeQty = 0;
    std::int32_t bidQty = 0;
    std::int32_t askQty = 0;
    std::int32_t bidCount = 0;
    std::int32_t askCount = 0;
    std::int32_t tradeCount = 0;

    for (int i = 0; i < MAXTICKS; i++) {
      if (bidBook[i] == 0)
        continue;
      double bidPrice = static_cast<double>( MINPRICE + i * TICKSIZE);
      avgBid +=bidPrice;
      bidCount += bidBook[i];
      bidCount++;
    }
    avgBid = (bidCount > 0) ? (avgBid / bidCount) : 0.0;
      
    for (int i = 0; i < MAXTICKS; i++){
      if (askBook[i] == 0) 
        continue;
      double askPrice = static_cast<double>(MINPRICE + (i + 1) * TICKSIZE);
      avgAsk += askPrice;
      askQty += askBook[i];
      askCount++;
    }

    avgAsk = (askCount > 0) ? (avgAsk / askCount) : 0.0;

    auto now = std::chrono::steady_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      now.time_since_epoch()).count();
    os << "At time:" << ns << ", Bid:" << avgBid << ", Ask:" << avgAsk << std::endl;      
  };
  void addUpdateTrades(
    double price, std::string side, double size, time_point<system_clock, duration<uint64_t, std::nano>> time, std::ostream& os = std::cout
  ) {
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
                      t.time.time_since_epoch()).count();
       os << "Trade: "
           << t.price << " "
           << t.size  << " "
           << ns      << " "
           << t.side
           << '\n';

    }
  }
};
