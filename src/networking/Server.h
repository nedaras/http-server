#pragma once

#include "Request.h"
#include <functional>

class Server
{

public:

  Server(const std::function<void(Request& request)>& m_callback) : m_callback(m_callback) {}

  int listen(const char* port);

private:

  const std::function<void(Request& request)>& m_callback;
  int m_listenSocket;

};

