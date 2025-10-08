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
#include <optional>
#include <memory>

double generateRandomPrice(double min, double max);

std::int32_t generateRandomInt(std::int32_t min, std::int32_t max);

Side generateSide();

std::int32_t priceToIdx(const double price);

Side oppositeSide(const Side side);