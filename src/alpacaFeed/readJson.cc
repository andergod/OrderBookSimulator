#include <iostream>
#include "config.hpp"
#include <cstdint>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main()
{   
    std::ifstream file("data/alpaca_sample.jsonl");
    std::string line;

    while (std::getline(file, line)) {
        try {
            json j = json::parse(line);
            std::cout << j.dump(2) << std::endl;
        } catch (json::parse_error& e) {
            std::cerr << "Parse error: " << e.what() << std::endl;
        }
    }
}