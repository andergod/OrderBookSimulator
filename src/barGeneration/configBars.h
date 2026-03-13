#pragma once
#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace config {

inline constexpr std::string_view HOST   = "stream.data.alpaca.markets";
inline constexpr std::string_view PORT   = "443";
inline constexpr std::string_view TARGET = "/v1beta3/crypto/us";

inline std::string get_api_key()
{
  const char* raw = std::getenv("API_KEY");
  if (!raw)
    throw std::runtime_error("API_KEY not set");
  return raw;
}

inline std::string get_api_secret()
{
  const char* raw = std::getenv("API_SECRET");
  if (!raw)
    throw std::runtime_error("API_SECRET not set");
  return raw;
}

}