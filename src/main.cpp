#include "networking/Server.h"
#include <iostream>

// i think best practise is to handle non blocking io in main thread and intensive cpu work in threadpool

// so this is intensive cpu but we can like add request.onData and do dome stuff im main thread
//
// mb make Handler func single threaded an inside we can specify if we want to throw it inside threadpool???,
// thats has to be the best thing
void Handler(const Request* request, const Response& response)
{
  // GOAL: non blocking read chunks and write to file, meaning that we can have unlimited conections
  // GOAL: intensive CPU work for handling chunked data, meaning async io with multithreading together
  
  if (request->getPath() == "/favicon.ico")
  {

    response.writeHead("Content-Type", "text/html");
    response.writeHead("Connection", "keep-alive");

    response.writeBody("no fav icon."); 

    response.end();

  }

  if (request->getPath() == "/css/style.css")
  {

    response.writeHead("Content-Type", "text/css");
    response.writeHead("Connection", "keep-alive");

    response.writeBody("h2 { color: red; }"); 

    response.end();

    return;

  }

  response.writeHead("Content-Type", "text/html");
  response.writeHead("Connection", "keep-alive");
  response.writeHead("Transfer-Encoding", "chunked");

  response.write("<!DOCTYPE html><html><head><link rel='stylesheet' href='./css/style.css'></head><body><h1>hello world</h1>");
  response.write("<h2>this is a chunk</h2>");
  response.write("</body></html>");

  response.end();

}

struct Compare
{

  bool operator()(float& left, float& right) const
  {

    return left > right;

  }

};

int main()
{
  // mb init threadpool here
  
  Server server(Handler);

  server.listen("3000");

  return 0;

}

