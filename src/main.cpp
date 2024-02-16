#include "networking/Server.h"
#include <iostream>
#include <sys/socket.h>

void Handler(const Request* request, const Response& response)
{
  // GOAL: non blocking read chunks and write to file, meaning that we can have unlimited conections
  // GOAL: intensive CPU work for handling chunked data, meaning async io with multithreading together

  std::cout << "Connection: " << request->getHeader("Connection").value_or("Header not found") << "\n";

  response.writeHead("Content-Type", "text/html");
  response.writeHead("Transfer-Encoding", "chunked");
  response.writeHead("Connection", "keep-alive");
  
  response.write("<!DOCTYPE html><html><head></head><body><h1>hello world</h1>");
  response.write("<h2>this is a chunk</h2>");
  response.write("</body></html>");

  response.end(); // should res end like close connection, or it just means that we done this connection

}

int main()
{
 
  Server server(Handler);

  server.listen("3000");

  return 0;

}

