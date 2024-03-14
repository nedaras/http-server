#pragma once

#include "Request.h"
#include "../minheap/MinHeap.h"
#include <functional>
#include <sys/epoll.h>

class Server
{

public:

  Server(const std::function<void(const Request* request)>& callback) : m_callback(std::move(callback)) {}

  int listen(const char* port);

private:

  friend class Request;

  const std::function<void(const Request* request)> m_callback;

  struct CompareRequests
  {

    bool operator()(const Request* left, const Request* right)
    {
      return left->m_timeout > right->m_timeout;
    }

  };

  std::vector<epoll_event> m_events;
  MinHeap<const Request*, CompareRequests> m_timeouts;

  int m_listenSocket;
  int m_epoll;

};
