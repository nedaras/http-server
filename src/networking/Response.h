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
  void write(const char* buffer, std::size_t size) const;

  void writeBody(std::string_view buffer) const;
  void writeBody(const char* buffer, std::size_t size) const;

  void end() const;

private:

  constexpr Request::ResponseData& m_responseData() const
  {
    return m_request->m_responseData;
  }

private:

  friend class Server;

  Request* m_request;
  Server* m_server;

};

