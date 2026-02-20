#include <boost/asio/connect.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <cstdio>
namespace beast     = boost::beast;
namespace http      = beast::http;
namespace websocket = beast::websocket;
namespace net       = boost::asio;
namespace ssl       = net::ssl;
using tcp           = net::ip::tcp;

void load_root_certificates(ssl::context& ctx)
{
  ctx.set_default_verify_paths();
}

void connect() {
  this->ctx = ssl::context ctx{ssl::context::tlsv12_client};
  load_root_certificates(ctx);
  this->resolver = tcp::resolver                                     resolver{ioc};
  this->ws = websocket::stream<beast::ssl_stream<tcp::socket>> ws{ioc, ctx};
  results = resolver.resolve(host, port);
  auto ep = net::connect(beast::get_lowest_layer(ws), results);
	if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str())) {
		throw beast::system_error(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
		}
  // SSL handshake
  ws.next_layer().handshake(ssl::stream_base::client);
  ws.handshake(host, target);
}

std::string readMessage(
  buffer.consume(buffer.size());
  ws.read(buffer);
  return beast::buffers_to_string(buffer.data());
)

void send(const json& j) {
	ws.write(net::buffer(j.dump()));
}

void WbsocketClient(std::string h, std::string p, std::string t) {
  host = h, port  = p, target = t;
}

class WebsocketClient {
  public:
    WebsocketClient(std::string host, std::string port, std::string target);
    void connect();
    std::string readMessage();
    void send(const json& j);
  private:
    net::io_context ioc;
    ssl::context ctx;
    tcp::resolver resolver;
    websocket::stream<beast::ssl_stream<tcp::socket>> ws;
    beast::flat_buffer buffer;
    void load_root_certificates(ssl::context& ctx);
}



