#include "Server.h"

#include <iostream>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <vector>
#include "./Request.h"

#define PRINT_ERROR(f, l) std::cout << __FILE__  ":" << __LINE__ - l << "\n\t" f "(); // throwed " << errno << "\n\nError: " << std::strerror(errno) << "\n";

// TODO: better api,
// TODO: handle errors
// TODO: strict http parser
// TODO: threadpoll
// TODO: chunked data handling
// TODO: profit
// TODO: turn of recv travic if we aint expecting to recv
 
// should we crash at errors?

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

    int epolls = epoll_wait(m_epoll, m_events.data(), m_events.size(), -1);

    for (int i = 0; i < epolls; i++)
    {
    
      if (!(m_events[i].events & EPOLLIN)) continue;
      if (m_events[i].data.ptr == &m_listenSocket)
      {

        int clientSocket = accept(m_listenSocket, nullptr, nullptr); // but these addresses in request
        
        if (clientSocket == -1)
        {
          PRINT_ERROR("accept", 2);
          continue;
        }

        Request* request = new Request(clientSocket); // do we need to check if ptr is nullptr?
        
        if (request == nullptr) std::cout << "nullptr request\n";                                              

        epoll_event event;

        event.events = EPOLLIN | EPOLLET; 
        event.data.ptr = request;

        if (epoll_ctl(m_epoll, EPOLL_CTL_ADD, clientSocket, &event) == -1)
        {
          PRINT_ERROR("epoll_ctl", 2);  
          continue;
        }

        m_mutex.lock();
        m_events.push_back({});
        m_mutex.unlock(); 

        continue;

      }

      std::cout << "new read" << "\n";
      Request* request = static_cast<Request*>(m_events[i].data.ptr);
      REQUEST_STATUS status = request->parse();
      
      epoll_event event {};
      

      switch (status)
      {
      case REQUEST_SUCCESS:
        m_callback(request, Response(request, this));
        break;
      case REQUEST_INCOMPLETE:
        break;
      case REQUEST_CLOSE:

        if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, &event) == -1) PRINT_ERROR("epoll_ctl", 0);

        m_mutex.lock();
        m_events.pop_back();
        m_mutex.unlock();

        close(request->m_socket);
        request->m_socket = 0;

        if (!request->m_parsed) delete request;

        break;
      case REQUEST_CHUNK_ERROR:
        std::cout << "REQUEST_CHUNK_ERROR\n";
        break;
      default:
        send(request->m_socket, "WTF UR DOING", 12, 0); // send some http response

        if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, &event) == -1) PRINT_ERROR("epoll_ctl", 0);

        m_mutex.lock();
        m_events.pop_back();
        m_mutex.unlock();

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
