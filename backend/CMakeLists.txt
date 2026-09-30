cmake_minimum_required(VERSION 3.14)
project(shortx_backend CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

set(BACKEND_SOURCES
    src/Base62Encoder.cpp
    src/Database.cpp
    src/URLService.cpp
    src/LRUCache.cpp
    src/ThreadPool.cpp
    src/AnalyticsQueue.cpp
    src/LoadBalancer.cpp
    src/Server.cpp
    third_party/sqlite3.c
)

add_executable(shortx_server main.cpp ${BACKEND_SOURCES})

if(WIN32)
    target_link_libraries(shortx_server PRIVATE ws2_32 crypt32)
endif()

find_package(Threads REQUIRED)
target_link_libraries(shortx_server PRIVATE Threads::Threads)
