#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <mutex>
#include "sqlite3.h"

struct URLRecord {
    long long id;
    std::string shortCode;
    std::string originalUrl;
    std::string createdAt;
    bool active;
    int clickCount;
};

struct ClickEventRecord {
    long long id;
    std::string shortCode;
    std::string timestamp;
};

class Database {
private:
    sqlite3* db;
    std::string dbPath;
    std::mutex dbMutex;

public:
    explicit Database(const std::string& dbPath = "shortx.db");
    ~Database();

    bool initialize();

    // URL Table Operations
    long long createUrlRecord(const std::string& originalUrl, const std::string& customAlias = "");
    bool updateShortCode(long long id, const std::string& shortCode);
    bool getUrlByShortCode(const std::string& shortCode, URLRecord& record);
    std::vector<URLRecord> getAllUrls();
    bool deleteUrl(const std::string& shortCode);
    bool incrementClickCount(const std::string& shortCode);

    // Analytics Table Operations
    bool logClickEvent(const std::string& shortCode, const std::string& timestamp);
    int getTotalClicks(const std::string& shortCode);
    int getClicksToday(const std::string& shortCode, const std::string& todayDateStr);
    std::vector<std::pair<std::string, int>> getClicksByDay(const std::string& shortCode);
};

#endif // DATABASE_H
