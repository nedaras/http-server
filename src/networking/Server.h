#pragma once

#include "Request.h"
#include "Response.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <sys/epoll.h>

class Server
{

public:

  Server(const std::function<void(Request* request, Response& response)>& m_callback) : m_callback(std::move(m_callback)) {}

  int listen(const char* port);

  std::atomic<int> allocated_requests = 0;

private:

  friend class Response;

  std::mutex m_mutex;

  const std::function<void(Request* request, Response& response)> m_callback;
  std::vector<epoll_event> m_events;

  int m_listenSocket;
  int m_epoll;

};
