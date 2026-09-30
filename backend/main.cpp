#include <iostream>
#include <string>
#include "Database.h"
#include "URLService.h"
#include "LRUCache.h"
#include "AnalyticsQueue.h"
#include "ThreadPool.h"
#include "LoadBalancer.h"
#include "Server.h"

int main(int argc, char* argv[]) {
    std::string dbPath = "shortx.db";
    std::string host = "localhost";
    int port = 8080;

    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    std::cout << "Starting ShortX Backend Services..." << std::endl;

    Database db(dbPath);
    if (!db.initialize()) {
        std::cerr << "Failed to initialize SQLite database." << std::endl;
        return 1;
    }
    std::cout << "[1/5] SQLite Database initialized successfully." << std::endl;

    URLService urlService(db);
    std::cout << "[2/5] URL Service initialized." << std::endl;

    LRUCache lruCache(100);
    std::cout << "[3/5] Custom LRU Cache initialized (Capacity: 100)." << std::endl;

    AnalyticsQueue analyticsQueue(db);
    std::cout << "[4/5] Asynchronous Analytics Queue worker initialized." << std::endl;

    LoadBalancer loadBalancer(3);
    std::cout << "[5/5] Load Balancer Simulator initialized with 3 servers." << std::endl;

    ThreadPool threadPool(4);
    std::cout << "Thread Pool running with 4 worker threads." << std::endl;

    Server server(host, port, urlService, &lruCache, &analyticsQueue, &loadBalancer);

    std::cout << "ShortX Server listening on http://" << host << ":" << port << "..." << std::endl;
    server.start();

    return 0;
}
