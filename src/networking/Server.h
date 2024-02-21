#pragma once

#include "Request.h"
#include "Response.h"
#include "../minheap/MinHeap.h"
#include <chrono>
#include <functional>
#include <mutex>
#include <queue>
#include <sys/epoll.h>

class Server
{

public:

  // is move good?
  Server(const std::function<void(const Request* request, const Response& response)>& m_callback) : m_callback(std::move(m_callback)) {}

  int listen(const char* port);

private:

  void removeRequest(Request* request); 

private:

  friend class Response;

  std::mutex m_mutex;

  const std::function<void(const Request* request, const Response& response)> m_callback;

  struct CompareRequests
  {

    bool operator()(Request* left, Request* right)
    {
      return left > right;
    }

  };

  std::vector<epoll_event> m_events;
  MinHeap<Request*, CompareRequests> m_timeouts;

  int m_listenSocket;
  int m_epoll;

};
