#pragma once

#include <mutex>
#include <condition_variable>
#include <functional>
#include <vector>
#include <queue>

class ThreadPool {
public:
    ThreadPool(std::size_t numThreads);

    ~ThreadPool();

    void assignWork(std::function<void()> work);

    void wait();
private:
    bool _stopping = false;
    uint32_t _numActiveTasks = 0;
    std::mutex _mutex;
    std::condition_variable _availableWorkCV;
    std::condition_variable _waitCV;
    std::vector<std::thread> _threads;
    std::queue<std::function<void()>> _workQueue;
};