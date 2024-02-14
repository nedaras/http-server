#include "Request.h"

#include <sys/socket.h>

Request::Request(int socket)
{

  m_socket = socket;
  m_parser = http::Parser(m_buffer.get());

}

int Request::parse()
{

  if (m_bufferLength + m_chunkSize > m_bufferSize) return -1;

  ssize_t bytesRead = recv(m_socket, m_buffer.get() + m_bufferLength, m_chunkSize, 0);
            
  if (bytesRead == -1)
  {
    m_status = REQUEST_TIMEOUT;
    return -1;
  }

  if (bytesRead == 0)
  {
    m_status = REQUEST_CLOSE; 
    return -1;
  }

  m_bufferLength += bytesRead;    
  
  return m_parser.parse(bytesRead);

}

std::optional<std::string_view> Request::getHeader(std::string_view header) const
{
  
  for (auto& [ key, value ] : m_parser.headers)
  {

    if (key == header) return value;

  }
  
  return {};

}
