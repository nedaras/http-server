#pragma once

#include "Request.h"
#include "Response.h"
#include <functional>
#include <mutex>
#include <sys/epoll.h>

class Server
{

public:

  Server(const std::function<void(const Request* request, Response& response)>& m_callback) : m_callback(std::move(m_callback)) {}

  int listen(const char* port);

private:

  void m_makeResponse(Request* request);

private:

  std::mutex m_mutex;

  const std::function<void(const Request* request, Response& response)> m_callback;
  std::vector<epoll_event> m_events;

  int m_listenSocket;
  int m_epoll;

};

