#include "config.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <vector>

using json = nlohmann::json;

int main()
{
  std::ifstream file(
    "/mnt/c/Users/jande/OneDrive/Code/OrderBookSimulator/data/"
    "alpaca_sample.jsonl");
  if (!file.is_open()) {
    std::cerr << "Failed to open alpaca_sample.jsonl" << std::endl;
    return 1;
  }
  std::string line;

  while (std::getline(file, line)) {
    try {
      json j = json::parse(line);
      std::cout << j.dump(2) << std::endl;
    }
    catch (json::parse_error& e) {
      std::cerr << "Parse error: " << e.what() << std::endl;
    }
  }
  return 0;
}