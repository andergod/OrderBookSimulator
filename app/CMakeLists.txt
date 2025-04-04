set(EXE_SOURCES
    "main.cc")
set(EXE_INCLUDES
    "./")

message(STATUS "Executable name: ${EXECUTABLE_NAME}")
message(STATUS "Executable sources: ${EXE_SOURCES}")

add_executable(${EXECUTABLE_NAME} ${EXE_SOURCES})
target_include_directories(${EXECUTABLE_NAME} PUBLIC
    ${EXE_INCLUDES})


target_link_libraries(
    ${EXECUTABLE_NAME}
    PUBLIC ${LIBRARY_NAME})  

if (ENABLE_WARNINGS) 
    target_set_warnings(
        ${EXECUTABLE_NAME}
        ON
        OFF
    )
endif()
