#include "parser.h"

static std::string_view token(char*& buffer, char token, size_t length)
{

  char* start = buffer;

  while (buffer != &buffer[length] && *buffer++ != token); // handle incomplete toekns if length reached
  
  return std::string_view(start, buffer - start - 1); // we're not sure that we reached space we can get out of bounds

}

int http::Parser::parse(size_t bytes) // we're parsing without state now
{

  method = token(m_buffer, ' ', bytes);
  path = token(m_buffer, ' ', bytes);

  return 0;

}
