#include "networking/Server.h"
#include <iostream>

void Handler(const Request& request, const Response& response)
{

  if (request != REQUEST_SUCCESS)
  {
    std::cout << "Err in request\n";
  }


  std::cout << request.getHeader("Connection").value_or("Header not found") << "\n";

  std::string html = "<!DOCTYPE html><html><head></head><body><h1>hello world</h1></body></html>";

  response.writeHead("Content-Type", "text/html");
  response.writeHead("Content-Length", std::to_string(html.size()));

  response.write(html);

}

int main()
{
  
  Server server(Handler);

  server.listen("3000");

  return 0;

}

