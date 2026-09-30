#include "LRUCache.h"

LRUCache::LRUCache(size_t capacity)
    : capacity(capacity), hits(0), misses(0), evictions(0) {
    if (this->capacity == 0) {
        this->capacity = 1;
    }
}

bool LRUCache::get(const std::string& key, std::string& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = cacheMap.find(key);
    if (it == cacheMap.end()) {
        misses++;
        return false;
    }

    // Move accessed node to front (most recently used)
    cacheList.splice(cacheList.begin(), cacheList, it->second);
    value = it->second->value;
    hits++;
    return true;
}

void LRUCache::put(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = cacheMap.find(key);

    if (it != cacheMap.end()) {
        // Key exists, update value and move node to front
        it->second->value = value;
        cacheList.splice(cacheList.begin(), cacheList, it->second);
        return;
    }

    // Check capacity and evict LRU item if full
    if (cacheList.size() >= capacity) {
        auto last = cacheList.end();
        --last;
        cacheMap.erase(last->key);
        cacheList.pop_back();
        evictions++;
    }

    // Insert new item at front
    cacheList.push_front({key, value});
    cacheMap[key] = cacheList.begin();
}

void LRUCache::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = cacheMap.find(key);
    if (it != cacheMap.end()) {
        cacheList.erase(it->second);
        cacheMap.erase(it);
    }
}

size_t LRUCache::size() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return cacheList.size();
}

void LRUCache::clear() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    cacheList.clear();
    cacheMap.clear();
}

long long LRUCache::getHits() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return hits;
}

long long LRUCache::getMisses() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return misses;
}

long long LRUCache::getEvictions() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return evictions;
}

double LRUCache::getHitRate() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    long long total = hits + misses;
    if (total == 0) return 0.0;
    return (static_cast<double>(hits) / total) * 100.0;
}
