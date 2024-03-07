#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>
#include "../http/parser.h"

enum REQUEST_STATUS
{
  REQUEST_HTTP_COMPLETE,
  REQUEST_INCOMPLETE,
  REQUEST_ERROR,
  REQUEST_CLOSE,
  REQUEST_HTTP_ERROR,
  REQUEST_HTTP_BUFFER_ERROR,
  REQUEST_CHUNK_ERROR,
  REQUEST_CHUNK_COMPLETE,
  REQUEST_CHUNK_END
};

enum REQUEST_STATE : std::uint8_t
{
  REQUEST_WAITING_FOR_DATA,
  REQUEST_READING_HTTP,
  REQUEST_READING_BODY,
  REQUEST_READING_CHUNK_SIZE,
  REQUEST_READING_CHUNK,
  REQUEST_READING_CHUNK_EOF
};

enum REQUEST_EVENTS : std::uint8_t
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

  constexpr std::string_view getMethod() const
  {
    return m_parser.method;
  }

  using EventFunction = std::function<void(std::string_view data)>;
  void on(REQUEST_EVENTS event, const EventFunction& function) const;

public:

  std::string body;

private:

  ParserResponse m_parse();

  void m_updateTimeout(unsigned long milliseconds);

  std::tuple<REQUEST_STATUS, ssize_t> m_safeRecv(char* buffer, std::size_t bufferSize, std::size_t bufferCapacity);

  void m_callEvent(REQUEST_EVENTS event, std::string_view data) const;

  constexpr bool m_firstRequest()
  {
    return m_state == REQUEST_WAITING_FOR_DATA;
  }

private:

  friend class Server;
  friend class Response;

  int m_socket;
  std::chrono::milliseconds m_timeout;

  mutable std::unique_ptr<EventFunction> m_events[2];

  http::Parser m_parser;

  std::string m_chunk;
  std::size_t m_chunkSentSize;

  REQUEST_STATE m_state = REQUEST_WAITING_FOR_DATA;

  constexpr static std::size_t m_bufferLength = 8 * 1024;

  std::unique_ptr<char[]> m_buffer = std::make_unique<char[]>(m_bufferLength); // we can add one bit
  std::size_t m_bufferSize = 0;

  struct ResponseData
  {
    bool keepAlive : 1;
    bool headSent : 1;
    bool chunkSent : 1;
    bool contentLengthSent : 1;
  };

  ResponseData m_responseData;

};
