#include "Server.h"

#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "./Request.h"
#include "Response.h"
#include <vector>

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
    
    return 1;

  }

  m_listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  
  if (m_listenSocket == 0)
  {
  
    return 1;

  }

  if (bind(m_listenSocket, result->ai_addr, result->ai_addrlen) == -1)
  {

    return 1;

  }

  freeaddrinfo(result);

  if (::listen(m_listenSocket, SOMAXCONN) == -1)
  {

    return 1;

  }

  timeval timeout;

  timeout.tv_sec = 5;
  timeout.tv_usec = 0;

  int efd = epoll_create1(0);

  epoll_event ev;
  
  std::vector<epoll_event> events;

  ev.events = EPOLLIN; // brainstorm ev.data can store 8 bytes why not to store like to a request objects pointer
  ev.data.fd = m_listenSocket;
  
  epoll_ctl(efd, EPOLL_CTL_ADD, m_listenSocket, &ev);
  events.push_back({});

  while (true)// single threaded :(, blocking io
  {
    int nfds = epoll_wait(efd, events.data(), events.size(), -1);

    std::cout << events.size() << "\n";

    for (int i = 0; i < nfds; i++)
    {

      if (events[i].events & EPOLLIN)
      {

        if (events[i].data.fd == m_listenSocket)
        {
          
          std::cout << "new con\n";

          int fd_new = accept(m_listenSocket, nullptr, nullptr);
      
          ev.events = EPOLLIN;
          ev.data.fd = fd_new;
          
          events.push_back({});
          epoll_ctl(efd, EPOLL_CTL_ADD, fd_new, &ev);

        }
        else
        {

          char buffer[1024];
          ssize_t bytes = recv(events[i].data.fd, buffer, 1024, 0);

          if (bytes == 0) 
          {

            std::cout << "con closed\n";
            
            ev.events = EPOLLET;
            ev.data.fd = events[i].data.fd;

            events.pop_back();
            epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
            close(events[i].data.fd);

            continue;
          }
          std::cout << "new data\n";
          std::cout.write(buffer, bytes);
  
          send(events[i].data.fd, "Sup!", 4, 0);

        }
      }
    }

    continue;

    int clientSocket = accept(m_listenSocket, nullptr, nullptr);
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));// dont depend on timeouts make it non blocking io with select

    // how to implememt epoll, how to track buffers efficent and in async way
    // if send blocks we need to make it non blocking too, but then how to send packets in correct order 

    Request request(clientSocket);
       
    if (request == REQUEST_CLOSE)
    {
      close(clientSocket);
      continue;
    }

    Response response(clientSocket);
    m_callback(request, response); // we need to handle chunk data like idk in request add reader with callback
    
    // handle keep alive if response was so

  }
  // remove listensocket from epoll
  close(m_listenSocket);

  return 0; 

}
