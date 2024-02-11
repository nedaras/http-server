#include "Request.h"

#include <sys/socket.h>

Request::Request(int socket)
{

  http::Parser parser(m_buffer.get());

  while (m_bufferSize > m_bufferLength + m_chunkSize)
  {

    ssize_t bytesRead = recv(socket, m_buffer.get(), m_chunkSize, 0);
  
    if (bytesRead == -1)
    {
      //err
    }

    if (bytesRead == 0)
    {
      //close
    }
      
    m_bufferLength += bytesRead;    
    
    if (parser.parse(bytesRead) == 0) break; // include like dead state

  }

  method = parser.method;
  path = parser.path;

  headers = std::move(parser.headers);

}

