#include "Worker.h"

void Worker::operator()()
{

  std::unique_lock<std::mutex> lock(m_threadPool->m_mutex); 

  while(m_threadPool->m_running)
  {

    m_threadPool->var.wait(lock);    

    if (!m_threadPool->m_tasks.empty())
    {

      std::function<void()> task = m_threadPool->m_tasks.front();

      m_threadPool->m_tasks.pop();

      lock.unlock();
      task();
      lock.lock();

    }

  }
  
}
