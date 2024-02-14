#pragma once

#include <string_view>

class Response
{

public:

  Response(int socket) : m_socket(socket) {};

  void writeHead(std::string_view key, std::string_view value) const;

  void write(std::string_view buffer) const;

  void writeData(std::string_view buffer) const;

  void end() const;

private:
  // make it one bit 
  mutable bool m_headSent = false;
  mutable bool m_chunkSent = false;

  int m_socket;

};

