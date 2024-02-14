#include "Server.h"

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include "./Request.h"
#include "Response.h"

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
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));// dont depend on timeouts make it non blocking io with select
                                                                                 // and mby add multithreading for handling a request mby, mby
                                                                                 // user should make multithreading itself idk

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

  close(m_listenSocket);

  return 0; 

}
