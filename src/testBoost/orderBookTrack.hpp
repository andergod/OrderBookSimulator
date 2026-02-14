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

constexpr double   TICKSIZE = 0.1;
constexpr double   MINPRICE = 30000;
constexpr double   MAXPRICE = 100000;
constexpr unsigned MAXTICKS = (MAXPRICE - MINPRICE) / TICKSIZE;
using json                  = nlohmann::json;

enum class Side : bool { a = true, b = false };

struct update {
  std::int32_t quantity;
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

class orderBook {
private:
  std::vector<double> bidBook;
  std::vector<double> askBook;
  std::int32_t        priceToIdx(double price)
  {
    return static_cast<std::int32_t>((price - MINPRICE) / TICKSIZE);
  }

public:
  // orderBook definition
  orderBook() : bidBook(MAXTICKS, 0.0), askBook(MAXTICKS, 0.0) {};
  void addUpdate(
    std::int32_t price, Side side, std::int32_t quantity, std::ostream& os = std::cout)
  {
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
  void showBook(std::ostream& os = std::cout)
  {
    os << "bidBook: ";
    for (int i = 0; i < MAXTICKS; i++) {
      if (bidBook[i] == 0)
        continue;
      os << "Price " << priceToIdx(MINPRICE + i * TICKSIZE) << ": ";
      os << bidBook[i] << " ";
      os << std::endl;
    }
    os << std::endl;
    os << "askBook: ";
    for (int i = 0; i < MAXTICKS; i++) {
      if (askBook[i] == 0)
        continue;
      os << "Price " << priceToIdx(MINPRICE + i * TICKSIZE) << ": ";
      os << askBook[i] << " ";
      os << std::endl;
    }
    os << std::endl;
  };
};
