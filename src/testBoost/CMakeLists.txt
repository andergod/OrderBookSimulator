set(LIBRARY_INCLUDES "./" "${CMAKE_BINARY_DIR}/configured_files/include")
add_executable(testBoost testBoost.cc root_certificates.hpp)
target_include_directories(testBoost PUBLIC ${LIBRARY_INCLUDES})
target_link_libraries(testBoost PRIVATE Boost::beast OpenSSL::SSL
                                        OpenSSL::Crypto)
