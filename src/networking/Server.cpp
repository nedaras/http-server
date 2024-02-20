#include "Server.h"

#include "./Request.h"
#include <chrono>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <vector>
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

    long timeout = -1;

    std::unique_lock<std::mutex> lock(m_mutex);

    // this is shit couse we can double free
    while (!m_timeouts.empty()) // this is so bad bad bad, we need priority queue with atleast O(log n) deletion time complexity
    {

      Timeout m_timeout = m_timeouts.front();
      std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

      if (now > m_timeout.timeout)
      {

        if (m_timeout.timeout == m_timeout.request->m_timeout) removeRequest(m_timeout.request);
        m_timeouts.pop();

        continue;

      }

      if (m_timeout.timeout == m_timeout.request->m_timeout) 
      {

        timeout = (m_timeout.timeout - now).count();
        break;

      }

      m_timeouts.pop();

    }

    epoll_event* events = m_events.data();
    std::size_t eventSize = m_events.size();

    lock.unlock();

    int epolls = epoll_wait(m_epoll, events, eventSize, timeout);

    if (epolls == 0)
    {

      removeRequest(m_timeouts.front().request);
      m_timeouts.pop();

    }

    for (int i = 0; i < epolls; i++)
    {
  
      if (m_events[i].events & EPOLLERR || m_events[i].events & EPOLLHUP)
      {

        if (m_events[i].data.ptr == &m_listenSocket) continue;

        Request* request = static_cast<Request*>(m_events[i].data.ptr);

        removeRequest(request);

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
          std::cout << "new con\n";
          m_events.push_back({});
          lock.unlock();

        }

        continue;

      }

      Request* request = static_cast<Request*>(m_events[i].data.ptr);
      REQUEST_STATUS status = request->m_parse(); // lets make this http 1.1, it means that we handle chunks and other http requests

      switch (status)
      {
      case REQUEST_SUCCESS:
        m_callback(request, Response(request, this));
        break;
      case REQUEST_INCOMPLETE: // dont push to timeout
        break;
      case REQUEST_CLOSE: // we should close it if its aint in timeouts
        if (request->m_parsed) break;

        removeRequest(request);

        break;
      case REQUEST_CHUNK_ERROR: // timeout should habdling it, but its not so great, we should atleast remove it from event list
        break;
      default:

        if (status == REQUEST_ERROR) PRINT_ERROR("Request::parse", 33);
        if (request->m_parsed) break;

        send(request->m_socket, "WTF UR DOING", 12, 0); // send some http response

        removeRequest(request);
                
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
