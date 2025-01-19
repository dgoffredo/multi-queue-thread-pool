#include "fixed-thread-pool.h"

void FixedThreadPool::consume() {
    std::list<std::function<void()>> node;
    for (;;) {
        std::unique_lock<std::mutex> lock{mutex};
        not_empty.wait(lock, [this]() {
            return shutting_down || !jobs.empty();
        });
        if (shutting_down) {
            return;
        }
        node.splice(node.end(), jobs, jobs.begin());

        lock.unlock();
       (*node.begin())();
       node.clear();
    }
}

FixedThreadPool::FixedThreadPool(int num_threads)
: shutting_down(false) {
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([this]() { consume(); });
    }
}

FixedThreadPool::~FixedThreadPool() {
    {
        std::lock_guard<std::mutex> lock{mutex};
        shutting_down = true;
    }
    not_empty.notify_all();
    for (std::thread& thread : threads) {
        thread.join();
    }
}

void FixedThreadPool::enqueue(std::function<void()> job) {
    std::list<std::function<void()>> node;
    node.push_back(std::move(job));
    std::size_t old_size;
    {
        std::lock_guard<std::mutex> lock{mutex};
        old_size = jobs.size();
        jobs.splice(jobs.end(), node);
    }
    if (old_size == 0) {
        not_empty.notify_all();
    }
}
