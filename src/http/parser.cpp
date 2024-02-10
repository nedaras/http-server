#include "parser.h"
#include <iostream>
#include <string_view>

// yes this code is the worst and can be exploted so simply

static std::string_view token(char*& buffer, char token)
{

  char* start = buffer;

  while (*buffer++ != token);
  
  return std::string_view(start, buffer - start - 1); 

}

int http::Parser::parse(size_t bytes)
{

  char* end = &m_buffer[bytes];
  
  while (m_buffer != end)
  {

    size_t length = m_buffer - m_unhandledBuffer;

    std::cout << *m_buffer;

    switch (m_state)
    {
    case REQUEST_METHOD:

      if (*m_buffer != ' ') break;

      method = std::string_view(m_unhandledBuffer, length);
      m_state = REQUEST_PATH_BEGIN;

      break;
    case REQUEST_PATH_BEGIN:

      if (*m_buffer == ' ') break;

      m_state = REQUEST_PATH;
      m_unhandledBuffer = m_buffer;

      break;
    case REQUEST_PATH:

      if (*m_buffer != ' ') break;

      path = std::string_view(m_unhandledBuffer, length);
      m_state = REQUEST_H;
      
      break;
    default:

      if (*m_buffer == '\n') return 0;

      break;
    }
  
    m_buffer++;

  }

  return 1;

  method = token(m_buffer, ' ');
  path = token(m_buffer, ' ');

  if (*m_buffer++ != 'H') return 1;
  if (*m_buffer++ != 'T') return 1;
  if (*m_buffer++ != 'T') return 1;
  if (*m_buffer++ != 'P') return 1;
  if (*m_buffer++ != '/') return 1;
  if (*m_buffer++ != '1') return 1;
  if (*m_buffer++ != '.') return 1;
  if (*m_buffer++ != '1') return 1;
  if (*m_buffer++ != '\r') return 1;
  if (*m_buffer++ != '\n') return 1;
  
  while (true)
  {

    if (*m_buffer == '\r') break;

    std::string_view key = token(m_buffer, ':');
    m_buffer++;
    std::string_view value = token(m_buffer, '\n');

    headers.push_back({ key, std::string_view(value.data(), value.size() - 1) });

  }

  return 0;

}
