#include "Server.h"

#include <iostream>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <vector>
#include "./Request.h"
#include <fcntl.h>

#define PRINT_ERROR(f, l) std::cout << __FILE__  ":" << __LINE__ - l << "\n\t" f "(); // throwed " << errno << "\n\nError: " << std::strerror(errno) << "\n";

// TODO: better api,
// TODO: handle errors
// TODO: strict http parser
// TODO: threadpoll
// TODO: chunked data handling
// TODO: profit
// TODO: turn of recv travic if we aint expecting to recv
 
// should we crash at errors?

// nah bro main thrad has to handle m_events wtf
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

  int flags = fcntl(m_listenSocket, F_GETFL, 0);

  if (flags == -1) PRINT_ERROR("a", 0); 

  flags |= O_NONBLOCK;

  if (fcntl(m_listenSocket, F_SETFL, flags) == -1) PRINT_ERROR("fcntl", 0);

  if (m_listenSocket == -1)
  {
    PRINT_ERROR("socket", 4);
    freeaddrinfo(result);
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

    std::unique_lock<std::mutex> lock(m_mutex);

    epoll_event* events = m_events.data();
    std::size_t eventSize = m_events.size();

    lock.unlock();

    int epolls = epoll_wait(m_epoll, events, eventSize, -1); // o kurwa

    for (int i = 0; i < epolls; i++)
    {
  
      if (m_events[i].events & EPOLLERR || m_events[i].events & EPOLLHUP)
      {

        std::cout << "errrrroroorororo\n"; // i never got an error i rly dont even know how should i handle them

      }

      if (!(m_events[i].events & EPOLLIN)) continue;
      if (m_events[i].data.ptr == &m_listenSocket)
      {
        
        while (true)
        {

          int clientSocket = accept(m_listenSocket, nullptr, nullptr); // but these addresses in request

          if (clientSocket == -1)
          {

            if (errno == EWOULDBLOCK) break;

            PRINT_ERROR("accept", 7);
            break;
          }

          int flags = fcntl(clientSocket, F_GETFL, 0);

          if (flags == -1) PRINT_ERROR("a", 0); 

          flags |= O_NONBLOCK;

          if (fcntl(clientSocket, F_SETFL, flags) == -1) PRINT_ERROR("fcntl", 0);

          Request* request = new Request(clientSocket); // do we need to check if ptr is nullptr?

          if (request == nullptr)
          {
            std::cout << "nullptr request\n";
            break;
          }

          epoll_event event;

          event.events = EPOLLIN | EPOLLET; 
          event.data.ptr = request;
          if (epoll_ctl(m_epoll, EPOLL_CTL_ADD, clientSocket, &event) == -1)
          {
            PRINT_ERROR("epoll_ctl", 2);  
            break;
          }

          lock.lock();
          m_events.push_back({});
          lock.unlock(); 
        }

        continue;

      }

      Request* request = static_cast<Request*>(m_events[i].data.ptr);
      REQUEST_STATUS status = request->parse(); // we should exit till we reach that EWOULDBLOCK state
            
      epoll_event event {};
      
      switch (status)
      {
      case REQUEST_SUCCESS:
        m_callback(request, Response(request, this));
        break;
      case REQUEST_INCOMPLETE:
        std::cout << "ayaya cocojumpo\n";
        break;
      case REQUEST_CLOSE:

        if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, &event) == -1) PRINT_ERROR("epoll_ctl", 0);

        lock.lock();
        m_events.pop_back();
        lock.unlock();

        close(request->m_socket);
        request->m_socket = 0;

        if (!request->m_parsed) 
        {
          delete request;
        }

        break;
      case REQUEST_CHUNK_ERROR:
        std::cout << "REQUEST_CHUNK_ERROR\n";
        break;
      default:

        if (status == REQUEST_ERROR) PRINT_ERROR("Request::parse", 33);

        send(request->m_socket, "WTF UR DOING", 12, 0); // send some http response

        if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, &event) == -1) PRINT_ERROR("epoll_ctl", 0);

        lock.lock();
        m_events.pop_back();
        lock.unlock();

        close(request->m_socket);
        delete request;
        
        break;
      }

    }

  }

  epoll_event event {};
  
  if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, m_listenSocket, &event) == -1) PRINT_ERROR("epoll_ctl", 0);

  close(m_listenSocket);
  close(m_epoll); 

  return 0; 

}
