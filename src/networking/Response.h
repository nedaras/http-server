#pragma once

#include <string_view>

class Response
{

public:

  Response(int socket, int epoll) : m_socket(socket), m_epoll(epoll) {};

  void writeHead(std::string_view key, std::string_view value) const;

  void write(std::string_view buffer) const;

  void writeData(std::string_view buffer) const;

  void end() const;

private:

  friend class Server;

  int m_socket;
  int m_epoll;

  mutable bool m_headSent = false;
  mutable bool m_chunkSent = false;

};

