#pragma once

#include <functional>

class Server
{

public:

  Server(std::function<void()> m_callback) : m_callback(m_callback) {}

  int listen(const char* port);

private:

  std::function<void()> m_callback;
  int m_listenSocket;

};

