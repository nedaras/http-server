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

  Server(const std::function<void(const Request* request, const Response& response)>& callback) : m_callback(callback) {}

  int listen(const char* port);

private:

  friend class Response;

  const std::function<void(const Request* request, const Response& response)> m_callback;

  struct CompareRequests
  {

    bool operator()(Request* left, Request* right)
    {
      return left->m_timeout > right->m_timeout;
    }

  };

  std::vector<epoll_event> m_events;
  MinHeap<Request*, CompareRequests> m_timeouts;

  int m_listenSocket;
  int m_epoll;

};
