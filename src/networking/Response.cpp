#include "Response.h"

#include "Request.h"
#include <cstddef>
#include <cstring>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Server.h"

#define PRINT_ERROR(f, l) std::cout << __FILE__  ":" << __LINE__ - l << "\n\t" f "(); // trowed " << errno << "\n\nError: " << std::strerror(errno) << "\n";

// TODO: make it work without that MSG_NOSIGNAL flag
void Response::writeHead(std::string_view key, std::string_view value) const
{

  if (m_request->m_socket == 0) return;

  if (!m_headSent)
  {
    send(m_request->m_socket, "HTTP/1.1 200 OK\r\n", 17, 0);
    m_headSent = true;
  }

  send(m_request->m_socket, key.data(), key.size(), 0);
  send(m_request->m_socket, ": ", 2, 0);
  send(m_request->m_socket, value.data(), value.size(), 0);
  send(m_request->m_socket, "\r\n", 2, 0);

}

static std::size_t toHex(char*& buffer, std::size_t number) 
{

  constexpr std::size_t bufferLength = 5;
  std::size_t i = bufferLength;

  while (number != 0 && i != 0)
  {

    char reminder = number & 15;
    char hex = reminder < 10 ? reminder + 48 : reminder + 55; 

    buffer[--i] = hex;
    number >>= 4;
      
  }

  buffer += i;

  return bufferLength - i;

}

void Response::write(std::string_view buffer) const
{

  write(buffer.data(), buffer.size());

}


void Response::write(const char* buffer, std::size_t size) const
{

  if (m_request->m_socket == 0) return;

  if (size > 0x10000) return; // sending too much, break it up

  char hexBuffer[5];
  char* pHexBuffer = hexBuffer;

  std::size_t length = toHex(pHexBuffer, size);
  
  if (!m_chunkSent)
  {
    send(m_request->m_socket, "\r\n", 2, 0);
    m_chunkSent = true;
  }

  send(m_request->m_socket, pHexBuffer, length, 0); 
  send(m_request->m_socket, "\r\n", 2, 0);
  send(m_request->m_socket, buffer, size, 0);
  send(m_request->m_socket, "\r\n", 2, 0);

}

void Response::writeBody(std::string_view buffer) const
{

  if (!m_contentLengthSent)
  {
    writeHead("Content-Length", std::to_string(buffer.size()));
    m_contentLengthSent = true;
  }
  
  send(m_request->m_socket, "\r\n", 2, 0);
  send(m_request->m_socket, buffer.data(), buffer.size(), 0);

}

void Response::end() const
{

  if (m_chunkSent) send(m_request->m_socket, "0\r\n\r\n", 5, 0);

  m_request->m_updateTimeout(5000);
  m_server->m_timeouts.push(m_request);

}
