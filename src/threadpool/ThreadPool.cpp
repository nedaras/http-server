#include "ThreadPool.h"

#include <thread>
#include "Worker.h"


ThreadPool::~ThreadPool()
{

  m_running = false;

}

ThreadPool::ThreadPool(unsigned int threads)
{

  m_threads.reserve(threads);

  for (unsigned int i = 0; i < threads; i++)
  {
 
    Worker worker = Worker(this);

    m_threads.push_back(std::thread(worker));

  }

}
