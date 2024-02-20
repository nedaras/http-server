#pragma once

#include "Request.h"
#include "Response.h"
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

  struct Timeout
  {
    Request* request;
    std::chrono::milliseconds timeout;
  };

  friend class Response;

  std::mutex m_mutex;

  const std::function<void(const Request* request, const Response& response)> m_callback;

  std::vector<epoll_event> m_events;
  std::queue<Timeout> m_timeouts;

  int m_listenSocket;
  int m_epoll;

};
