#include "networking/Server.h"
#include <iostream>
#include <sys/socket.h>

void Handler(const Request& request, const Response& response)
{

  std::cout << request.getHeader("Connection").value_or("Header not found") << "\n";

  response.writeHead("Content-Type", "text/html");
  //response.writeHead("Content-Length", std::to_string(html.size()));
  response.writeHead("Transfer-Encoding", "chunked");

  response.write("<!DOCTYPE html><html><head></head><body><h1>hello world</h1>");
  response.write("<h2>this is a chunk</h2>");
  response.write("</body></html>");

  // just for now
  send(request.getSocket(), "0\r\n\r\n", 5, 0);

  //for now we closing automaticly
  //response.end(); // not if keep alive, but we should add some like timeout function

}

int main()
{
  
  Server server(Handler);

  server.listen("3000");

  return 0;

}

