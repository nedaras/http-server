#include "Server.h"

#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "./Request.h"
#include "Response.h"
#include <unordered_map>
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
  std::unordered_map<int, Request> requests;

  ev.events = EPOLLIN; // brainstorm ev.data can store 8 bytes why not to store like to a request objects pointer
  ev.data.fd = m_listenSocket;
  
  epoll_ctl(efd, EPOLL_CTL_ADD, m_listenSocket, &ev);

  events.push_back({});
  
  while (true)
  {
    int nfds = epoll_wait(efd, events.data(), events.size(), -1);

    for (int i = 0; i < nfds; i++)
    {

      if (events[i].events & EPOLLIN)
      {

        if (events[i].data.fd == m_listenSocket)
        {

          int fd_new = accept(m_listenSocket, nullptr, nullptr);
      
          ev.events = EPOLLIN;
          ev.data.fd = fd_new;
          
          events.push_back({});
          epoll_ctl(efd, EPOLL_CTL_ADD, fd_new, &ev);

        }
        else
        {
            
          if (requests.find(events[i].data.fd) == requests.end())
          {
            
            Request request(events[i].data.fd);
            requests[events[i].data.fd] = std::move(request);

          }

          Request request = requests.at(events[i].data.fd);
          
          int r = request.parse(); 

          if (r == 0)
          {

            Response response(events[i].data.fd);

            m_callback(request, response);

            ev.events = EPOLLET;
            ev.data.fd = events[i].data.fd;

            events.pop_back();
            requests.erase(events[i].data.fd);

            epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
            close(events[i].data.fd);
            
            

          }
        }
      }
    }
  }
  // remove listensocket from epoll
  close(m_listenSocket);

  return 0; 

}
