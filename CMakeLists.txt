cmake_minimum_required(VERSION 3.14)
project(shortx CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(backend)
add_subdirectory(tests)
add_subdirectory(benchmarks)
