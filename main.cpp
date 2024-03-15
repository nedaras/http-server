#include "src/networking/Server.h"

static void Handler(const Request* request)
{

  request->writeBody("Hello World!");
  request->end();

}

int main()
{

  Server server(Handler);
  server.listen("3000");

  return 0;

}
