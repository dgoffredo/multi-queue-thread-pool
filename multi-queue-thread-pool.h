#pragma once

#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>

class ThreadPool;

class JobQueue {
    std::mutex mutex;
    std::list<std::function<void()>> jobs;
    std::condition_variable current_job_done;
    std::condition_variable empty;
    bool job_pending : 1; // `jobs.front()` is in the thread pool
    bool sealed : 1; // enqueuing is disallowed
    bool paused : 1; // thread pool will not run another job

    void run_front_job(ThreadPool&);

public:
    JobQueue();
    // The behavior is undefined unless this queue is paused an no job is
    // pending.
    ~JobQueue() = default;

    // return zero on success
    int enqueue(std::function<void()> job, ThreadPool&);

    void seal(); // disable enqueue
    void unseal(); // enable enqueue
    void pause(); // disable job execution
    void unpause(ThreadPool&); // enable job execution
    void wait_for_current(); // block until the currently pending job, if any, finishes
    void wait_for_empty(); // block until the queue is empty
    void flush(); // `seal()` and `wait_for_empty()`
    void stop(); // `pause()` and `wait_for_current()`
};

class QueueHandle {
    std::list<JobQueue>::iterator queue;
    friend class MultiQueueThreadPool;
    QueueHandle() = delete;
    // for use by `MultiQueueThreadPool` only
    explicit QueueHandle(std::list<JobQueue>::iterator);

public:
    QueueHandle(const QueueHandle&) = default;
    QueueHandle(QueueHandle&&) = default;
    ~QueueHandle() = default;
    QueueHandle& operator=(const QueueHandle&) = default;
    QueueHandle& operator=(QueueHandle&&) = default;
};

class MultiQueueThreadPool {
    std::mutex mutex;
    std::list<JobQueue> queues;
    ThreadPool& pool;

public:
    explicit MultiQueueThreadPool(ThreadPool&);
    ~MultiQueueThreadPool();

    MultiQueueThreadPool(const MultiQueueThreadPool&) = delete;
    MultiQueueThreadPool(MultiQueueThreadPool&&) = delete;
    MultiQueueThreadPool& operator=(const MultiQueueThreadPool&) = delete;
    MultiQueueThreadPool& operator=(MultiQueueThreadPool&&) = delete;

    QueueHandle create_queue();
    void destroy_queue(QueueHandle);

    // return zero on success
    int enqueue(QueueHandle, std::function<void()> job);

    void seal(QueueHandle); // disable enqueue
    void unseal(QueueHandle); // enable enqueue
    void pause(QueueHandle); // disable future execution of enqueued jobs
    void unpause(QueueHandle); // enable future execution of enqueued jobs
    void stop(QueueHandle); // `pause()` and wait for current job to finish
    void flush(QueueHandle); // `seal()` and wait for queue to empty
    
    void flush_all();
};
