#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include "../http/parser.h"

enum REQUEST_STATUS
{
  REQUEST_SUCCESS,
  REQUEST_INCOMPLETE,
  REQUEST_ERROR,
  REQUEST_CLOSE,
  REQUEST_HTTP_ERROR,
  REQUEST_HTTP_BUFFER_ERROR,
  REQUEST_CHUNK_ERROR 
};

enum REQUEST_EVENTS : char
{
  END,
  DATA
};

struct ParserResponse
{
  REQUEST_STATUS status;
  bool newRequest;
};

class Response;
class Request
{

public:

  Request(int socket);
  Request(const Request& other) = delete;

  Request& operator=(const Request& other) = delete;

  std::optional<std::string_view> getHeader(std::string_view header) const;

public:

  std::string_view method;
  std::string_view path;

private:

  ParserResponse m_parse();

  void m_updateTimeout(unsigned long milliseconds);

private:

  friend class Server;
  friend class Response;

  int m_socket;

  std::chrono::milliseconds m_timeout; 

  http::Parser m_parser;

  bool m_firstRequest = true;

  constexpr static std::size_t m_bufferLength = 8 * 1024;

  std::unique_ptr<char[]> m_buffer = std::make_unique<char[]>(m_bufferLength); // we can add one bit
  std::size_t m_bufferSize = 0; 

};
