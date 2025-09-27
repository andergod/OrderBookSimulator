set(LIBRARY_SOURCES "common.cc" "dequeOrderBook.cc" "orderGenerator.cc" "intrusiveOrderBook.cc" "orderBook.cc")
# "pmrBook.cc" "pmrPool.cc"
set(LIBRARY_HEADERS "orderBook.h" "common.h")
set(LIBRARY_INCLUDES  "./" "${CMAKE_BINARY_DIR}/configured_files/include")

add_library(${LIBRARY_NAME} STATIC
    ${LIBRARY_SOURCES}
    ${LIBRARY_HEADERS})
target_include_directories(${LIBRARY_NAME} PUBLIC
    ${LIBRARY_INCLUDES})
