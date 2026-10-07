cmake_minimum_required(VERSION 3.22)

project(orderbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ORDERBOOK_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(ORDERBOOK_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    add_link_options(-fsanitize=address,undefined)
endif()

add_library(orderbook_core src/core/types.cpp)
add_library(orderbook::core ALIAS orderbook_core)

target_include_directories(orderbook_core PUBLIC include)

target_compile_options(orderbook_core PRIVATE -Wall -Wextra -Wpedantic)

find_package(GTest REQUIRED)
enable_testing()

add_executable(orderbook_core_tests
    tests/core/types_test.cpp
    tests/core/instrument_test.cpp
)
target_link_libraries(orderbook_core_tests PRIVATE orderbook::core GTest::gtest_main)
target_compile_options(orderbook_core_tests PRIVATE -Wall -Wextra -Wpedantic)

include(GoogleTest)
gtest_discover_tests(orderbook_core_tests)
