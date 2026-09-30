#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include "Base62Encoder.h"
#include "LRUCache.h"
#include "Database.h"
#include "URLService.h"
#include "LoadBalancer.h"
#include "AnalyticsQueue.h"

int testCount = 0;
int passCount = 0;

#define TEST_ASSERT(condition, msg) \
    do { \
        testCount++; \
        if (condition) { \
            passCount++; \
            std::cout << "  [PASS] " << msg << std::endl; \
        } else { \
            std::cout << "  [FAIL] " << msg << " (Line " << __LINE__ << ")" << std::endl; \
        } \
    } while (0)

void testBase62Encoder() {
    std::cout << "\n--- Testing Base62Encoder ---" << std::endl;
    TEST_ASSERT(Base62Encoder::encode(1) == "1", "encode(1) == '1'");
    TEST_ASSERT(Base62Encoder::encode(62) == "10", "encode(62) == '10'");
    TEST_ASSERT(Base62Encoder::encode(3844) == "100", "encode(3844) == '100'");
    TEST_ASSERT(Base62Encoder::decode("1") == 1, "decode('1') == 1");
    TEST_ASSERT(Base62Encoder::decode("10") == 62, "decode('10') == 62");
    TEST_ASSERT(Base62Encoder::decode("100") == 3844, "decode('100') == 3844");

    long long num = 123456789LL;
    std::string encoded = Base62Encoder::encode(num);
    TEST_ASSERT(Base62Encoder::decode(encoded) == num, "Base62 Round-trip test");
}

void testLRUCache() {
    std::cout << "\n--- Testing LRUCache ---" << std::endl;
    LRUCache cache(2);

    std::string val;
    TEST_ASSERT(!cache.get("a", val), "Cache miss on empty cache");
    
    cache.put("a", "alpha");
    cache.put("b", "beta");
    TEST_ASSERT(cache.size() == 2, "Cache size should be 2");

    TEST_ASSERT(cache.get("a", val) && val == "alpha", "Cache hit for 'a'");

    // Put third item, should evict 'b' (since 'a' was accessed recently)
    cache.put("c", "gamma");
    TEST_ASSERT(cache.get("a", val), "a still present after 'b' evicted");
    TEST_ASSERT(!cache.get("b", val), "b was evicted as LRU");
    TEST_ASSERT(cache.getHits() > 0, "Hits recorded");
    TEST_ASSERT(cache.getEvictions() == 1, "Eviction count is 1");
}

void testURLService() {
    std::cout << "\n--- Testing URLService & Database ---" << std::endl;
    Database db("test_shortx.db");
    TEST_ASSERT(db.initialize(), "Database initialization");

    URLService service(db);

    std::string code1 = service.createURL("https://github.com/");
    TEST_ASSERT(!code1.empty(), "Created short URL code");

    URLRecord rec;
    TEST_ASSERT(service.getURL(code1, rec), "Get created URL record");
    TEST_ASSERT(rec.originalUrl == "https://github.com/", "Original URL matches");

    // Custom alias test
    std::string alias = service.createURL("https://google.com/", "google-search");
    TEST_ASSERT(alias == "google-search", "Custom alias created");

    // Duplicate alias test
    bool caughtConflict = false;
    try {
        service.createURL("https://bing.com/", "google-search");
    } catch (const std::runtime_error& e) {
        if (std::string(e.what()) == "ALIAS_EXISTS") {
            caughtConflict = true;
        }
    }
    TEST_ASSERT(caughtConflict, "Duplicate alias rejected with ALIAS_EXISTS");

    // Delete URL test
    TEST_ASSERT(service.deleteURL(code1), "Delete URL succeeded");
    TEST_ASSERT(!service.getURL(code1, rec), "Deleted URL no longer retrieved");
}

void testLoadBalancer() {
    std::cout << "\n--- Testing LoadBalancer ---" << std::endl;
    LoadBalancer lb(3);
    TEST_ASSERT(lb.getServers().size() == 3, "Initial 3 servers");

    // Round Robin distribution test
    lb.setAlgorithm("ROUND_ROBIN");
    int s1 = lb.routeRequest("req1");
    int s2 = lb.routeRequest("req2");
    int s3 = lb.routeRequest("req3");
    int s4 = lb.routeRequest("req4");
    TEST_ASSERT(s1 == 1 && s2 == 2 && s3 == 3 && s4 == 1, "Round Robin cyclic routing");

    // Unhealthy server exclusion test
    lb.killServer(2);
    int s5 = lb.routeRequest("req5");
    int s6 = lb.routeRequest("req6");
    TEST_ASSERT(s5 != 2 && s6 != 2, "Unhealthy server 2 excluded from routing");

    // Least Connections test
    lb.restoreServer(2);
    lb.setAlgorithm("LEAST_CONNECTIONS");
    int routed = lb.routeRequest("req7");
    TEST_ASSERT(routed >= 1 && routed <= 3, "Least Connections routed valid server");

    // Consistent Hashing test
    lb.setAlgorithm("CONSISTENT_HASHING");
    int r1 = lb.routeRequest("user_123");
    int r2 = lb.routeRequest("user_123");
    TEST_ASSERT(r1 == r2, "Consistent Hashing routes identical key to same server");
}

void testAnalyticsQueue() {
    std::cout << "\n--- Testing AnalyticsQueue ---" << std::endl;
    Database db("test_analytics.db");
    db.initialize();

    AnalyticsQueue queue(db);
    queue.push("testCode", "2026-09-30 14:00:00");
    queue.push("testCode", "2026-09-30 14:05:00");

    // Give background worker thread time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TEST_ASSERT(db.getTotalClicks("testCode") >= 2, "AnalyticsQueue processed click events into DB");
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "       ShortX Unit Test Suite             " << std::endl;
    std::cout << "==========================================" << std::endl;

    testBase62Encoder();
    testLRUCache();
    testURLService();
    testLoadBalancer();
    testAnalyticsQueue();

    std::cout << "\n==========================================" << std::endl;
    std::cout << " Summary: " << passCount << " / " << testCount << " tests PASSED." << std::endl;
    std::cout << "==========================================" << std::endl;

    return (passCount == testCount) ? 0 : 1;
}
