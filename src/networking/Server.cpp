#include "Server.h"

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <iostream>

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

  int clientSocket = accept(m_listenSocket, nullptr, nullptr);

  char buffer[8 * 1024];

  int bytes = recv(clientSocket, buffer, sizeof(buffer), 0);
  
  std::cout.write(buffer, bytes);

  close(clientSocket); 

  return 0; 

}
