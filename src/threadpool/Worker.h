#pragma once

#include "ThreadPool.h"

class Worker
{

public:

  Worker(ThreadPool* threadPool) : m_threadPool(threadPool) {}; 

  void operator()();
  
private:

  ThreadPool* m_threadPool;

};
