#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace http
{

struct Header
{ 

  std::string_view key;
  std::string_view value;

};


class Parser
{

public:

  Parser(char* buffer) : m_buffer(buffer), m_unhandledBuffer(buffer) {};
  Parser() : m_buffer(nullptr), m_unhandledBuffer(nullptr) {};

  int parse(std::size_t bytes);
  int parseChunk(char* buffer, std::size_t bytes);

public:

  std::string_view method;
  std::string_view path;
 // we net to bench mark but mb map would be faster, it should be for sure if the headers would be very big, but it aint
  std::vector<Header> headers;

  std::size_t bytesRead = 0;
  std::size_t bodyLength = 0;

  bool chunked = false;

private:

  int m_newHeader(Header& header);

private:

  enum STATE
  {
    REQUEST_METHOD,
    REQUEST_PATH_BEGIN,
    REQUEST_PATH,
    REQUEST_H,
    REQUEST_HT,
    REQUEST_HTT,
    REQUEST_HTTP,
    REQUEST_HTTP_DASH,
    REQUEST_HTTP_MINOR,
    REQUEST_HTTP_DOT,
    REQUEST_HTTP_MAJOR,
    REQUEST_HTTP_ALMOST_END,
    REQUEST_HTTP_END,
    REQUEST_HEADER_KEY_BEGIN,
    REQUEST_HEADER_KEY,
    REQUEST_HEADER_KEY_END,
    REQUEST_HEADER_VALUE_BEGIN, 
    REQUEST_HEADER_VALUE,
    REQUEST_HEADER_END,
    REQUEST_EOF
  };

  enum CHUNK_STATE
  {
    REQUEST_CHUNK_SIZE,
    REQUEST_CHUNK_BEGIN,
    REQUEST_CHUNK_BODY,
    REQUEST_CHUNK_END_CR,
    REQUEST_CHUNK_END_LF
  };

  char* m_buffer;
  char* m_unhandledBuffer;

  Header m_header;

  std::uint32_t m_chunkSize = 0;
  std::size_t m_chunkSizeReceived = 0;

  STATE m_state = REQUEST_METHOD;
  CHUNK_STATE m_chunkState = REQUEST_CHUNK_SIZE;

};

}
