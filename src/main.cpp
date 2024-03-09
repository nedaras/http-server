#include "networking/Server.h"
#include "siphash/siphash.h"
#include <cstdint>
#include <cstring>

// TODO: add like default not found behavior
// TODO: handle conection close

// we will add ssh
//
//
// TODO: auth
//
// TODO: make server.h throw error if we tryna call like request on DATA if its null and handle cases
void Handler(const Request* request)
{

  //request->on(END, [request](auto dt)
  //{

    //std::cout << dt << "\n";
    //request.end();

  //});

  //request->on(DATA, [](auto dt)
  //{

    //std::cout << dt << "\n";

  //});

  request->setHead("Content-Type", "text/html");

  request->writeBody("<h1>Server Component!</h1>");
  request->end();

}

int main()
{

  Server server(Handler);

  server.listen("3000");

}

