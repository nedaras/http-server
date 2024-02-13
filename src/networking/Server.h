#pragma once

#include "Request.h"
#include "Response.h"
#include <functional>

class Server
{

public:

  Server(const std::function<void(Request& request, Response& response)>& m_callback) : m_callback(m_callback) {}

  int listen(const char* port);

private:

  const std::function<void(Request& request, Response& response)>& m_callback;
  int m_listenSocket;

};

