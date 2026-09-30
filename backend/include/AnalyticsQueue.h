#ifndef ANALYTICS_QUEUE_H
#define ANALYTICS_QUEUE_H

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "Database.h"

struct ClickEvent {
    std::string shortCode;
    std::string timestamp;
};

class AnalyticsQueue {
private:
    std::queue<ClickEvent> queue;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stopping;
    std::thread workerThread;
    Database& database;

    void processQueue();

public:
    explicit AnalyticsQueue(Database& db);
    ~AnalyticsQueue();

    void push(const ClickEvent& event);
    void push(const std::string& shortCode, const std::string& timestamp);
    void stop();
    size_t size();
};

#endif // ANALYTICS_QUEUE_H
