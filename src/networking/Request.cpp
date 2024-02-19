#include "Request.h"

#include <cerrno>
#include <sys/socket.h>

Request::Request(int socket)
{

  m_socket = socket;
  m_parser = http::Parser(m_buffer.get());

}
// habdle http 1.1 chunks and other http requests.
REQUEST_STATUS Request::m_parse() // make even more states and rename them what are these names
{
   
  if (m_bufferSize >= m_bufferLength) // no way it can be bigger then length
  {

    char byte;
    ssize_t bytes = recv(m_socket, &byte, 1, 0);

    return bytes == 0 ? REQUEST_CLOSE : REQUEST_HTTP_BUFFER_ERROR; // we aint checking if its -1 couse we dont care 

  }

  while (true)
  {
   
    ssize_t bytes = recv(m_socket, m_buffer.get() + m_bufferSize, m_bufferLength - m_bufferSize, 0);

    if (bytes == 0) return REQUEST_CLOSE;
    if (bytes == -1) return errno == EWOULDBLOCK ? REQUEST_INCOMPLETE : REQUEST_ERROR;
    
    m_bufferSize += bytes;
    
    if (m_parsed) return REQUEST_CHUNK_ERROR; // i dont like this

    switch (m_parser.parse(bytes))
    {
    case 0: // EOF REACHED
      m_parsed = true;
      return REQUEST_SUCCESS;
    case 1: // EOF NOT REACHED
      break;
    default: return REQUEST_HTTP_ERROR;
    }

  }

}

std::optional<std::string_view> Request::getHeader(std::string_view header) const
{
  
  for (auto& [ key, value ] : m_parser.headers)
  {

    if (key == header) return value;

  }
  
  return {};

}

std::string_view Request::getPath() const
{

  return m_parser.path;

}
