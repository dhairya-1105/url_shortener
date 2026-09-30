#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>

enum class LoadBalancingAlgorithm {
    ROUND_ROBIN,
    LEAST_CONNECTIONS,
    CONSISTENT_HASHING
};

struct SimulatedServer {
    int id;
    int activeConnections;
    long long totalRequests;
    bool healthy;
};

class LoadBalancer {
private:
    std::vector<SimulatedServer> servers;
    LoadBalancingAlgorithm algorithm;
    size_t roundRobinIndex;
    std::map<size_t, int> hashRing; // hash position -> server ID
    mutable std::mutex lbMutex;

    void rebuildHashRing();
    std::string algorithmToString(LoadBalancingAlgorithm algo) const;
    LoadBalancingAlgorithm stringToAlgorithm(const std::string& str) const;

public:
    explicit LoadBalancer(int initialServerCount = 3);

    // Algorithm selection
    void setAlgorithm(const std::string& algoStr);
    std::string getAlgorithm() const;

    // Server lifecycle management
    std::vector<SimulatedServer> getServers() const;
    SimulatedServer addServer();
    bool removeServer(int id);
    bool killServer(int id);
    bool restoreServer(int id);

    // Request routing & simulation
    int routeRequest(const std::string& requestKey = "");
    std::map<int, long long> simulateRequests(int requestCount);

    void resetStats();
};

#endif // LOAD_BALANCER_H
