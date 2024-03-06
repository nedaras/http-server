#include "networking/Server.h"
#include <fstream>
#include <memory>

// TODO: add like default not found behavior
// TODO: handle conection close

// we will add ssh
//
//
// TODO: auth
void Handler(const Request* request, const Response& response)
{

  if (request->getPath() == "/")
  {

    response.writeHead("Connection", "keep-alive");
    response.writeHead("Content-Type", "text/html");

    std::ifstream stream("/home/nedas/source/http-server/index.html");

    stream.seekg(0, std::ios::end);
    std::size_t size = stream.tellg();
    stream.seekg(0, std::ios::beg);

    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(size);
    stream.read(buffer.get(), size);

    response.writeBody(buffer.get(), size);

    response.end();

  }

  if (request->getPath() == "/api/server")
  {
    
    response.writeHead("Connection", "keep-alive");
    response.writeHead("Content-Type", "text/html");

    response.writeBody("<h1>Server Component!</h1>");

    response.end();

  }

}

int main()
{

  Server server(Handler);

  server.listen("3000");

}

