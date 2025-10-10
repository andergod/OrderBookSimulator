#include "config.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <cstdlib>
#include <string>
#include <iostream>
#include <boost/version.hpp>

int main()
{
    std::cout << "Boost version: " << BOOST_VERSION << std::endl;
    std::cout << "Boost major version: " << BOOST_MAJOR_VERSION << std::endl;
    std::cout << "Boost minor version: " << BOOST_MINOR_VERSION << std::endl;
    std::cout << "Boost patch version: " << BOOST_PATCH_VERSION << std::endl;
    return 0;
}