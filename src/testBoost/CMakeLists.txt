set(LIBRARY_INCLUDES  "./" "${CMAKE_BINARY_DIR}/configured_files/include")
add_executable(testBoost testBoost.cc)
target_include_directories(testBoost PUBLIC
    ${LIBRARY_INCLUDES})
target_link_libraries(server PRIVATE Boost::beast)
