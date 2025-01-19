#pragma once

#include <functional>

class ThreadPool {
 public:
  virtual ~ThreadPool() = default;

  // Submit the specified `job` to be executed by a thread.
  virtual void enqueue(std::function<void()> job) = 0;
};
