#include "Server.h"
  request->setHead("Content-Length", "10");

#include "./Request.h"
#include <chrono>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define PRINT_ERRNO(f, l) std::cout << __FILE__  ":" << __LINE__ - l << "\n\t" f "(); // trowed " << errno << "\n\nError: " << std::strerror(errno) << "\n";
#define PRINT_ERROR(f, e, l) std::cout << __FILE__  ":" << __LINE__ - l << "\n\t" f "(); // trowed " << errno << "\n\nError: " << std::strerror(errno) << "\n";

static int setNonBlocking(int socket)
{

  int flags = fcntl(socket, F_GETFL, 0);
  return flags ? fcntl(socket, F_SETFL, flags | O_NONBLOCK) : -1;

}

int Server::listen(const char* port) 
{
  
  addrinfo params = {}; 
  addrinfo* result = nullptr;

  params.ai_family = AF_INET;
  params.ai_socktype = SOCK_STREAM;
  params.ai_protocol = IPPROTO_TCP;
  params.ai_flags = AI_PASSIVE;

  if (getaddrinfo(nullptr, port, &params, &result))
  {
    PRINT_ERROR("getaddrinfo", "invalid port", 2);
    return 1;
  }

  m_listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

  if (m_listenSocket == -1)
  {
    PRINT_ERRNO("socket", 4);
    freeaddrinfo(result);
    return 1;
  }

  if (setNonBlocking(m_listenSocket) == -1)
  {
    PRINT_ERRNO("setNonBlocking", 2);
    freeaddrinfo(result);
    close(m_listenSocket);
    return 1;
  }

  if (bind(m_listenSocket, result->ai_addr, result->ai_addrlen) == -1)
  {
    PRINT_ERRNO("bind", 2);
    freeaddrinfo(result);
    close(m_listenSocket);
    return 1;
  }

  freeaddrinfo(result);

  if (::listen(m_listenSocket, SOMAXCONN) == -1)
  {
    PRINT_ERRNO("listen", 2);
    close(m_listenSocket);
    return 1;
  }

  m_epoll = epoll_create1(0);

  if (m_epoll == -1)
  {
    PRINT_ERRNO("epoll_create1", 4);
    close(m_listenSocket);
    return 1;
  }
  
  {

    epoll_event event;

    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = &m_listenSocket;

    if (epoll_ctl(m_epoll, EPOLL_CTL_ADD, m_listenSocket, &event) == -1)
    {
      PRINT_ERRNO("epoll_ctl", 2);
      close(m_listenSocket);
      return 1;
    }

    m_events.push_back({});
  
  }

  while (true)
  {

    std::chrono::milliseconds::rep timeout = -1;

    while (!m_timeouts.empty())
    {

      const Request* request = m_timeouts.top();
      std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

      if (now >= request->m_timeout)
      {

        if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, nullptr) == -1) PRINT_ERRNO("epoll_ctl", 0);

        m_timeouts.pop();
        m_events.pop_back();

        close(request->m_socket);

        delete request;

        continue;

      }
  
      timeout = (request->m_timeout - now).count();
      break;

    }

    epoll_event* events = m_events.data();
    std::size_t eventSize = m_events.size();

    // imagine if event size was 2,147,483,647 + 1, so it will make event size equal to 0, wthat should we do.
    int epolls = epoll_wait(m_epoll, events, static_cast<int>(eventSize), static_cast<int>(timeout));

    if (epolls == 0)
    {

      while (!m_timeouts.empty())
      {

        const Request* request = m_timeouts.top();
        std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

        if (now >= request->m_timeout)
        {

          if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, nullptr) == -1) PRINT_ERRNO("epoll_ctl", 0);

          m_timeouts.pop();
          m_events.pop_back();

          close(request->m_socket);

          delete request;

          continue;

        }

        break;

      }

      continue;

    }

    for (int i = 0; i < epolls; i++)
    {
  
      if (m_events[i].events & EPOLLERR || m_events[i].events & EPOLLHUP)
      {

        if (m_events[i].data.ptr == &m_listenSocket) continue;

        Request* request = static_cast<Request*>(m_events[i].data.ptr);

        if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, nullptr) == -1) PRINT_ERRNO("epoll_ctl", 0);

        m_timeouts.erase(request);
        m_events.pop_back();

        close(request->m_socket);

        delete request;

        break;

      }

      if (!(m_events[i].events & EPOLLIN)) continue;
      if (m_events[i].data.ptr == &m_listenSocket)
      {

        while (true)
        {

          int clientSocket = accept(m_listenSocket, nullptr, nullptr); // but these addresses in request

          if (clientSocket == -1)
          {
            if (errno != EWOULDBLOCK) PRINT_ERRNO("accept", 4);
            break;
          }

          if (setNonBlocking(clientSocket) == -1)
          {
            PRINT_ERRNO("setNonBlocking", 2);
            close(clientSocket);
            break;
          }
          
          Request* request = new Request(clientSocket, this);

          if (request == nullptr)
          {
            std::cout << "nullptr request\n";
            close(clientSocket);
            break;
          }

          epoll_event event;

          event.events = EPOLLIN | EPOLLET; 
          event.data.ptr = request;

          if (epoll_ctl(m_epoll, EPOLL_CTL_ADD, clientSocket, &event) == -1)
          {
            PRINT_ERRNO("epoll_ctl", 2);
            close(clientSocket);
            break;
          }

          m_events.push_back({});
          request->m_updateTimeout(60000); // we will wait one min for user to complete a request

        }

        continue;

      }

      Request* request = static_cast<Request*>(m_events[i].data.ptr);

      while (request->m_recv())
      {

        

      }


      m_callback(request);

    }

  }

  if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, m_listenSocket, nullptr) == -1) PRINT_ERRNO("epoll_ctl", 0);

  close(m_listenSocket);
  close(m_epoll); 

  return 0; 

}
