#include "AnalyticsQueue.h"
#include <iostream>

AnalyticsQueue::AnalyticsQueue(Database& db)
    : stopping(false), database(db) {
    workerThread = std::thread(&AnalyticsQueue::processQueue, this);
}

AnalyticsQueue::~AnalyticsQueue() {
    stop();
}

void AnalyticsQueue::push(const ClickEvent& event) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (stopping) return;
        queue.push(event);
    }
    condition.notify_one();
}

void AnalyticsQueue::push(const std::string& shortCode, const std::string& timestamp) {
    push(ClickEvent{shortCode, timestamp});
}

void AnalyticsQueue::processQueue() {
    while (true) {
        ClickEvent event;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] {
                return stopping || !queue.empty();
            });

            if (stopping && queue.empty()) {
                return;
            }

            event = queue.front();
            queue.pop();
        }

        // Process event asynchronously: insert into click_events table and increment click_count
        database.logClickEvent(event.shortCode, event.timestamp);
        database.incrementClickCount(event.shortCode);
    }
}

void AnalyticsQueue::stop() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (stopping) return;
        stopping = true;
    }
    condition.notify_all();

    if (workerThread.joinable()) {
        workerThread.join();
    }
}

size_t AnalyticsQueue::size() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return queue.size();
}
