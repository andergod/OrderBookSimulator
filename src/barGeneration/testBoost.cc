#include "orderBookTrack.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>
#include <parquet/arrow/writer.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <ctime>
#include <cstdio>

using namespace std::chrono;
namespace beast     = boost::beast;
namespace http      = beast::http;
namespace websocket = beast::websocket;
namespace net       = boost::asio;
namespace ssl       = net::ssl;
using tcp           = net::ip::tcp;
using json          = nlohmann::json;

void load_root_certificates(ssl::context& ctx)
{
  ctx.set_default_verify_paths();
}

int main()
{
  const std::string host   = "stream.data.alpaca.markets";
  const std::string port   = "443";
  const std::string target = "/v1beta3/crypto/us";

  const char* raw_key = std::getenv("API_KEY");
  const char* raw_secret = std::getenv("API_SECRET");

  if (!raw_key || !raw_secret) throw std::runtime_error("API_KEY or API_SECRET not set");

  const std::string api_key = raw_key;  // safe copy
  const std::string api_secret = raw_secret;

  try {
    net::io_context ioc;
    ssl::context    ctx{ssl::context::tlsv12_client};
    load_root_certificates(ctx);

    tcp::resolver                                     resolver{ioc};
    websocket::stream<beast::ssl_stream<tcp::socket>> ws{ioc, ctx};

    // Resolve host
    auto const results = resolver.resolve(host, port);

    // Connect TCP
    auto ep = net::connect(beast::get_lowest_layer(ws), results);

    std::ofstream log_file("log.txt", std::ios::out);
    if (!log_file.is_open())
      throw std::runtime_error("Failed to open log file");

    std::ofstream bars_file("bars.txt", std::ios::out);
    if (!bars_file.is_open())
      throw std::runtime_error("Failed to open bars file");

    // Set SNI using only hostname (no port!)
    if (!SSL_set_tlsext_host_name(
          ws.next_layer().native_handle(), host.c_str())) {
      throw beast::system_error(
        static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
    }

    // SSL handshake
    ws.next_layer().handshake(ssl::stream_base::client);

    ws.handshake(host, target);

    // Send authentication JSON
    json auth = {{"action", "auth"}, {"key", api_key}, {"secret", api_secret}};

    ws.write(net::buffer(auth.dump()));

    // Read server response
    beast::flat_buffer buffer;
    ws.read(buffer);
    std::cout << "Auth response: " << beast::make_printable(buffer.data())
              << std::endl;
    log_file << "Auth response: " << beast::make_printable(buffer.data())
             << std::endl;

    // --- Subscribe to BTC/USD orderbook and trades ---
    json sub_msg = {{"action", "subscribe"}, {"orderbooks", {"BTC/USD"}}};

    ws.write(net::buffer(sub_msg.dump()));

    // Open output file
    std::ofstream out("src/barGeneration/alpaca_data.jsonl", std::ios::out);
    if (!out.is_open())
      throw std::runtime_error("Failed to open output file");
    buffer.consume(buffer.size());
    ws.read(buffer);
    std::cout << "Sub response: " << beast::make_printable(buffer.data())
              << std::endl;
    log_file << "Sub response: " << beast::make_printable(buffer.data())
             << std::endl;
    buffer.consume(buffer.size());
    ws.read(buffer);
    std::cout << "Subscriptions: " << beast::make_printable(buffer.data())
              << std::endl;
    log_file << "Subscriptions: " << beast::make_printable(buffer.data())
             << std::endl;
    // The correct syntax when handeling buffer is
    // ws.read(buffer) ads the latest message into the buffer
    // beast::make_printable(buffer.data()) throws the info in bits somewhere
    // buffer.consume(buffer.size()) cleans the buffer so the next read is only
    // one line if you don't do this, the next we.read(buffer) will append and
    // you'll have two messages together

    auto       start   = std::chrono::steady_clock::now();
    const auto runtime = std::chrono::seconds(30);

    orderBook ob;
    ob.firstBarCameIn();

    // Read messages for 5 seconds
    while (std::chrono::steady_clock::now() - start < runtime) {
      buffer.consume(buffer.size()); // clear previous data
      ws.read(buffer);

      // Convert buffer to string
      std::string msg = beast::buffers_to_string(buffer.data());
      // this should be a timestamp from the current bar
      auto now = std::chrono::steady_clock::now();

      try {
        json parsed = json::parse(msg);
        out << parsed.dump() << "\n";
        for (int i=0; i<parsed.size(); ++i) {
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
          if (resp["T"]=="t") {
            trade t = resp.get<trade>();
            ob.addUpdateTrades(t.price, t.side, t.size, t.time, log_file);
          }
        }
      }
      catch (json::parse_error& e) {
        std::cerr << "Invalid JSON received: " << msg << std::endl;
      }
      if ((now > ob.next_bar_close) ||
          (now == ob.next_bar_close)) {
        ob.createBar(bars_file);
        // ob.reset_state(); missing implementation for trades specially
        ob.next_bar_close += ob.bar_interval;
      }
    }

    // Close WebSocket
    ws.close(websocket::close_code::normal);
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
