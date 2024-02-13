#include "Server.h"

#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <iostream>
#include <string>
#include "./Request.h"
#include "Response.h"

static bool isEOF(const char* buffer, size_t bufferLength, size_t bytesRead)
{

  for (ssize_t i = bufferLength; i < bufferLength + bytesRead; i++) // find end of header
  {

    if (buffer[i] == '\n' && i - 3 >= 0)
    {

      if(std::strncmp(&buffer[i] - 3, "\r\n\r\n", 4) == 0) return 1;

    }

  }
  
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

  while (true)// single threaded :(, blocking io
  {

    int clientSocket = accept(m_listenSocket, nullptr, nullptr);
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    Request request(clientSocket);


    Response response(clientSocket);
    m_callback(request, response);

    close(clientSocket); 

  }

  close(m_listenSocket);

  return 0; 

}
