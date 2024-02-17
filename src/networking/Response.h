#pragma once

#include "Request.h"
#include <string_view>

class Response
{

public:

  Response(int socket, void* server) : m_socket(socket), m_server(server) {}; // i dont like that void ptr

  void writeHead(std::string_view key, std::string_view value) const;

  void write(std::string_view buffer) const;

  void writeData(std::string_view buffer) const;

  void end(Request* request) const;

private:

  friend class Server;

  int m_socket;
  void* m_server;

  mutable bool m_headSent = false;
  mutable bool m_chunkSent = false;

};

