#include <http/http.h>
#include <fstream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

static std::vector<char> getFile(fs::path path)
{

  std::ifstream file(path);
  if (!file) 
  {
    std::cout << "file not found!\n";
    std::exit(0);
  }

  file.seekg(0, std::ios::end);
  std::streampos size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<char> buffer(static_cast<std::size_t>(size));
  file.read(buffer.data(), size);

  return buffer;

}

static void Handler(const http::request* request)
{

  std::vector<char> buffer = getFile("../index.html");

  request->write_body(buffer.data());
  request->end();

}

int main()
{

  http::server server = http::createServer(Handler);
  server.listen("3000");

  return 0;

}
