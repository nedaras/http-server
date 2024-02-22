#pragma once

#include <chrono>
#include <cstddef>
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

struct ParserResponse
{
  REQUEST_STATUS status;
  bool newRequest;
};

class Request
{

public:

  Request(int socket);
  Request(const Request& other) = delete;

  Request& operator=(const Request& other) = delete;

  std::optional<std::string_view> getHeader(std::string_view header) const;

  std::string_view getPath() const;

  void updateTimeout(unsigned long milliseconds); // this should not be public

private:

  ParserResponse m_parse();

private:

  friend class Server;
  friend class Response;

  int m_socket;

  std::chrono::milliseconds m_timeout; 

  http::Parser m_parser;
 
  std::string_view m_method;
  std::string_view m_path;

  bool m_firstRequest = true;

  constexpr static std::size_t m_bufferLength = 8 * 1024;

  std::unique_ptr<char[]> m_buffer = std::make_unique<char[]>(m_bufferLength); // we can add one bit
  std::size_t m_bufferSize = 0; 

};
