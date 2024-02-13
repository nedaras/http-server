#include "Response.h"
#include <string>
#include <sys/socket.h>

void Response::writeHead(std::string_view key, std::string_view value) const
{
  // this aint that what we ant
  std::string header;

  if (!m_headSent)
  {

    header.reserve(16 + key.size() + 2 + value.size() + 2);
    header.append("HTTP/1.1 200 OK\r\n");

    m_headSent = true;

  }
  else header.reserve(key.size() + 2 + value.size() + 2);

  header.append(key);
  header.append(": ");
  header.append(value);
  header.append("\r\n"); 

  send(m_socket, header.c_str(), header.size(), 0);

}


void Response::write(std::string_view buffer) const // send chunked, writeData will send not chunked
{

  send(m_socket, "\r\n", 2, 0);
  send(m_socket, buffer.data(), buffer.size(), 0);  
  send(m_socket, "\r\n\r\n", 4, 0);

  m_headSent = true;

}
