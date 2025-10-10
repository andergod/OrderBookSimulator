cmake_minimum_required(VERSION 3.22)

project(OrderBook VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        ON)

set(VCPKG_HOME "$ENV{VCPKG_ROOT}")
string(REPLACE "\\" "/" VCPKG_HOME "${VCPKG_HOME}")
set(CMAKE_TOOLCHAIN_FILE
    "${VCPKG_HOME}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "")

find_package(boost_beast CONFIG REQUIRED)

option(ENABLE_WARNINGS "Enable compiler warnings for a target" ON)

option(ENABLE_SANITIZE_ADDR "Enable address sanitizer" ON)
option(ENABLE_SANITIZE_UNDEF "Enable undefined sanitizer" OFF)

set(LIBRARY_NAME Library)
set(EXECUTABLE_NAME Executable)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/")
include(Docs)
include(AddGitSubmodule)

add_git_submodule(external/json)
   
if(ENABLE_WARNINGS)
    include(Warnings)
endif()

if(ENABLE_SANITIZE_ADDR OR ENABLE_SANITIZE_UNDEF)
    include(Sanitizer)
    add_sanitizer_flags(ENABLE_SANITIZE_ADDR ENABLE_SANITIZE_UNDEF)
endif()

add_subdirectory(configured)
add_subdirectory(src)
add_subdirectory(app)
add_subdirectory(external)

if(ENABLE_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()