#include "networking/Server.h"

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

  request->readData([request](auto data) {

    std::string_view body = data.value_or("no data");

    if (body.size() == 0) 
    {

      request->setHead("Content-Type", "text/html");
      request->writeBody("<h1>Server Component!</h1>");

      request->end();

    }

    return true;

  });

  //request->setHead("Content-Type", "application/json");

  //request->end(); // it we comma it it will make readData recv everything, for now even http req

}

int main()
{

  Server server(Handler);

  server.listen("3000");

}

