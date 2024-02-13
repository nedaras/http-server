#pragma once

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

  std::optional<std::string_view> getHeader(std::string_view header) const;

  operator RESPONSE_STATUS() const; 

public:

  std::string_view method;
  std::string_view path;

private:

  std::vector<http::Header> m_headers;

  constexpr static std::size_t m_bufferSize = 8 * 1024;
  constexpr static std::size_t m_chunkSize = 1024;

  std::unique_ptr<char[]> m_buffer = std::make_unique<char[]>(m_bufferSize);
  std::size_t m_bufferLength = 0; 

  RESPONSE_STATUS m_status = REQUEST_SUCCESS;

};
