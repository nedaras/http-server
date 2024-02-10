#include "networking/Server.h"

int main()
{

  Server server(nullptr);

  server.listen("3000");

  return 0;

}

