#include "Response.h"

#include "Request.h"
#include "Server.h"
#include <cstddef>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

// TODO: some err handling for invalid sockets
void Response::writeHead(std::string_view key, std::string_view value) const
{

  if (!m_headSent)
  {
    send(m_socket, "HTTP/1.1 200 OK\r\n", 16, MSG_NOSIGNAL);
    m_headSent = true;
  }

  send(m_socket, key.data(), key.size(), MSG_NOSIGNAL);
  send(m_socket, ": ", 2, MSG_NOSIGNAL);
  send(m_socket, value.data(), value.size(), MSG_NOSIGNAL);
  send(m_socket, "\r\n", 2, MSG_NOSIGNAL);

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

  char hexBuffer[5];
  char* pHexBuffer = hexBuffer;

  std::size_t length = toHex(pHexBuffer, buffer.size());
  
  if (!m_chunkSent)
  {
    send(m_socket, "\r\n", 2, MSG_NOSIGNAL);
    m_chunkSent = true;
  }

  send(m_socket, pHexBuffer, length, MSG_NOSIGNAL);
  send(m_socket, "\r\n", 2, MSG_NOSIGNAL);
  send(m_socket, buffer.data(), buffer.size(), MSG_NOSIGNAL);
  send(m_socket, "\r\n", 2, MSG_NOSIGNAL);

}

void Response::end(Request* request) const
{

  send(m_socket, "0\r\n\r\n", 5, MSG_NOSIGNAL);

  epoll_event event {};
  
  Server* server = static_cast<Server*>(m_server);

  if (epoll_ctl(server->m_epoll, EPOLL_CTL_DEL, m_socket, &event) == -1)
  {

    std::cout << "err in Response\n";

  }

  server->m_mutex.lock();
  server->m_events.pop_back();
  server->m_mutex.unlock();

  close(m_socket);

  delete request; // fuck
  server->allocated_requests--;

}
