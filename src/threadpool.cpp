// Copyright (c) Maxwell Hunt and Alexander Kaminsky 2025. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license
// information.

#include "threadpool.h"

#include <print>

#include "tracy/Tracy.hpp"

ThreadPool::ThreadPool(std::size_t numThreads) {
    _threads.reserve(numThreads);
    for(int i = 0;i < numThreads;i++) {
        _threads.emplace_back([this, i] {
            while(true) {
                tracy::SetThreadName(std::format("ThreadPool #{}", i).c_str());
                std::function<void()> workUnit;
                {
                    ZoneScopedN("Waiting for queued work");
                    std::unique_lock lock(_mutex);
                    _availableWorkCV.wait(lock, [&]{return _stopping || !_workQueue.empty();});

                    if(_stopping && _workQueue.empty())
                        break;

                    workUnit = std::move(_workQueue.front());
                    _workQueue.pop();
                }
                {
                    ZoneScopedN("Running work unit");
                    workUnit();
                }
                {
                    ZoneScopedN("Notify work completed");
                    std::lock_guard lock(_mutex);
                    --_numActiveTasks;
                    if(_numActiveTasks == 0 && _workQueue.empty())
                        _waitCV.notify_all();
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
    std::lock_guard lock(_mutex);
    _stopping = true;
    }
    _availableWorkCV.notify_all();
    for(std::thread& thread : _threads) {
        thread.join();
    }
}

void ThreadPool::assignWork(std::function<void()> work) {
    {
        std::lock_guard lock(_mutex);
        _workQueue.push(std::move(work));
        ++_numActiveTasks;
    }
    _availableWorkCV.notify_one();
}

void ThreadPool::wait() {
    std::unique_lock lock(_mutex);
    _waitCV.wait(lock, [this] {
        return _numActiveTasks == 0 && _workQueue.empty();
    });
}
