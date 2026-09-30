#include "LoadBalancer.h"
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <iostream>

LoadBalancer::LoadBalancer(int initialServerCount)
    : algorithm(LoadBalancingAlgorithm::ROUND_ROBIN), roundRobinIndex(0) {
    for (int i = 1; i <= initialServerCount; ++i) {
        servers.push_back(SimulatedServer{i, 0, 0, true});
    }
    rebuildHashRing();
}

std::string LoadBalancer::algorithmToString(LoadBalancingAlgorithm algo) const {
    switch (algo) {
        case LoadBalancingAlgorithm::ROUND_ROBIN: return "ROUND_ROBIN";
        case LoadBalancingAlgorithm::LEAST_CONNECTIONS: return "LEAST_CONNECTIONS";
        case LoadBalancingAlgorithm::CONSISTENT_HASHING: return "CONSISTENT_HASHING";
    }
    return "ROUND_ROBIN";
}

LoadBalancingAlgorithm LoadBalancer::stringToAlgorithm(const std::string& str) const {
    if (str == "LEAST_CONNECTIONS") return LoadBalancingAlgorithm::LEAST_CONNECTIONS;
    if (str == "CONSISTENT_HASHING") return LoadBalancingAlgorithm::CONSISTENT_HASHING;
    return LoadBalancingAlgorithm::ROUND_ROBIN;
}

void LoadBalancer::rebuildHashRing() {
    hashRing.clear();
    for (const auto& s : servers) {
        if (s.healthy) {
            // Add 3 virtual nodes per server for uniform distribution
            for (int v = 0; v < 3; ++v) {
                std::string vnodeKey = "server_" + std::to_string(s.id) + "#vnode_" + std::to_string(v);
                size_t hashPos = std::hash<std::string>{}(vnodeKey);
                hashRing[hashPos] = s.id;
            }
        }
    }
}

void LoadBalancer::setAlgorithm(const std::string& algoStr) {
    std::lock_guard<std::mutex> lock(lbMutex);
    algorithm = stringToAlgorithm(algoStr);
}

std::string LoadBalancer::getAlgorithm() const {
    std::lock_guard<std::mutex> lock(lbMutex);
    return algorithmToString(algorithm);
}

std::vector<SimulatedServer> LoadBalancer::getServers() const {
    std::lock_guard<std::mutex> lock(lbMutex);
    return servers;
}

SimulatedServer LoadBalancer::addServer() {
    std::lock_guard<std::mutex> lock(lbMutex);
    int newId = 1;
    for (const auto& s : servers) {
        if (s.id >= newId) {
            newId = s.id + 1;
        }
    }
    SimulatedServer newServer{newId, 0, 0, true};
    servers.push_back(newServer);
    rebuildHashRing();
    return newServer;
}

bool LoadBalancer::removeServer(int id) {
    std::lock_guard<std::mutex> lock(lbMutex);
    auto it = std::remove_if(servers.begin(), servers.end(), [id](const SimulatedServer& s) {
        return s.id == id;
    });

    if (it != servers.end()) {
        servers.erase(it, servers.end());
        rebuildHashRing();
        return true;
    }
    return false;
}

bool LoadBalancer::killServer(int id) {
    std::lock_guard<std::mutex> lock(lbMutex);
    for (auto& s : servers) {
        if (s.id == id) {
            s.healthy = false;
            s.activeConnections = 0;
            rebuildHashRing();
            return true;
        }
    }
    return false;
}

bool LoadBalancer::restoreServer(int id) {
    std::lock_guard<std::mutex> lock(lbMutex);
    for (auto& s : servers) {
        if (s.id == id) {
            s.healthy = true;
            rebuildHashRing();
            return true;
        }
    }
    return false;
}

int LoadBalancer::routeRequest(const std::string& requestKey) {
    std::lock_guard<std::mutex> lock(lbMutex);

    std::vector<size_t> healthyIndices;
    for (size_t i = 0; i < servers.size(); ++i) {
        if (servers[i].healthy) {
            healthyIndices.push_back(i);
        }
    }

    if (healthyIndices.empty()) {
        return -1; // No healthy servers available
    }

    size_t chosenIndex = 0;

    if (algorithm == LoadBalancingAlgorithm::ROUND_ROBIN) {
        chosenIndex = healthyIndices[roundRobinIndex % healthyIndices.size()];
        roundRobinIndex++;
    } else if (algorithm == LoadBalancingAlgorithm::LEAST_CONNECTIONS) {
        size_t minConnIndex = healthyIndices[0];
        int minConn = servers[minConnIndex].activeConnections;

        for (size_t idx : healthyIndices) {
            if (servers[idx].activeConnections < minConn) {
                minConn = servers[idx].activeConnections;
                minConnIndex = idx;
            }
        }
        chosenIndex = minConnIndex;
    } else if (algorithm == LoadBalancingAlgorithm::CONSISTENT_HASHING) {
        std::string keyToHash = requestKey.empty() ? ("default_req_" + std::to_string(roundRobinIndex++)) : requestKey;
        size_t hashVal = std::hash<std::string>{}(keyToHash);

        if (hashRing.empty()) {
            chosenIndex = healthyIndices[0];
        } else {
            auto ringIt = hashRing.lower_bound(hashVal);
            if (ringIt == hashRing.end()) {
                ringIt = hashRing.begin();
            }

            int targetServerId = ringIt->second;
            // Locate server index
            bool found = false;
            for (size_t idx : healthyIndices) {
                if (servers[idx].id == targetServerId) {
                    chosenIndex = idx;
                    found = true;
                    break;
                }
            }
            if (!found) {
                chosenIndex = healthyIndices[0];
            }
        }
    }

    servers[chosenIndex].totalRequests++;
    return servers[chosenIndex].id;
}

std::map<int, long long> LoadBalancer::simulateRequests(int requestCount) {
    std::map<int, long long> distribution;

    // Fast-path lock inside loop
    for (int i = 0; i < requestCount; ++i) {
        std::string reqKey = "sim_req_" + std::to_string(i * 31 + 17);
        int routedServerId = routeRequest(reqKey);
        if (routedServerId != -1) {
            distribution[routedServerId]++;
        }
    }

    return distribution;
}

void LoadBalancer::resetStats() {
    std::lock_guard<std::mutex> lock(lbMutex);
    for (auto& s : servers) {
        s.activeConnections = 0;
        s.totalRequests = 0;
    }
    roundRobinIndex = 0;
}
