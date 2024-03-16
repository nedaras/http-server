#pragma once

#include "Request.h"
#include "../minheap/MinHeap.h"
#include <functional>
#include <sys/epoll.h>

using RequestHandler = std::function<void(const Request* request)>;

class Server
{

public:

  Server(const RequestHandler& callback) : m_callback(std::move(callback)) {}

  int listen(const char* port);

private:

  int removeRequest(const Request* request);

private:

  friend class Request;

  struct CompareRequests
  {

    bool operator()(const Request* left, const Request* right)
    {
      return left->m_timeout > right->m_timeout;
    }

  };

  RequestHandler m_callback;

  std::vector<epoll_event> m_events;
  MinHeap<const Request*, CompareRequests> m_timeouts;

  int m_listenSocket;
  int m_epoll;

};
