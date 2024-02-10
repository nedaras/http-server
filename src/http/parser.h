#pragma once

#include <cstddef>
#include <string_view>

namespace http
{

class Parser
{

public:

  Parser(char* buffer) : m_buffer(buffer) {};

  int parse(size_t bytes);

public:

  std::string_view method;
  std::string_view path;

private:

  char* m_buffer;
  size_t m_bufferLength = 0; 

};

}
