#include <boost/asio/connect.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

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

  const std::string api_key    = std::getenv("API_KEY");
  const std::string api_secret = std::getenv("API_SECRET");

  try {
    net::io_context ioc;
    ssl::context    ctx{ssl::context::tlsv12_client};
    load_root_certificates(ctx);

    tcp::resolver                                     resolver{ioc};
    websocket::stream<beast::ssl_stream<tcp::socket>> ws{ioc, ctx};

    // 🔹 Resolve domain
    auto const results = resolver.resolve(host, port);

    // 🔹 Connect TCP
    net::connect(ws.next_layer().next_layer(), results.begin(), results.end());

    // 🔹 Perform SSL handshake
    ws.next_layer().handshake(ssl::stream_base::client);

    // 🔹 Perform WebSocket handshake
    ws.handshake(host, target);

    // 🔹 Send authentication JSON
    json auth = {{"action", "auth"}, {"key", api_key}, {"secret", api_secret}};

    ws.write(net::buffer(auth.dump()));

    // 🔹 Read server response
    beast::flat_buffer buffer;
    ws.read(buffer);
    std::cout << "Auth response: " << beast::make_printable(buffer.data())
              << std::endl;

    // 🔹 Close cleanly
    ws.close(websocket::close_code::normal);
  }
  catch (std::exception const& e) {
    std::cerr << "Error: " << e.what() << std::endl;
  }
}
