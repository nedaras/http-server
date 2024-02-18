#include "networking/Server.h"
#include <iostream>
#include <thread>

// i think best practise is to handle non blocking io in main thread and intensive cpu work in threadpool

// so this is intensive cpu but we can like add request.onData and do dome stuff im main thread
//
// mb make Handler func single threaded an inside we can specify if we want to throw it inside threadpool???,
// thats has to be the best thing
void Handler(const Request* request, const Response& response)
{
  // GOAL: non blocking read chunks and write to file, meaning that we can have unlimited conections
  // GOAL: intensive CPU work for handling chunked data, meaning async io with multithreading together
  
  std::cout << request->getPath() << "\n";

  if (request->getPath() == "/long")
  {
    
    std::thread([request, response] {

      std::this_thread::sleep_for(std::chrono::seconds(10));

      response.writeHead("Content-Type", "text/html");
      response.writeHead("Transfer-Encoding", "chunked");
      response.writeHead("Connection", "closed");

      response.write("<!DOCTYPE html><html><head></head><body><h1>hello world</h1>");
      response.write("<h2>this is a chunk</h2>");
      response.write("</body></html>");

      // we can do threadpool.addTask, do saome long calculation and call inside response.end

      response.end(); // this should say that we ended the request.
    }).detach(); 
   
    return;

  }

  response.writeHead("Content-Type", "text/html");
  response.writeHead("Transfer-Encoding", "chunked");
  response.writeHead("Connection", "closed");

  response.write("<!DOCTYPE html><html><head></head><body><h1>hello world</h1>");
  response.write("<h2>this is a chunk</h2>");
  response.write("</body></html>");

  response.end();

}

int main()
{
  // mb init threadpool here 
  Server server(Handler);

  server.listen("3000");

  return 0;

}

