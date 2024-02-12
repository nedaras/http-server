#include "Server.h"

#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <iostream>
#include <string>
#include "./Request.h"

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

  int clientSocket = accept(m_listenSocket, nullptr, nullptr);

  timeval timeout;

  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  
  setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  Request request(clientSocket);

  m_callback(request);  
  
  std::string html = "<form method='post'><label for='a'>TEXT:</label><input type='text' id='a' name='input' required><button type='submit'>SEND</button></form>";
  std::string http = std::string("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ") + std::to_string(html.size()) + "\r\n\r\n" + html;

  send(clientSocket, http.c_str(), http.size(), 0);  
  
  close(clientSocket); 
  close(m_listenSocket);

  return 0; 

}
