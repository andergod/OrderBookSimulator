#include "config.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/version.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#define BOOST_MAJOR_VERSION (BOOST_VERSION / 100000)
#define BOOST_MINOR_VERSION ((BOOST_VERSION / 100) % 1000)
#define BOOST_PATCH_VERSION (BOOST_VERSION % 100)

int main()
{
  std::cout << "Boost version: " << BOOST_VERSION << std::endl;
  std::cout << "Boost major version: " << BOOST_MAJOR_VERSION << std::endl;
  std::cout << "Boost minor version: " << BOOST_MINOR_VERSION << std::endl;
  std::cout << "Boost patch version: " << BOOST_PATCH_VERSION << std::endl;
  std::cout << "Boost version (string): " << BOOST_LIB_VERSION << std::endl;
  return 0;
}