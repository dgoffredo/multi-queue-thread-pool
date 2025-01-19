#pragma once

#include "thread-pool.h"

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

class FixedThreadPool : public ThreadPool {
    std::mutex mutex;
    std::list<std::function<void()>> jobs;
    std::condition_variable not_empty;
    bool shutting_down;

    std::vector<std::thread> threads;
    
    void consume();

public:
    explicit FixedThreadPool(int num_threads);
    ~FixedThreadPool();

    void enqueue(std::function<void()> job) override;
};
