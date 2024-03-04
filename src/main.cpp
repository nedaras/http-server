#include "networking/Server.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

static fs::path publicDir;

// we aint handling close connection
// we will add ssh
void Handler(const Request* request, const Response& response)
{

  std::string file = request->getPath() == "/" ? "index.html" : std::string(&request->getPath()[1], request->getPath().size() - 1);
  std::string extension = file.substr(file.find('.') + 1); // bad :)

  response.writeHead("Content-Type", "text/" + extension);
  response.writeHead("Connection", "keep-alive");

  request->on(END, [response](std::string_view) {

    response.end();

  });

  if (request->getMethod() == "POST")
  {

    response.writeBody("<p>POST</p>");

    return;

  }

  // should we put body is this data thing or just send chunked data here
  request->on(DATA, [](std::string_view data) {
    
    std::cout << data << "new data\n";

  });
  
  if (!fs::exists(publicDir / file))
  {

    response.writeBody("no file brah");

    return;

  }

  response.writeHead("Transfer-Encoding", "chunked");

  std::ifstream stream(publicDir / file);
  std::array<char, 0x1000> buffer;
  
  while (!stream.eof())
  {

    stream.read(buffer.data(), buffer.size());
    ssize_t length = stream.gcount();

    if (length > 0) response.write(buffer.data(), length);

  }

}

int main()
{

  publicDir = fs::read_symlink("/proc/self/exe").parent_path().parent_path().parent_path() / "public";

  Server server(Handler);

  server.listen("3000");

  return 0;

}

