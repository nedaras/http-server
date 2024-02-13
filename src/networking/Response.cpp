#include "Response.h"
#include <cstddef>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

void Response::writeHead(std::string_view key, std::string_view value) const
{
  // this aint that what we ant
  std::string header;

  if (!m_headSent) // we need to check if we can jus call send like a stream it would be zero allocations then
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

static std::size_t toHex(char*& buffer, std::size_t number) 
{

  constexpr std::size_t bufferLength = 5;
  std::size_t i = bufferLength;

  while (number != 0 && number != 0)
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

  // turn buffer.size to hex

  if (buffer.size() > 0x10000) return; // sending too much, break it up

  char hexBuffer[5]; // use std array
  char* pHexBuffer = hexBuffer;

  std::size_t length = toHex(pHexBuffer, buffer.size());
  
  send(m_socket, "\r\n", 2, 0);

  std::string chunk;
  chunk.reserve(length + 2 + buffer.size() + 2);

  chunk.append(pHexBuffer, length);
  chunk.append("\r\n");
  chunk.append(buffer);
  chunk.append("\r\n"),
  
  send(m_socket, chunk.data(), chunk.size(), 0);

}

void Response::end() const
{

  send(m_socket, "0\r\n\r\n", 5, 0); // we will handle this diffrently
  close(m_socket); // when implementing keep alive we will need todo some good shit  

}
