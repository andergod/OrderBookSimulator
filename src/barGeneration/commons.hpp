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
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std::chrono;
using json      = nlohmann::json;
using TimePoint = std::chrono::time_point<
  std::chrono::system_clock,
  std::chrono::duration<uint64_t, std::nano>>;

enum class Side : bool { a = true, b = false };

struct trade {
  double                                                  price;
  time_point<system_clock, duration<uint64_t, std::nano>> time;
  double                                                  size;
  std::string                                             side;
};

struct medianHeap {
  std::priority_queue<double> maxHeap; // Lower half
  std::priority_queue<double, std::vector<double>, std::greater<double>>
    minHeap; // Upper half

  void push(double price)
  {
    if (maxHeap.empty() || price <= maxHeap.top()) {
      maxHeap.push(price);
    }
    else {
      minHeap.push(price);
    }

    // Balance heaps
    if (maxHeap.size() > minHeap.size() + 1) {
      minHeap.push(maxHeap.top());
      maxHeap.pop();
    }
    else if (minHeap.size() > maxHeap.size()) {
      maxHeap.push(minHeap.top());
      minHeap.pop();
    }
  }

  double getMedian() const
  {
    if (maxHeap.empty())
      return 0.0;
    if (maxHeap.size() == minHeap.size()) {
      return (maxHeap.top() + minHeap.top()) / 2.0;
    }
    return maxHeap.top();
  }

  void reset()
  {
    maxHeap = std::priority_queue<double>();
    minHeap = std::
      priority_queue<double, std::vector<double>, std::greater<double>>();
  }
};

struct topOfBook {
  double                                                  price_bid;
  double                                                  price_ask;
  double                                                  bid_quantity;
  double                                                  ask_quantity;
  time_point<system_clock, duration<uint64_t, std::nano>> time;
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

inline TimePoint now_timepoint()
{
  auto tp = std::chrono::system_clock::now();
  auto ns
    = std::chrono::duration_cast<std::chrono::duration<uint64_t, std::nano>>(
      tp.time_since_epoch());
  return TimePoint(ns);
}

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
