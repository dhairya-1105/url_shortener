cmake_minimum_required(VERSION 3.14)
project(shortx_benchmarks CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/backend/include
    ${CMAKE_SOURCE_DIR}/backend/third_party
)

set(BENCHMARK_SOURCES
    benchmark.cpp
    ${CMAKE_SOURCE_DIR}/backend/src/Base62Encoder.cpp
    ${CMAKE_SOURCE_DIR}/backend/src/Database.cpp
    ${CMAKE_SOURCE_DIR}/backend/src/URLService.cpp
    ${CMAKE_SOURCE_DIR}/backend/src/LRUCache.cpp
    ${CMAKE_SOURCE_DIR}/backend/src/ThreadPool.cpp
    ${CMAKE_SOURCE_DIR}/backend/src/AnalyticsQueue.cpp
    ${CMAKE_SOURCE_DIR}/backend/src/LoadBalancer.cpp
    ${CMAKE_SOURCE_DIR}/backend/third_party/sqlite3.c
)

add_executable(benchmark_suite ${BENCHMARK_SOURCES})

if(WIN32)
    target_link_libraries(benchmark_suite PRIVATE ws2_32 crypt32)
endif()

find_package(Threads REQUIRED)
target_link_libraries(benchmark_suite PRIVATE Threads::Threads)
