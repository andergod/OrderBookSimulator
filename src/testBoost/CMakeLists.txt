add_executable(testBoost testBoost.cc orderBookTrack.hpp)
target_link_libraries(testBoost PRIVATE Boost::beast OpenSSL::SSL nlohmann_json::nlohmann_json
                                        OpenSSL::Crypto)
