add_executable(testBoost testBoost.cc orderBookTrack.hpp)
target_link_libraries(testBoost PRIVATE Boost::beast OpenSSL::SSL nlohmann_json::nlohmann_json
                                        OpenSSL::Crypto)

target_link_libraries(testBoost PRIVATE "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Arrow::arrow_static,Arrow::arrow_shared>")
target_link_libraries(testBoost PRIVATE "$<IF:$<BOOL:${ARROW_BUILD_STATIC}>,Parquet::parquet_static,Parquet::parquet_shared>")