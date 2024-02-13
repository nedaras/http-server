#include "Request.h"

#include <sys/socket.h>

Request::Request(int socket)
{

  http::Parser parser(m_buffer.get());

  while (m_bufferSize > m_bufferLength + m_chunkSize)
  {

    ssize_t bytesRead = recv(socket, m_buffer.get() + m_bufferLength, m_chunkSize, 0);
  
    if (bytesRead == -1)
    {
      m_status = REQUEST_TIMEOUT;
      return;
    }

    if (bytesRead == 0)
    {
      m_status = REQUEST_CLOSE; 
      return;
    }
      
    m_bufferLength += bytesRead;    
    
    if (parser.parse(bytesRead) == 0) break; // include like dead state, btw handle errors

  }

  method = parser.method;
  path = parser.path;

  m_headers = std::move(parser.headers);

}

std::optional<std::string_view> Request::getHeader(std::string_view header) const
{
  
  for (auto& [ key, value ] : m_headers)
  {

    if (key == header) return value;

  }
  
  return {};

}

Request::operator RESPONSE_STATUS() const
{
  
  return m_status;

}
