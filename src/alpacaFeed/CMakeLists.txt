set(LIBRARY_INCLUDES  "./" "${CMAKE_BINARY_DIR}/configured_files/include")
add_executable(read_json_app readJson.cc)
target_include_directories(read_json_app PUBLIC
    ${LIBRARY_INCLUDES})
target_link_libraries(read_json_app PRIVATE nlohmann_json::nlohmann_json)
