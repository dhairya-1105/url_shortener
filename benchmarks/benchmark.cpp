#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <iomanip>
#include "Database.h"
#include "URLService.h"
#include "LRUCache.h"
#include "ThreadPool.h"
#include "LoadBalancer.h"

void benchmarkCacheVsSQLite() {
    std::cout << "\n==================================================" << std::endl;
    std::cout << " 1. BENCHMARK: LRU Cache vs SQLite Lookup Time" << std::endl;
    std::cout << "==================================================" << std::endl;

    Database db("bench_shortx.db");
    db.initialize();
    URLService service(db);

    std::string shortCode = service.createURL("https://example.com/benchmark-target");
    LRUCache cache(100);
    cache.put(shortCode, "https://example.com/benchmark-target");

    const int iterations = 10000;

    // Measure SQLite direct lookup time
    auto startSqlite = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        URLRecord rec;
        service.getURL(shortCode, rec);
    }
    auto endSqlite = std::chrono::high_resolution_clock::now();
    double sqliteMs = std::chrono::duration<double, std::milli>(endSqlite - startSqlite).count();

    // Measure LRU Cache lookup time
    auto startCache = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        std::string val;
        cache.get(shortCode, val);
    }
    auto endCache = std::chrono::high_resolution_clock::now();
    double cacheMs = std::chrono::duration<double, std::milli>(endCache - startCache).count();

    std::cout << std::left << std::setw(25) << "Lookup Target" 
              << std::setw(15) << "Iterations" 
              << std::setw(20) << "Total Time (ms)" 
              << std::setw(20) << "Avg per op (us)" << std::endl;
    std::cout << "----------------------------------------------------------------------------------" << std::endl;
    std::cout << std::left << std::setw(25) << "SQLite Database" 
              << std::setw(15) << iterations 
              << std::setw(20) << sqliteMs 
              << std::setw(20) << (sqliteMs * 1000.0 / iterations) << std::endl;
    std::cout << std::left << std::setw(25) << "LRU Cache" 
              << std::setw(15) << iterations 
              << std::setw(20) << cacheMs 
              << std::setw(20) << (cacheMs * 1000.0 / iterations) << std::endl;
    std::cout << "Speedup factor: " << (sqliteMs / cacheMs) << "x faster with LRU Cache." << std::endl;
}

void benchmarkThreadPool() {
    std::cout << "\n==================================================" << std::endl;
    std::cout << " 2. BENCHMARK: Thread Pool Workload Scaling" << std::endl;
    std::cout << "==================================================" << std::endl;

    const int taskCount = 1000;
    std::vector<int> workerCounts = {1, 2, 4, 8};

    std::cout << std::left << std::setw(20) << "Worker Threads" 
              << std::setw(15) << "Tasks" 
              << std::setw(20) << "Execution Time (ms)" << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;

    for (int workers : workerCounts) {
        auto start = std::chrono::high_resolution_clock::now();
        {
            ThreadPool pool(workers);
            for (int i = 0; i < taskCount; ++i) {
                pool.enqueue([]() {
                    // Simulate light computation work
                    volatile int sum = 0;
                    for (int j = 0; j < 5000; ++j) {
                        sum += j;
                    }
                });
            }
            pool.shutdown();
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        std::cout << std::left << std::setw(20) << workers 
                  << std::setw(15) << taskCount 
                  << std::setw(20) << ms << std::endl;
    }
}

void benchmarkLoadBalancer() {
    std::cout << "\n==================================================" << std::endl;
    std::cout << " 3. BENCHMARK: Load Balancer Algorithm Performance" << std::endl;
    std::cout << "==================================================" << std::endl;

    const int requests = 100000;
    std::vector<std::string> algorithms = {"ROUND_ROBIN", "LEAST_CONNECTIONS", "CONSISTENT_HASHING"};

    std::cout << std::left << std::setw(25) << "Algorithm" 
              << std::setw(15) << "Requests" 
              << std::setw(20) << "Duration (ms)" 
              << std::setw(20) << "Throughput (req/s)" << std::endl;
    std::cout << "----------------------------------------------------------------------------------" << std::endl;

    for (const auto& algo : algorithms) {
        LoadBalancer lb(5);
        lb.setAlgorithm(algo);

        auto start = std::chrono::high_resolution_clock::now();
        lb.simulateRequests(requests);
        auto end = std::chrono::high_resolution_clock::now();

        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        double reqPerSec = (requests / ms) * 1000.0;

        std::cout << std::left << std::setw(25) << algo 
                  << std::setw(15) << requests 
                  << std::setw(20) << ms 
                  << std::setw(20) << static_cast<long long>(reqPerSec) << std::endl;
    }
}

int main() {
    std::cout << "ShortX Performance Benchmark Suite" << std::endl;
    std::cout << "===================================" << std::endl;

    benchmarkCacheVsSQLite();
    benchmarkThreadPool();
    benchmarkLoadBalancer();

    return 0;
}
