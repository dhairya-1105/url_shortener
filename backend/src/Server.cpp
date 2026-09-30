#include "Server.h"
#include "LRUCache.h"
#include "AnalyticsQueue.h"
#include "LoadBalancer.h"
#include "json.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

using json = nlohmann::json;

static std::string getNowTimestampString() {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuffer;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tmBuffer, &nowTime);
#else
    localtime_r(&nowTime, &tmBuffer);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tmBuffer, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

static std::string getTodayDateString() {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuffer;
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tmBuffer, &nowTime);
#else
    localtime_r(&nowTime, &tmBuffer);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tmBuffer, "%Y-%m-%d");
    return ss.str();
}

Server::Server(const std::string& host, int port, URLService& urlService,
               LRUCache* cache, AnalyticsQueue* analytics, LoadBalancer* lb)
    : host(host), port(port), urlService(urlService),
      lruCache(cache), analyticsQueue(analytics), loadBalancer(lb) {
    setupRoutes();
}

Server::~Server() {
    stop();
}

void Server::setCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

void Server::setupRoutes() {
    // OPTIONS handler for CORS preflight
    httpServer.Options(".*", [this](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        res.status = 204;
    });

    // POST /api/urls - Create short URL
    httpServer.Post("/api/urls", [this](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        try {
            auto body = json::parse(req.body);
            std::string url = body.value("url", "");
            std::string customAlias = body.value("customAlias", "");

            std::string shortCode = urlService.createURL(url, customAlias);
            std::string shortUrl = "http://" + host + ":" + std::to_string(port) + "/" + shortCode;

            json responseJson = {
                {"shortCode", shortCode},
                {"shortUrl", shortUrl}
            };

            res.status = 201;
            res.set_content(responseJson.dump(), "application/json");
        } catch (const std::invalid_argument& e) {
            json errorJson = {{"error", e.what()}};
            res.status = 400;
            res.set_content(errorJson.dump(), "application/json");
        } catch (const std::runtime_error& e) {
            std::string errStr = e.what();
            if (errStr == "ALIAS_EXISTS") {
                json errorJson = {{"error", "Custom alias already exists"}};
                res.status = 409;
                res.set_content(errorJson.dump(), "application/json");
            } else {
                json errorJson = {{"error", errStr}};
                res.status = 500;
                res.set_content(errorJson.dump(), "application/json");
            }
        } catch (const std::exception&) {
            json errorJson = {{"error", "Invalid JSON or bad request"}};
            res.status = 400;
            res.set_content(errorJson.dump(), "application/json");
        }
    });

    // GET /api/urls - List all URLs
    httpServer.Get("/api/urls", [this](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        auto records = urlService.getAllURLs();
        json arr = json::array();
        for (const auto& r : records) {
            arr.push_back({
                {"id", r.id},
                {"shortCode", r.shortCode},
                {"originalUrl", r.originalUrl},
                {"createdAt", r.createdAt},
                {"active", r.active},
                {"clickCount", r.clickCount}
            });
        }
        json responseJson = {{"urls", arr}};
        res.status = 200;
        res.set_content(responseJson.dump(), "application/json");
    });

    // GET /api/metrics - Metrics endpoint
    httpServer.Get("/api/metrics", [this](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        auto records = urlService.getAllURLs();
        long long totalRedirects = 0;
        for (const auto& r : records) {
            totalRedirects += r.clickCount;
        }

        long long hits = lruCache ? lruCache->getHits() : 0;
        long long misses = lruCache ? lruCache->getMisses() : 0;
        double hitRate = lruCache ? lruCache->getHitRate() : 0.0;

        json responseJson = {
            {"totalUrls", records.size()},
            {"totalRedirects", totalRedirects},
            {"cacheHits", hits},
            {"cacheMisses", misses},
            {"cacheHitRate", hitRate}
        };

        res.status = 200;
        res.set_content(responseJson.dump(), "application/json");
    });

    // GET /api/urls/:shortCode/analytics - Analytics for short code
    httpServer.Get(R"(/api/urls/([^/]+)/analytics)", [this](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        std::string shortCode = req.matches[1];

        URLRecord record;
        if (!urlService.getURL(shortCode, record)) {
            json errorJson = {{"error", "URL not found"}};
            res.status = 404;
            res.set_content(errorJson.dump(), "application/json");
            return;
        }

        // We access database through URLService context or query via record clickCount / DB
        // Let's create helper struct or JSON
        json responseJson = {
            {"shortCode", shortCode},
            {"totalClicks", record.clickCount},
            {"originalUrl", record.originalUrl},
            {"createdAt", record.createdAt}
        };

        res.status = 200;
        res.set_content(responseJson.dump(), "application/json");
    });

    // GET /api/urls/:shortCode - Get URL details
    httpServer.Get(R"(/api/urls/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        std::string shortCode = req.matches[1];
        URLRecord record;
        if (urlService.getURL(shortCode, record)) {
            json responseJson = {
                {"id", record.id},
                {"shortCode", record.shortCode},
                {"originalUrl", record.originalUrl},
                {"createdAt", record.createdAt},
                {"active", record.active},
                {"clickCount", record.clickCount}
            };
            res.status = 200;
            res.set_content(responseJson.dump(), "application/json");
        } else {
            json errorJson = {{"error", "URL not found"}};
            res.status = 404;
            res.set_content(errorJson.dump(), "application/json");
        }
    });

    // DELETE /api/urls/:shortCode - Delete URL
    httpServer.Delete(R"(/api/urls/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        std::string shortCode = req.matches[1];
        if (urlService.deleteURL(shortCode)) {
            if (lruCache) {
                lruCache->remove(shortCode);
            }
            json responseJson = {{"success", true}};
            res.status = 200;
            res.set_content(responseJson.dump(), "application/json");
        } else {
            json errorJson = {{"error", "Failed to delete URL or URL not found"}};
            res.status = 404;
            res.set_content(errorJson.dump(), "application/json");
        }
    });

    // Load Balancer Endpoints
    httpServer.Get("/api/load-balancer", [this](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        if (!loadBalancer) {
            res.status = 404;
            res.set_content("{\"error\":\"Load balancer disabled\"}", "application/json");
            return;
        }

        auto sList = loadBalancer->getServers();
        json sArr = json::array();
        for (const auto& s : sList) {
            sArr.push_back({
                {"id", s.id},
                {"healthy", s.healthy},
                {"activeConnections", s.activeConnections},
                {"totalRequests", s.totalRequests}
            });
        }

        json responseJson = {
            {"algorithm", loadBalancer->getAlgorithm()},
            {"servers", sArr}
        };
        res.status = 200;
        res.set_content(responseJson.dump(), "application/json");
    });

    httpServer.Post("/api/load-balancer/algorithm", [this](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        if (!loadBalancer) {
            res.status = 404;
            res.set_content("{\"error\":\"Load balancer disabled\"}", "application/json");
            return;
        }

        try {
            auto body = json::parse(req.body);
            std::string algo = body.value("algorithm", "ROUND_ROBIN");
            loadBalancer->setAlgorithm(algo);

            json responseJson = {
                {"algorithm", loadBalancer->getAlgorithm()},
                {"success", true}
            };
            res.status = 200;
            res.set_content(responseJson.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid request body\"}", "application/json");
        }
    });

    httpServer.Get("/api/servers", [this](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        if (!loadBalancer) {
            res.status = 404;
            return;
        }
        auto sList = loadBalancer->getServers();
        json sArr = json::array();
        for (const auto& s : sList) {
            sArr.push_back({
                {"id", s.id},
                {"healthy", s.healthy},
                {"activeConnections", s.activeConnections},
                {"totalRequests", s.totalRequests}
            });
        }
        json responseJson = {{"servers", sArr}};
        res.status = 200;
        res.set_content(responseJson.dump(), "application/json");
    });

    httpServer.Post("/api/servers", [this](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        if (!loadBalancer) {
            res.status = 404;
            return;
        }
        SimulatedServer s = loadBalancer->addServer();
        json responseJson = {
            {"id", s.id},
            {"healthy", s.healthy},
            {"activeConnections", s.activeConnections},
            {"totalRequests", s.totalRequests}
        };
        res.status = 201;
        res.set_content(responseJson.dump(), "application/json");
    });

    httpServer.Delete(R"(/api/servers/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        if (!loadBalancer) {
            res.status = 404;
            return;
        }
        int id = std::stoi(req.matches[1]);
        if (loadBalancer->removeServer(id)) {
            res.status = 200;
            res.set_content("{\"success\":true}", "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"error\":\"Server not found\"}", "application/json");
        }
    });

    httpServer.Post(R"(/api/servers/(\d+)/kill)", [this](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        if (!loadBalancer) {
            res.status = 404;
            return;
        }
        int id = std::stoi(req.matches[1]);
        if (loadBalancer->killServer(id)) {
            res.status = 200;
            res.set_content("{\"success\":true}", "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"error\":\"Server not found\"}", "application/json");
        }
    });

    httpServer.Post(R"(/api/servers/(\d+)/restore)", [this](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        if (!loadBalancer) {
            res.status = 404;
            return;
        }
        int id = std::stoi(req.matches[1]);
        if (loadBalancer->restoreServer(id)) {
            res.status = 200;
            res.set_content("{\"success\":true}", "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"error\":\"Server not found\"}", "application/json");
        }
    });

    httpServer.Post("/api/load-balancer/simulate", [this](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        if (!loadBalancer) {
            res.status = 404;
            return;
        }

        int count = 1000;
        try {
            if (!req.body.empty()) {
                auto body = json::parse(req.body);
                count = body.value("count", 1000);
            }
        } catch (...) {}

        auto dist = loadBalancer->simulateRequests(count);
        json distObj = json::object();
        for (const auto& kv : dist) {
            distObj[std::to_string(kv.first)] = kv.second;
        }

        auto sList = loadBalancer->getServers();
        json sArr = json::array();
        for (const auto& s : sList) {
            sArr.push_back({
                {"id", s.id},
                {"healthy", s.healthy},
                {"activeConnections", s.activeConnections},
                {"totalRequests", s.totalRequests}
            });
        }

        json responseJson = {
            {"algorithm", loadBalancer->getAlgorithm()},
            {"simulatedCount", count},
            {"distribution", distObj},
            {"servers", sArr}
        };
        res.status = 200;
        res.set_content(responseJson.dump(), "application/json");
    });

    // GET /:shortCode - Redirect to original URL (with Cache + Async Analytics integration)
    httpServer.Get(R"(/([a-zA-Z0-9_-]+))", [this](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        std::string shortCode = req.matches[1];

        // Skip static system or api paths if match occurs
        if (shortCode == "api" || shortCode == "favicon.ico") {
            res.status = 404;
            return;
        }

        std::string cachedUrl;
        std::string nowStr = getNowTimestampString();

        // 1. Check LRU Cache
        if (lruCache && lruCache->get(shortCode, cachedUrl)) {
            // Cache HIT
            if (analyticsQueue) {
                analyticsQueue->push(shortCode, nowStr);
            }
            res.set_redirect(cachedUrl.c_str(), 302);
            return;
        }

        // 2. Cache MISS -> Query SQLite DB
        URLRecord record;
        bool found = urlService.getURL(shortCode, record);

        if (found) {
            if (lruCache) {
                lruCache->put(shortCode, record.originalUrl);
            }
            if (analyticsQueue) {
                analyticsQueue->push(shortCode, nowStr);
            }
            res.set_redirect(record.originalUrl.c_str(), 302);
        } else {
            res.status = 404;
            res.set_content("<html><body><h1>404 Not Found</h1><p>Short URL code not found.</p></body></html>", "text/html");
        }
    });
}

void Server::start() {
    std::cout << "ShortX C++ Backend Server listening on http://" << host << ":" << port << std::endl;
    httpServer.listen(host.c_str(), port);
}

void Server::stop() {
    if (httpServer.is_running()) {
        httpServer.stop();
    }
}
