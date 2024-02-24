#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
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

enum REQUEST_STATE : char
{
  REQUEST_WAITING_FOR_DATA,
  REQUEST_READING_HTTP,
  REQUEST_READING_BODY,
  REQUESR_READING_CHUNKS
};

enum REQUEST_EVENTS : std::uint32_t
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

  constexpr std::string_view getPath() const
  {
    return m_parser.path;
  }

public:

  std::string body;

private:

  ParserResponse m_parse();

  void m_updateTimeout(unsigned long milliseconds);

  constexpr bool m_firstRequest()
  {
    return m_state == REQUEST_WAITING_FOR_DATA;
  }

private:

  friend class Server;
  friend class Response;

  int m_socket;

  std::chrono::milliseconds m_timeout; 

  http::Parser m_parser;

  REQUEST_STATE m_state = REQUEST_WAITING_FOR_DATA;

  constexpr static std::size_t m_bufferLength = 8 * 1024;

  std::unique_ptr<char[]> m_buffer = std::make_unique<char[]>(m_bufferLength); // we can add one bit
  std::size_t m_bufferSize = 0;

};
