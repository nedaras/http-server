#include "Request.h"

#include <sys/socket.h>

Request::Request(int socket)
{

  m_socket = socket;
  m_parser = http::Parser(m_buffer.get());

}

REQUEST_STATUS Request::parse() // make even more states
{

  if (m_bufferLength + m_chunkSize > m_bufferSize) return REQUEST_HTTP_BUFFER_ERROR;

  ssize_t bytesRead = recv(m_socket, m_buffer.get() + m_bufferLength, m_chunkSize, 0);
  
  if (bytesRead > 0)
  {

    m_bufferLength += bytesRead;    

    switch (m_parser.parse(bytesRead))
    {
    case 0: return REQUEST_SUCCESS;
    case 1: return REQUEST_INCOMPLETE;
    default: return REQUEST_HTTP_ERROR;
    }

  }

  return bytesRead == 0 ? REQUEST_CLOSE : REQUEST_ERROR;

}

std::optional<std::string_view> Request::getHeader(std::string_view header) const
{
  
  for (auto& [ key, value ] : m_parser.headers)
  {

    if (key == header) return value;

  }
  
  return {};

}
