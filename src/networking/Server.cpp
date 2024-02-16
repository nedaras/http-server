#include "Server.h"

#include <functional>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "./Request.h"
#include "Response.h"
#include <vector>
#include "../threadpool/ThreadPool.h"

// TODO: better api,
// TODO: handle errors
// TODO: strict http parser
// TODO: threadpoll
// TODO: chunked data handling
// TODO: profit
 


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

  ThreadPool threadPool(std::thread::hardware_concurrency());
  m_epoll = epoll_create1(0);

  
  {

    epoll_event event;

    event.events = EPOLLIN;
    event.data.ptr = &m_listenSocket;

    epoll_ctl(m_epoll, EPOLL_CTL_ADD, m_listenSocket, &event);

    m_events.push_back({});
  
  }
  // where in the fuck is error handling hu? 
  while (true)
  {

    int epolls = epoll_wait(m_epoll, m_events.data(), m_events.size(), -1);

    for (int i = 0; i < epolls; i++)
    {
     
      if (!(m_events[i].events & EPOLLIN)) continue;
      if (m_events[i].data.ptr == &m_listenSocket)
      {

        int clientSocket = accept(m_listenSocket, nullptr, nullptr);
        Request* request = new Request(clientSocket);

        epoll_event event;

        event.events = EPOLLIN;
        event.data.ptr = request;

        epoll_ctl(m_epoll, EPOLL_CTL_ADD, clientSocket, &event);

        m_events.push_back({});

        continue;

      }

      Request* request = static_cast<Request*>(m_events[i].data.ptr);

      int result = request->parse(); // put in worker to? 

      if (result == -1)
      {

        epoll_event event;

        event.events = EPOLLET;
        event.data.ptr = nullptr;
        
        send(request->getSocket(), "WTF UR DOING", 12, 0);

        request->dead = true;

        epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->getSocket(), &event);
        close(request->getSocket());
        
        m_events.pop_back();
        delete request;
      
        continue;
      }

      if (result == 0) // throw this in thread pool and think how should we handle chunked encoding and stuff jeez
      {

        threadPool.addTask(&Server::m_makeResponse, this, request); // handle closing conection, couse now we just crashing

      }

    }

  }

  epoll_event event;
  
  event.events = EPOLLET;
  event.data.ptr = nullptr;

  epoll_ctl(m_epoll, EPOLL_CTL_DEL, m_listenSocket, &event);

  close(m_listenSocket);
  close(m_epoll); 

  return 0; 

}

void Server::m_makeResponse(Request* request) // add state to check if con is closed
{

  Response response(request->getSocket());

  m_callback(request, response);

  if (request->dead) return;

  epoll_event event;

  event.events = EPOLLET;
  event.data.ptr = nullptr;

  epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->getSocket(), &event); // cant pass closed socket,
                                                                 // btw we need state to handle keep alive
  close(request->getSocket());

  m_events.pop_back();

  delete request; // i have that m_buffer gets deleted too

}
