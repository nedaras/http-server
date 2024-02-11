#pragma once

#include <memory>
#include <string_view>
#include <vector>
#include "../http/parser.h"

class Request
{

public:

  Request(int socket);

public:

  std::string_view method;
  std::string_view path;

  std::vector<http::Header> headers;

private:

  constexpr static std::size_t m_bufferSize = 8 * 1024;
  constexpr static std::size_t m_chunkSize = 1024;

  std::unique_ptr<char[]> m_buffer = std::make_unique<char[]>(m_bufferSize);
  std::size_t m_bufferLength = 0; 

};
