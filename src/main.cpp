#include <iostream>
#include "networking/Server.h"

int main()
{

  Server server(nullptr);

  server.listen("8080");

  return 0;

}

