#include "Response.h"
#include <cstddef>
#include <sys/socket.h>
#include <unistd.h>

void Response::writeHead(std::string_view key, std::string_view value) const
{

  if (!m_headSent)
  {
    send(m_socket, "HTTP/1.1 200 OK\r\n", 16, 0);
    m_headSent = true;
  }

  send(m_socket, key.data(), key.size(), 0);
  send(m_socket, ": ", 2, 0);
  send(m_socket, value.data(), value.size(), 0);
  send(m_socket, "\r\n", 2, 0);

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

void Response::write(std::string_view buffer) const // send chunked, writeData will send not chunked
{

  if (buffer.size() > 0x10000) return; // sending too much, break it up

  char hexBuffer[5]; // use std array
  char* pHexBuffer = hexBuffer;

  std::size_t length = toHex(pHexBuffer, buffer.size());
  
  if (!m_chunkSent)
  {
    send(m_socket, "\r\n", 2, 0); // check if first chunk then send it
    m_chunkSent = true;
  }

  send(m_socket, pHexBuffer, length, 0);
  send(m_socket, "\r\n", 2, 0);
  send(m_socket, buffer.data(), buffer.size(), 0);
  send(m_socket, "\r\n", 2, 0);

}

void Response::end() const
{

  send(m_socket, "0\r\n\r\n", 5, 0); // we will handle this diffrently
  close(m_socket); // when implementing keep alive we will need todo some good shit  

}
