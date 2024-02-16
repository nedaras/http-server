#include "networking/Server.h"
#include <iostream>
#include <sys/socket.h>
#include <thread>

void Handler(const Request* request, const Response& response)
{
  // GOAL: non blocking read chunks and write to file, meaning that we can have unlimited conections
  // GOAL: intensive CPU work for handling chunked data, meaning async io with multithreading together

  //std::this_thread::sleep_for(std::chrono::seconds(2));

  std::cout << request->getHeader("Connection").value_or("Header not found") << "\n";

  response.writeHead("Content-Type", "text/html");
  response.writeHead("Transfer-Encoding", "chunked");

  response.write("<!DOCTYPE html><html><head></head><body><h1>hello world</h1>");
  response.write("<h2>this is a chunk</h2>");
  response.write("</body></html>");

  // just for now
  send(request->getSocket(), "0\r\n\r\n", 5, 0);
  //for now we closing automaticly
  //response.end(); // not if keep alive, but we should add some like timeout function

}

int main()
{
 
  Server server(Handler);

  server.listen("3000");

  return 0;

}

