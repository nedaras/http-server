#pragma once

#include <string_view>

class Response
{

public:

  Response(int socket) : m_socket(socket) {};

  void writeHead(std::string_view key, std::string_view value) const;

  void write(std::string_view buffer) const;

private:
  
  mutable bool m_headSent = false;
  int m_socket;

};

