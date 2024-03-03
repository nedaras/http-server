#include "Server.h"

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

#define PRINT_ERROR(f, l) std::cout << __FILE__  ":" << __LINE__ - l << "\n\t" f "(); // trowed " << errno << "\n\nError: " << std::strerror(errno) << "\n";

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
    PRINT_ERROR("getaddrinfo", 2); // make mby PRINT_ERRNO and PRINT_ERROR, some function will wail without setting errno
    return 1;
  }

  m_listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

  if (m_listenSocket == -1)
  {
    PRINT_ERROR("socket", 4);
    freeaddrinfo(result);
    return 1;
  }

  if (setNonBlocking(m_listenSocket) == -1)
  {
    PRINT_ERROR("setNonBlocking", 2);
    freeaddrinfo(result);
    close(m_listenSocket);
    return 1;
  }

  if (bind(m_listenSocket, result->ai_addr, result->ai_addrlen) == -1)
  {
    PRINT_ERROR("bind", 2);
    freeaddrinfo(result);
    close(m_listenSocket);
    return 1;
  }

  freeaddrinfo(result);

  if (::listen(m_listenSocket, SOMAXCONN) == -1)
  {
    PRINT_ERROR("listen", 2);
    close(m_listenSocket);
    return 1;
  }

  m_epoll = epoll_create1(0);

  if (m_epoll == -1)
  {
    PRINT_ERROR("epoll_create1", 4);
    close(m_listenSocket);
    return 1;
  }
  
  {

    epoll_event event;

    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = &m_listenSocket;

    if (epoll_ctl(m_epoll, EPOLL_CTL_ADD, m_listenSocket, &event) == -1)
    {
      PRINT_ERROR("epoll_ctl", 2);
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

      Request* request = m_timeouts.top();
      std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

      if (now >= request->m_timeout)
      {

        if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, nullptr) == -1) PRINT_ERROR("epoll_ctl", 0);

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

    int epolls = epoll_wait(m_epoll, events, eventSize, timeout);

    if (epolls == 0)
    {

      while (!m_timeouts.empty())
      {

        Request* request = m_timeouts.top();
        std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

        if (now >= request->m_timeout)
        {

          if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, nullptr) == -1) PRINT_ERROR("epoll_ctl", 0);

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

        if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, nullptr) == -1) PRINT_ERROR("epoll_ctl", 0);

        m_timeouts.erase(request);
        m_events.pop_back();

        request->m_callEvent(END, "err");

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
            if (errno != EWOULDBLOCK) PRINT_ERROR("accept", 4);
            break;
          }

          if (setNonBlocking(clientSocket) == -1)
          {
            PRINT_ERROR("setNonBlocking", 2);
            close(clientSocket);
            break;
          }
          
          Request* request = new Request(clientSocket);

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
            PRINT_ERROR("epoll_ctl", 2);
            close(clientSocket);
            break;
          }


          m_events.push_back({});

          request->m_updateTimeout(60000); // we will wait one min for user to complete a request
          m_timeouts.push(request);

        }

        continue;

      }

      Request* request = static_cast<Request*>(m_events[i].data.ptr);

      while (true)
      {

        auto [ status, newRequest ] = request->m_parse(); // lets make this http 1.1, it means that we handle chunks and other http requests

        switch (status)
        {
        case REQUEST_HTTP_COMPLETE:

          m_timeouts.erase(request);
          m_callback(request, Response(request, this));

          request->m_callEvent(END, "end");

          goto CONTINUE;
        case REQUEST_CHUNK_COMPLETE:

          if (newRequest) // if chunk complete and new req it means we skipped the REQUEST_HTTP_COMPLETE
          {

            m_timeouts.erase(request);
            m_callback(request, Response(request, this));

          }

          request->m_callEvent(DATA, request->m_chunk);

          m_timeouts.erase(request);
          request->m_updateTimeout(60000);
          m_timeouts.push(request);

          goto CONTINUE;
        case REQUEST_CHUNK_END:

          request->m_callEvent(END, "chunk_end");

          goto CONTINUE;
        case REQUEST_INCOMPLETE:
          if (newRequest)
          {
            m_timeouts.erase(request);
            request->m_updateTimeout(60000);
            m_timeouts.push(request);
          }
          break;
        case REQUEST_CLOSE:
        case REQUEST_HTTP_ERROR:
        case REQUEST_HTTP_BUFFER_ERROR:
        case REQUEST_ERROR:
        case REQUEST_CHUNK_ERROR:

          if (status != REQUEST_CLOSE) std::cout << "ERR " << status << "\n";

          if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, nullptr) == -1) PRINT_ERROR("epoll_ctl", 0);

          request->m_callEvent(END, "end");

          m_timeouts.erase(request);
          m_events.pop_back();

          close(request->m_socket);

          delete request;

          break;
        }
        
        break;

CONTINUE:

        // enable c++ 23

        std::cout << "c\n";

      }

    }

  }

  if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, m_listenSocket, nullptr) == -1) PRINT_ERROR("epoll_ctl", 0);

  close(m_listenSocket);
  close(m_epoll); 

  return 0; 

}
