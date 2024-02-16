#pragma once

#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool
{

public:
 
  ThreadPool(unsigned int threads);
  ~ThreadPool();

  template <typename F, typename... Args>
  void addTask(F&& function, Args&&... args)
  {

    std::function<void()> task = std::bind(std::forward<F>(function), std::forward<Args>(args)...);
    
    std::lock_guard<std::mutex> lock(m_mutex);

    m_tasks.push(std::move(task));

    var.notify_one();

  }


private:
 
  friend class Worker;

  bool m_running = true;

  std::mutex m_mutex;
  std::condition_variable var; 

  std::vector<std::thread> m_threads;
  std::queue<std::function<void()>> m_tasks; 

};
