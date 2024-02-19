#pragma once

#include "Request.h"
#include "Response.h"
#include <functional>
#include <mutex>
#include <queue>
#include <sys/epoll.h>

class Server
{

public:

  Server(const std::function<void(const Request* request, const Response& response)>& m_callback) : m_callback(std::move(m_callback)) {}

  int listen(const char* port);

private:

  void removeRequest(Request* request);

private:

  struct CompareTimeouts
  {

    bool operator()(const Request* lhs, const Request* rhs) const
    {

      return lhs->m_timeout > rhs->m_timeout;

    }

  };

  friend class Response;

  std::mutex m_mutex;

  const std::function<void(const Request* request, const Response& response)> m_callback;

  std::vector<epoll_event> m_events;
  std::priority_queue<Request*, std::vector<Request*>, CompareTimeouts> m_timeouts;

  int m_listenSocket;
  int m_epoll;

};
