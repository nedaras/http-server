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
  
  // ok im the biggest idiot by reading this request by its chunk size,
  // use whole buffers empty size, then if we extend to our buffer size it means request is too big
  // so we have to close it down, so we will read till EWOULDBLOCK or if we overflow buffer
  // if so we will break recv and send some http response otherwise if EWOULDBLOCK we will just parse the buffer
  // and send correct status, like request incomplete or mb there was error in parsing

  ssize_t bytesRead = recv(m_socket, m_buffer.get() + m_bufferLength, m_chunkSize, 0); // ok but we need to recv to know if its closed
                                                                                       // we can add one byte for like closed if whole
                                                                                       // buffer is taken
  if (bytesRead == 0) return REQUEST_CLOSE;
  if (bytesRead == -1) return REQUEST_ERROR;

  if (m_parsed) return REQUEST_CHUNK_ERROR;

  m_bufferLength += bytesRead;    

  switch (m_parser.parse(bytesRead))
  {
  case 0:
    m_parsed = true;
    return REQUEST_SUCCESS;
  case 1: return REQUEST_INCOMPLETE;
  default: return REQUEST_HTTP_ERROR;
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
