#include "networking/Server.h"
#include <iostream>

void Handler(const Request& request)
{

  std::cout << request.GetHeader("Accept").value_or("Header not found") << "\n";

}

int main()
{
  
  Server server(Handler);

  server.listen("3000");

  return 0;

}

