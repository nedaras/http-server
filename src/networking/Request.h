#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include "../http/parser.h"

enum RESPONSE_STATUS : char
{
  REQUEST_SUCCESS,
  REQUEST_TIMEOUT,
  REQUEST_CLOSE
};

class Request
{

public:

  Request(int socket);
  Request() {};

  std::optional<std::string_view> getHeader(std::string_view header) const;

  int parse();

  operator RESPONSE_STATUS() const; 

public:

private:

  int m_socket;
  http::Parser m_parser;
  
  constexpr static std::size_t m_bufferSize = 8 * 1024;
  constexpr static std::size_t m_chunkSize = 1024;

  std::shared_ptr<char[]> m_buffer = std::make_unique<char[]>(m_bufferSize);
  std::size_t m_bufferLength = 0; 

  RESPONSE_STATUS m_status = REQUEST_SUCCESS;

};
