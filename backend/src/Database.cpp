#include "Database.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

static std::string getCurrentTimestamp() {
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

Database::Database(const std::string& dbPath)
    : db(nullptr), dbPath(dbPath) {}

Database::~Database() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool Database::initialize() {
    std::lock_guard<std::mutex> lock(dbMutex);
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // Enable WAL mode for better concurrency
    char* errMsgs = nullptr;
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);

    const char* createUrlsTableSQL = R"(
        CREATE TABLE IF NOT EXISTS urls (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            short_code TEXT UNIQUE,
            original_url TEXT NOT NULL,
            created_at TEXT NOT NULL,
            active INTEGER NOT NULL DEFAULT 1,
            click_count INTEGER NOT NULL DEFAULT 0
        );
    )";

    rc = sqlite3_exec(db, createUrlsTableSQL, nullptr, nullptr, &errMsgs);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to create urls table: " << errMsgs << std::endl;
        sqlite3_free(errMsgs);
        return false;
    }

    const char* createClickEventsTableSQL = R"(
        CREATE TABLE IF NOT EXISTS click_events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            short_code TEXT NOT NULL,
            timestamp TEXT NOT NULL
        );
    )";

    rc = sqlite3_exec(db, createClickEventsTableSQL, nullptr, nullptr, &errMsgs);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to create click_events table: " << errMsgs << std::endl;
        sqlite3_free(errMsgs);
        return false;
    }

    return true;
}

long long Database::createUrlRecord(const std::string& originalUrl, const std::string& customAlias) {
    std::lock_guard<std::mutex> lock(dbMutex);
    std::string nowStr = getCurrentTimestamp();

    const char* sql = "INSERT INTO urls (short_code, original_url, created_at, active, click_count) VALUES (?, ?, ?, 1, 0);";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return -1;
    }

    if (customAlias.empty()) {
        sqlite3_bind_null(stmt, 1);
    } else {
        sqlite3_bind_text(stmt, 1, customAlias.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(stmt, 2, originalUrl.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, nowStr.c_str(), -1, SQLITE_TRANSIENT);

    int stepResult = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (stepResult != SQLITE_DONE) {
        return -1;
    }

    return sqlite3_last_insert_rowid(db);
}

bool Database::updateShortCode(long long id, const std::string& shortCode) {
    std::lock_guard<std::mutex> lock(dbMutex);
    const char* sql = "UPDATE urls SET short_code = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, shortCode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, id);

    int stepResult = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (stepResult == SQLITE_DONE);
}

bool Database::getUrlByShortCode(const std::string& shortCode, URLRecord& record) {
    std::lock_guard<std::mutex> lock(dbMutex);
    const char* sql = "SELECT id, short_code, original_url, created_at, active, click_count FROM urls WHERE short_code = ? AND active = 1;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, shortCode.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.id = sqlite3_column_int64(stmt, 0);
        const unsigned char* sc = sqlite3_column_text(stmt, 1);
        record.shortCode = sc ? reinterpret_cast<const char*>(sc) : "";
        const unsigned char* ou = sqlite3_column_text(stmt, 2);
        record.originalUrl = ou ? reinterpret_cast<const char*>(ou) : "";
        const unsigned char* ca = sqlite3_column_text(stmt, 3);
        record.createdAt = ca ? reinterpret_cast<const char*>(ca) : "";
        record.active = (sqlite3_column_int(stmt, 4) != 0);
        record.clickCount = sqlite3_column_int(stmt, 5);
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

std::vector<URLRecord> Database::getAllUrls() {
    std::lock_guard<std::mutex> lock(dbMutex);
    std::vector<URLRecord> results;

    const char* sql = "SELECT id, short_code, original_url, created_at, active, click_count FROM urls WHERE active = 1 ORDER BY id DESC;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return results;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        URLRecord record;
        record.id = sqlite3_column_int64(stmt, 0);
        const unsigned char* sc = sqlite3_column_text(stmt, 1);
        record.shortCode = sc ? reinterpret_cast<const char*>(sc) : "";
        const unsigned char* ou = sqlite3_column_text(stmt, 2);
        record.originalUrl = ou ? reinterpret_cast<const char*>(ou) : "";
        const unsigned char* ca = sqlite3_column_text(stmt, 3);
        record.createdAt = ca ? reinterpret_cast<const char*>(ca) : "";
        record.active = (sqlite3_column_int(stmt, 4) != 0);
        record.clickCount = sqlite3_column_int(stmt, 5);
        results.push_back(record);
    }

    sqlite3_finalize(stmt);
    return results;
}

bool Database::deleteUrl(const std::string& shortCode) {
    std::lock_guard<std::mutex> lock(dbMutex);
    const char* sql = "UPDATE urls SET active = 0 WHERE short_code = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, shortCode.c_str(), -1, SQLITE_TRANSIENT);

    int stepResult = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (stepResult == SQLITE_DONE);
}

bool Database::incrementClickCount(const std::string& shortCode) {
    std::lock_guard<std::mutex> lock(dbMutex);
    const char* sql = "UPDATE urls SET click_count = click_count + 1 WHERE short_code = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, shortCode.c_str(), -1, SQLITE_TRANSIENT);

    int stepResult = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (stepResult == SQLITE_DONE);
}

bool Database::logClickEvent(const std::string& shortCode, const std::string& timestamp) {
    std::lock_guard<std::mutex> lock(dbMutex);
    const char* sql = "INSERT INTO click_events (short_code, timestamp) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, shortCode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, timestamp.c_str(), -1, SQLITE_TRANSIENT);

    int stepResult = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (stepResult == SQLITE_DONE);
}

int Database::getTotalClicks(const std::string& shortCode) {
    std::lock_guard<std::mutex> lock(dbMutex);
    const char* sql = "SELECT COUNT(*) FROM click_events WHERE short_code = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, shortCode.c_str(), -1, SQLITE_TRANSIENT);

    int total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return total;
}

int Database::getClicksToday(const std::string& shortCode, const std::string& todayDateStr) {
    std::lock_guard<std::mutex> lock(dbMutex);
    const char* sql = "SELECT COUNT(*) FROM click_events WHERE short_code = ? AND timestamp LIKE ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    std::string pattern = todayDateStr + "%";
    sqlite3_bind_text(stmt, 1, shortCode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, pattern.c_str(), -1, SQLITE_TRANSIENT);

    int total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        total = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return total;
}

std::vector<std::pair<std::string, int>> Database::getClicksByDay(const std::string& shortCode) {
    std::lock_guard<std::mutex> lock(dbMutex);
    std::vector<std::pair<std::string, int>> results;

    const char* sql = R"(
        SELECT substr(timestamp, 1, 10) AS day, COUNT(*) 
        FROM click_events 
        WHERE short_code = ? 
        GROUP BY day 
        ORDER BY day ASC;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return results;
    }

    sqlite3_bind_text(stmt, 1, shortCode.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* dayTxt = sqlite3_column_text(stmt, 0);
        std::string day = dayTxt ? reinterpret_cast<const char*>(dayTxt) : "";
        int count = sqlite3_column_int(stmt, 1);
        results.push_back({day, count});
    }

    sqlite3_finalize(stmt);
    return results;
}
