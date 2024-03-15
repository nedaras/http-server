#include "Server.h"

#include "./Request.h"
#include "ChunkPacket.h"
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

int Server::removeRequest(const Request* request)
{

  if (request->m_receivingData()) request->m_chunkPacket->handleChunk({});
  if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, request->m_socket, nullptr) == -1) return -1;

  m_timeouts.erase(request);
  m_events.pop_back();

  close(request->m_socket);

  delete request;

  return 0;

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
    PRINT_ERROR("getaddrinfo", "Invalid port", 2);
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

    std::time_t timeout = -1;

    // move this to a function
    while (!m_timeouts.empty())
    {

      const Request* request = m_timeouts.top();
      std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

      if (now >= request->m_timeout)
      {
        if (removeRequest(request) == -1) PRINT_ERRNO("removeRequest", 0);
        continue;
      }
  
      timeout = (request->m_timeout - now).count();
      break;

    }

    epoll_event* events = m_events.data();
    std::size_t eventSize = m_events.size();

    int epolls = epoll_wait(m_epoll, events, static_cast<int>(eventSize), static_cast<int>(timeout));

    if (epolls == 0)
    {

      while (!m_timeouts.empty())
      {

        const Request* request = m_timeouts.top();
        std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

        if (now >= request->m_timeout)
        {
          if (removeRequest(request) == -1) PRINT_ERRNO("removeRequest", 0);
          continue;
        }

        break;

      }

      continue;

    }

    for (std::size_t i = 0; i < static_cast<std::size_t>(epolls); i++)
    {
  
      if (m_events[i].events & EPOLLERR || m_events[i].events & EPOLLHUP)
      {

        if (m_events[i].data.ptr == &m_listenSocket) continue;

        Request* request = static_cast<Request*>(m_events[i].data.ptr);

        if (removeRequest(request) == -1) PRINT_ERRNO("removeRequest", 0);
        break;

      }

      if (!(m_events[i].events & EPOLLIN)) continue;
      if (m_events[i].data.ptr == &m_listenSocket)
      {

        while (true)
        {

          int clientSocket = accept(m_listenSocket, nullptr, nullptr); // put these addresses in request

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
            PRINT_ERROR("new Request", "allocation failed", 4);
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
          request->m_updateTimeout(60000);

        }

        continue;

      }

      Request* request = static_cast<Request*>(m_events[i].data.ptr);

      // -- #1 we need to know if this is like a new request
      // -- #2 we need to parse **only** the http part first
      // #3 we need to know when should a request be completed, even without calling response::end
      // -? #4 client has to set if we want to recv body or recv chunked data, like with methods request::parseBody
      //    if not we need to throw if data is invalid or throw if they're sending data (mb this is dumb con will be closed if we not read data)
     
      // #5 how the fuck should request::parseBody even work? if lets sey they dont sent whole body in one packet
      //    mb make a macro that would early return, mb to same shit as below
      
      // -? #6 fuck #5 first we need to habdle chunked encoding baby
      //    mb add request::parseData([request](optional<data>) -> return true)
      //      + what it does it will just add an callback where we recv for data and if optional is null it means connection was closed
      //      - return value just says if data sent was correct, if we return false connection will be terminated
      //      + if data size will be 0 it means we have recv the last chunk or everything is done
      //      - note only in call back we can call the request::end so we need to be robust and all
      //      +- cool thing is if we call this function we cann parse our left buffer and then call it in the callback
      // #7 we need to update chunked encoding timeouts to a minute we dont do it yeet we're like waiting for 5 seconds to send all data
      //    which is even worse

      bool loop = true;
      while (loop)
      {

        READ_RESPONSE status = request->m_read(); 

        switch(status)
        {
        case READ_RESPONSE_DONE:
          if (request->m_receivingData())
          {
            request->m_chunkPacket->handleChunk();
            break;
          }
          m_callback(request);
          break;
        case READ_RESPONSE_WAITING:
          break;
        case READ_RESPONSE_CLOSE:
          if (removeRequest(request) == -1) PRINT_ERRNO("removeRequest", 0);
          loop = false;
          break;
        default:
          if (errno == EWOULDBLOCK)
          {
            loop = false;
            break;
          }
          std::cout << "Errr: " << int(status) << "\n";
          if (removeRequest(request) == -1) PRINT_ERRNO("removeRequest", 0);
          loop = false;
          break;
        }

      }

    }

  }

  if (epoll_ctl(m_epoll, EPOLL_CTL_DEL, m_listenSocket, nullptr) == -1) PRINT_ERRNO("epoll_ctl", 0);

  close(m_listenSocket);
  close(m_epoll); 

  return 0; 

}
