#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <memory>
#include "httplib.h"
#include "URLService.h"

// Forward declaration for LRUCache & LoadBalancer & AnalyticsQueue (for subsequent phases)
class LRUCache;
class AnalyticsQueue;
class LoadBalancer;

class Server {
private:
    std::string host;
    int port;
    httplib::Server httpServer;
    URLService& urlService;
    LRUCache* lruCache;             // nullptr if cache not enabled
    AnalyticsQueue* analyticsQueue; // nullptr if analytics queue not enabled
    LoadBalancer* loadBalancer;     // nullptr if load balancer not enabled

    void setupRoutes();
    void setCorsHeaders(httplib::Response& res);

public:
    Server(const std::string& host, int port, URLService& urlService,
           LRUCache* cache = nullptr, AnalyticsQueue* analytics = nullptr, LoadBalancer* lb = nullptr);
    ~Server();

    void start();
    void stop();
};

#endif // SERVER_H
