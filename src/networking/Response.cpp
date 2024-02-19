#include "Response.h"

#include "Request.h"
#include <cstddef>
#include <cstring>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

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

void Response::write(std::string_view buffer) const // send chunked, writeData will send not chunked
{

  if (m_request->m_socket == 0) return;

  if (buffer.size() > 0x10000) return; // sending too much, break it up

  char hexBuffer[5];
  char* pHexBuffer = hexBuffer;

  std::size_t length = toHex(pHexBuffer, buffer.size());
  
  if (!m_chunkSent)
  {
    send(m_request->m_socket, "\r\n", 2, MSG_NOSIGNAL);
    m_chunkSent = true;
  }

  send(m_request->m_socket, pHexBuffer, length, MSG_NOSIGNAL); 
  send(m_request->m_socket, "\r\n", 2, MSG_NOSIGNAL);
  send(m_request->m_socket, buffer.data(), buffer.size(), MSG_NOSIGNAL);
  send(m_request->m_socket, "\r\n", 2, MSG_NOSIGNAL);

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

  if (m_request->m_socket == 0)
  {
    delete m_request;
    return;
  }

  if (m_chunkSent) send(m_request->m_socket, "0\r\n\r\n", 5, 0);

  //epoll_event event {};
  
  //Server* server = static_cast<Server*>(m_server);
  
  //if (epoll_ctl(server->m_epoll, EPOLL_CTL_DEL, m_request->m_socket, &event) == -1) PRINT_ERROR("epoll_ctl", 0);

  //server->m_mutex.lock();
  //server->m_events.pop_back();
  //server->m_mutex.unlock();

  //close(m_request->m_socket);

  //delete m_request;

}
