#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <string>
#include <unordered_map>
#include <list>
#include <mutex>

class LRUCache {
private:
    struct CacheNode {
        std::string key;
        std::string value;
    };

    size_t capacity;
    std::list<CacheNode> cacheList; // Front is most recently used, back is least recently used
    std::unordered_map<std::string, std::list<CacheNode>::iterator> cacheMap;
    mutable std::mutex cacheMutex;

    long long hits;
    long long misses;
    long long evictions;

public:
    explicit LRUCache(size_t capacity = 100);

    bool get(const std::string& key, std::string& value);
    void put(const std::string& key, const std::string& value);
    void remove(const std::string& key);
    size_t size() const;
    void clear();

    // Metrics getters
    long long getHits() const;
    long long getMisses() const;
    long long getEvictions() const;
    double getHitRate() const;
};

#endif // LRU_CACHE_H
