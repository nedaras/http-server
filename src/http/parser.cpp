#include "parser.h"

// yes this code is the worst and can be exploted so simply
// TODO: make return values an error return values
// TODO: make some standards like what chars can be used idk utf-8 if im not bored
// chunked encoding to
int http::Parser::parse(std::size_t bytes) // add some token chars arays
{

  char* end = &m_buffer[bytes];
 
  while (m_buffer != end)
  {

    switch (m_state)
    {
    case REQUEST_METHOD:
      if (*m_buffer != ' ') break;
      method = std::string_view(m_unhandledBuffer, m_buffer - m_unhandledBuffer);
      m_state = REQUEST_PATH_BEGIN;
      break;
    case REQUEST_PATH_BEGIN:
      if (*m_buffer == ' ') break;
      m_state = REQUEST_PATH;
      m_unhandledBuffer = m_buffer;
      break;
    case REQUEST_PATH:
      if (*m_buffer != ' ') break;
      path = std::string_view(m_unhandledBuffer, m_buffer - m_unhandledBuffer);
      m_state = REQUEST_H;
      break;
    case REQUEST_H:
      if (*m_buffer != 'H') return -1;
      m_state = REQUEST_HT;
      break;
    case REQUEST_HT: 
      if (*m_buffer != 'T') return -1;
      m_state = REQUEST_HTT;
      break;
    case REQUEST_HTT: 
      if (*m_buffer != 'T') return -1;
      m_state = REQUEST_HTTP;
      break;
    case REQUEST_HTTP:
      if (*m_buffer != 'P') return -1;
      m_state = REQUEST_HTTP_DASH; 
      break;
    case REQUEST_HTTP_DASH:
      if (*m_buffer != '/') return -1;
      m_state = REQUEST_HTTP_MINOR; 
      break;
    case REQUEST_HTTP_MINOR:
      if (*m_buffer != '1') return -1;
      m_state = REQUEST_HTTP_DOT; 
      break;
    case REQUEST_HTTP_DOT:
      if (*m_buffer != '.') return -1;
      m_state = REQUEST_HTTP_MAJOR; 
      break;
    case REQUEST_HTTP_MAJOR:
      if (*m_buffer != '1') return -1;
      m_state = REQUEST_HTTP_ALMOST_END; 
      break;
    case REQUEST_HTTP_ALMOST_END:
      if (*m_buffer != '\r') return -1;
      m_state = REQUEST_HTTP_END;
      break;
    case REQUEST_HTTP_END:
      if (*m_buffer != '\n') return -1;
      m_state = REQUEST_HEADER_KEY_BEGIN;
      break;
    case REQUEST_HEADER_KEY_BEGIN:
      if (*m_buffer == '\r')
      {
        m_state = REQUEST_EOF;
        break;
      }
      m_state = REQUEST_HEADER_KEY;
      m_unhandledBuffer = m_buffer;
      break;
    case REQUEST_HEADER_KEY:
      if (*m_buffer != ':') break;
      m_header.key = std::string_view(m_unhandledBuffer, m_buffer - m_unhandledBuffer);
      m_state = REQUEST_HEADER_VALUE_BEGIN;
      break;
    case REQUEST_HEADER_VALUE_BEGIN:
      if (*m_buffer == ' ') break;
      m_state = REQUEST_HEADER_VALUE;
      m_unhandledBuffer = m_buffer;
      break;
    case REQUEST_HEADER_VALUE:
      if (*m_buffer != '\n') break;
      if (*(m_buffer - 1) != '\r') break;
      m_header.value = std::string_view(m_unhandledBuffer, m_buffer - m_unhandledBuffer - 1);
      headers.push_back(m_header);
      m_state = REQUEST_HEADER_KEY_BEGIN; 
      break;
    case REQUEST_EOF:
      if (*m_buffer == '\n') return 0; // it prob should just throw error 
      m_state = REQUEST_HEADER_KEY;
      m_unhandledBuffer = m_buffer - 1;
      break;
    }
  
    m_buffer++;

  }

  return 1;

}
