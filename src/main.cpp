#include "networking/Server.h"

// TODO: add like default not found behavior
// TODO: handle conection close

// we will add ssh
//
//
// TODO: auth
//
// TODO: make server.h throw error if we tryna call like request on DATA if its null and handle cases
void Handler(const Request* request, const Response& response)
{

  request->on(END, [response](auto dt)
  {

    std::cout << dt << "\n";
    response.end();

  });

  request->on(DATA, [](auto dt)
  {

    std::cout << dt << "\n";

  });

  response.writeHead("Connection", "keep-alive");
  response.writeHead("Content-Type", "text/html");

  response.writeBody("<h1>Server Component!</h1>");

}

int main()
{

  Server server(Handler);

  server.listen("3000");

}

