#pragma once

#include "Request.h"
#include <string_view>

class Server;
class Response
{

public:

  Response(Request* request, Server* server) : m_request(request), m_server(server) {};

  void writeHead(std::string_view key, std::string_view value) const;

  void write(std::string_view buffer) const;

  void writeData(std::string_view buffer) const;

  void end() const;

private:

  friend class Server;

  Request* m_request;
  Server* m_server;

  mutable bool m_headSent = false;
  mutable bool m_chunkSent = false;

};

