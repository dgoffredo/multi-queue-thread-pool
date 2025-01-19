#include "multi-queue-thread-pool.h"
#include "thread-pool.h"

// class JobQueue
// --------------

void JobQueue::run_front_job(ThreadPool& pool) {
    (jobs.front())();

    std::list<std::function<void()>> node;
    {
        std::lock_guard<std::mutex> lock{mutex};
        
        job_pending = false;
        node.splice(node.end(), jobs, jobs.begin());
        // When we notify this condition variable, we might be waking up our
        // destructor. So, be sure to hold the lock until either
        // `job_pending == true` or we return.
        current_job_done.notify_all();
        
        // Enqueue the next job, unless there isn't one or if we're paused.
        if (jobs.empty()) {
            empty.notify_all();
            return;
        }
        if (paused) {
            return;
        }
        
        job_pending = true;
    }

    pool.enqueue([this, &pool]() {
        run_front_job(pool);
    });
}

JobQueue::JobQueue()
: job_pending(false)
, sealed(false)
, paused(false) {}

int JobQueue::enqueue(std::function<void()> job, ThreadPool& pool) {
    std::list<std::function<void()>> node;
    node.push_back(std::move(job));
    
    std::size_t old_size;
    {
        std::lock_guard<std::mutex> lock{mutex};
        if (sealed) {
            return 1;
        }
        old_size = jobs.size();
        jobs.splice(jobs.end(), node);
        if (old_size == 0) {
            job_pending = true;
        }
    }
    
    if (old_size == 0) {
        // Whichever thread previously finished a job (if any) didn't have any
        // more jobs to execute. Submit the job now.
        pool.enqueue([this, &pool]() {
            run_front_job(pool);
        });
    }
    
    return 0;
}

void JobQueue::seal() {
    std::lock_guard<std::mutex> lock{mutex};
    sealed = true;
}

void JobQueue::unseal() {
    std::lock_guard<std::mutex> lock{mutex};
    sealed = false;
}

void JobQueue::pause() {
    std::lock_guard<std::mutex> lock{mutex};
    paused = true;
}

void JobQueue::unpause() {
    std::lock_guard<std::mutex> lock{mutex};
    paused = false;
}

void JobQueue::wait_for_current() {
    std::unique_lock<std::mutex> lock{mutex};
    current_job_done.wait(lock, [this]() {
        return !job_pending;
    });
}

void JobQueue::wait_for_empty() {
    std::unique_lock<std::mutex> lock{mutex};
    empty.wait(lock, [this]() {
        return jobs.empty();
    });
}

void JobQueue::flush() {
    std::unique_lock<std::mutex> lock{mutex};
    sealed = true;
    empty.wait(lock, [this]() {
        return jobs.empty();
    });
}

void JobQueue::stop() {
    std::unique_lock<std::mutex> lock{mutex};
    paused = true;
    current_job_done.wait(lock, [this]() {
        return !job_pending;
    });
}

// class QueueHandle
// -----------------

QueueHandle::QueueHandle(std::list<JobQueue>::iterator queue)
: queue(queue) {}

// class MultiQueueThreadPool
// --------------------------

MultiQueueThreadPool::MultiQueueThreadPool(ThreadPool& pool)
: pool(pool) {}

MultiQueueThreadPool::~MultiQueueThreadPool() {
    for (JobQueue& queue : queues) {
        queue.stop();
    }
}

QueueHandle MultiQueueThreadPool::create_queue() {
    std::list<JobQueue> node(1);
    std::lock_guard<std::mutex> lock{mutex};
    queues.splice(queues.begin(), node);
    return QueueHandle(queues.begin());
}

void MultiQueueThreadPool::destroy_queue(QueueHandle handle) {
    handle.queue->stop();
    std::list<JobQueue> node;
    std::lock_guard<std::mutex> lock{mutex};
    node.splice(node.end(), queues, handle.queue);
}

int MultiQueueThreadPool::enqueue(QueueHandle handle, std::function<void()> job) {
    return handle.queue->enqueue(std::move(job), pool);
}

void MultiQueueThreadPool::seal(QueueHandle handle) {
    handle.queue->seal();
}

void MultiQueueThreadPool::unseal(QueueHandle handle) {
    handle.queue->unseal();
}

void MultiQueueThreadPool::pause(QueueHandle handle) {
    handle.queue->pause();
}

void MultiQueueThreadPool::unpause(QueueHandle handle) {
    handle.queue->unpause();
}

void MultiQueueThreadPool::stop(QueueHandle handle) {
    handle.queue->stop();
}

void MultiQueueThreadPool::flush(QueueHandle handle) {
    handle.queue->flush();
}

void MultiQueueThreadPool::flush_all() {
    for (JobQueue& queue : queues) {
        queue.flush();
    }
}
