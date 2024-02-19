#include "Server.h"

#include <asm-generic/socket.h>
#include <bits/types/struct_timeval.h>
#include <chrono>
#include <ctime>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <queue>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <vector>
#include "./Request.h"
#include <fcntl.h>

#define PRINT_ERROR(f, l) std::cout << __FILE__  ":" << __LINE__ - l << "\n\t" f "(); // trowed " << errno << "\n\nError: " << std::strerror(errno) << "\n";

// TODO: better api,
// TODO: strict http parser
// TODO: chunked data handling
// TODO: profit
// TODO: turn of recv travic if we aint expecting to recv
 
// should we crash at errors?

static int setNonBlocking(int socket)
{

  int flags = fcntl(socket, F_GETFL, 0);
  return flags ? fcntl(socket, F_SETFL, flags | O_NONBLOCK) : -1;

}

void Server::removeRequest(Request* request)
{

  epoll_event event {};

  if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, &event) == -1) PRINT_ERROR("epoll_ctl", 0);

  m_mutex.lock();
  m_events.pop_back(); 
  m_mutex.unlock();

  close(request->m_socket);

  delete request;

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

    std::unique_lock<std::mutex> lock(m_mutex);

    epoll_event* events = m_events.data();
    std::size_t eventSize = m_events.size();

    lock.unlock();

    long timeout = -1;
    
    while (!m_timeouts.empty())
    {
      
      Request* request = m_timeouts.top();
      std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

      if (now > request->m_timeout)
      {

        removeRequest(request);
        m_timeouts.pop();

        continue;

      }

      timeout = (request->m_timeout - now).count();
      
      break;

    }

    std::cout << timeout << "\n";

    int epolls = epoll_wait(m_epoll, events, eventSize, timeout);

    if (epolls == 0)
    {

      removeRequest(m_timeouts.top());
      m_timeouts.pop();

    }

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
            if (errno != EWOULDBLOCK) PRINT_ERROR("accept", 4);
            break;
          }

          if (setNonBlocking(clientSocket) == -1)
          {
            PRINT_ERROR("setNonBlocking", 2);
            close(clientSocket);
            break;
          }
          
          Request* request = new Request(clientSocket); // do we need to check if ptr is nullptr?

          if (request == nullptr) // if this happens just close program
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

          lock.lock();
          m_events.push_back({});
          m_timeouts.push(request);
          lock.unlock();

        }

        continue;

      }

      Request* request = static_cast<Request*>(m_events[i].data.ptr);
      REQUEST_STATUS status = request->m_parse(); // lets make this http 1.1, it means that we handle chunks and other http requests

      std::cout << request << "\n";

      epoll_event event {};
      
      switch (status)
      {
      case REQUEST_SUCCESS:

        m_callback(request, Response(request, this));

        

        { // deleting element, u dont even know how much i hate this

          std::vector<Request*> temp;

          while (!m_timeouts.empty())
          {
           
            if (m_timeouts.top() == request)
            {

              m_timeouts.pop();
              break;

            }
            
            temp.push_back(m_timeouts.top());
            m_timeouts.pop();

          }
          
          for (Request* element : temp) m_timeouts.push(element);

        }

        request->updateTimeout(5000);
        m_timeouts.push(request);

        break;
      case REQUEST_INCOMPLETE:
        std::cout << "ayaya cocojumpo\n";
        break;
      case REQUEST_CLOSE:
        std::cout << "REQUEST_CLOSE\n";
        if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, &event) == -1) PRINT_ERROR("epoll_ctl", 0);

        lock.lock();
        m_events.pop_back();

        { // deleting element

          std::vector<Request*> temp;

          while (!m_timeouts.empty())
          {
           
            if (m_timeouts.top() == request)
            {

              m_timeouts.pop();
              break;

            }
            
            temp.push_back(m_timeouts.top());
            m_timeouts.pop();

          }
          
          for (Request* element : temp) m_timeouts.push(element);

        }

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
        std::cout << "SOME_REQUEST_ERROR\n";
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
