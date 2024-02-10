#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace http
{

class Parser
{

public:

  Parser(char* buffer) : m_buffer(buffer), m_unhandledBuffer(buffer) {};

  int parse(size_t bytes);

public:

  std::string_view method;
  std::string_view path;

  struct Header
  { 

    std::string_view key;
    std::string_view value;

  };

  std::vector<Header> headers;

private:

  enum STATE : char
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
    REQUEST_HTTP_MAJOR
  };

  char* m_buffer;
  char* m_unhandledBuffer;

  STATE m_state = REQUEST_METHOD;

};

}
