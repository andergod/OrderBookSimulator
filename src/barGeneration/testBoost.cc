#include "networking.hpp"
#include "orderBookTrack.hpp"

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <string>

using namespace std::chrono;
namespace beast     = boost::beast;
namespace http      = beast::http;
namespace websocket = beast::websocket;
namespace net       = boost::asio;
namespace ssl       = net::ssl;
using tcp           = net::ip::tcp;
using json          = nlohmann::json;

int main()
{
  const std::string host   = "stream.data.alpaca.markets";
  const std::string port   = "443";
  const std::string target = "/v1beta3/crypto/us";

  const char* raw_key    = std::getenv("API_KEY");
  const char* raw_secret = std::getenv("API_SECRET");

  const std::string api_key    = raw_key; // safe copy
  const std::string api_secret = raw_secret;

  if (!raw_key || !raw_secret)
    throw std::runtime_error("API_KEY or API_SECRET not set");

  try {
    std::ofstream log_file("log.txt", std::ios::out);
    if (!log_file.is_open())
      throw std::runtime_error("Failed to open log file");

    std::ofstream bars_file("bars.txt", std::ios::out);
    if (!bars_file.is_open())
      throw std::runtime_error("Failed to open bars file");

    // Set SNI using only hostname (no port!)
    // We're not using this anymore not sure why
    // if (!SSL_set_tlsext_host_name(
    //       ws.next_layer().native_handle(), host.c_str())) {
    //   throw beast::system_error(
    //     static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
    // }

    WebsocketClient client(host, port, target);
    client.connect();
    // Send authentication JSON
    json auth = {{"action", "auth"}, {"key", api_key}, {"secret", api_secret}};

    client.send(auth);

    // Read server response
    std::string response = client.readMessage();
    std::cout << "Auth response: " << response << std::endl;
    log_file << "Auth response: " << response << std::endl;

    // --- Subscribe to BTC/USD orderbook and trades ---
    json sub_msg = {{"action", "subscribe"}, {"orderbooks", {"BTC/USD"}}};

    client.send(sub_msg);

    // Open output file
    std::ofstream out("src/barGeneration/alpaca_data.jsonl", std::ios::out);
    if (!out.is_open())
      throw std::runtime_error("Failed to open output file");
    response = client.readMessage();
    std::cout << "Sub response: " << response << std::endl;
    log_file << "Sub response: " << response << std::endl;
    response = client.readMessage();
    std::cout << "Subscriptions: " << response << std::endl;
    log_file << "Subscriptions: " << response << std::endl;

    auto       start   = std::chrono::steady_clock::now();
    const auto runtime = std::chrono::seconds(5);

    orderBook ob;
    ob.firstBarCameIn();

    // Read messages for 5 seconds
    while (std::chrono::steady_clock::now() - start < runtime) {
      // message from the buffer
      std::string msg = client.readMessage();
      // this should be a timestamp from the current bar
      auto now = std::chrono::steady_clock::now();

      try {
        json parsed = json::parse(msg);
        out << parsed.dump() << "\n";
        for (int i = 0; i < parsed.size(); ++i) {
          auto resp = parsed[i];
          if (resp["T"] == "o") {
            message m = resp.get<message>();
            for (auto u : m.askBook) {
              ob.addUpdateBook(u.price, Side::a, u.quantity, log_file);
            }
            for (auto u : m.bidBook) {
              ob.addUpdateBook(u.price, Side::b, u.quantity, log_file);
            }
          }
          if (resp["T"] == "t") {
            trade t = resp.get<trade>();
            ob.addUpdateTrades(t.price, t.side, t.size, t.time, log_file);
          }
        }
      }
      catch (json::parse_error& e) {
        std::cerr << "Invalid JSON received: " << msg << std::endl;
      }
      if ((now > ob.next_bar_close) || (now == ob.next_bar_close)) {
        ob.createBar(bars_file);
        // ob.reset_state(); missing implementation for trades specially
        ob.next_bar_close += ob.bar_interval;
      }
    }

    // Close WebSocket
    client.close();
    out.close();
    std::cout << "Data collection complete, saved to alpaca_data.jsonl\n";
    log_file << "Data collection complete, saved to alpaca_data.jsonl\n";
    ob.showBook(log_file);
    ob.showTrades(log_file);
    log_file.close();
    bars_file.close();
  }
  catch (std::exception const& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  return 0;
}
