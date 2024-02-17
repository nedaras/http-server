#include "networking/Server.h"
#include <chrono>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

// i think best practise is to handle non blocking io in main thread and intensive cpu work in threadpool



// so this is intensive cpu but we can like add request.onData and do dome stuff im main thread
//
// mb make Handler func single threaded an inside we can specify if we want to throw it inside threadpool???,
// thats has to be the best thing
void Handler(const Request* request, const Response& response)
{
  // GOAL: non blocking read chunks and write to file, meaning that we can have unlimited conections
  // GOAL: intensive CPU work for handling chunked data, meaning async io with multithreading together

  std::cout << "Connection: " << request->getHeader("Connection").value_or("Header not found") << "\n";

  std::this_thread::sleep_for(std::chrono::seconds(5));

  response.writeHead("Content-Type", "text/html");
  response.writeHead("Transfer-Encoding", "chunked");
  response.writeHead("Connection", "keep-alive");
  
  response.write("<!DOCTYPE html><html><head></head><body><h1>hello world</h1>");
  response.write("<h2>this is a chunk</h2>");
  response.write("</body></html>");

  // we can do threadpool.addTask, do saome long calculation and call inside response.end

  response.end(); // this should say that we ended the request.
                  // what it would mean is that we would just reset the request object

}

int main()
{
  // mb init threadpool here 
  Server server(Handler);

  server.listen("3000");

  return 0;

}

