#include "http.h"


Server http::createServer(const RequestHandler& callback)
{
  return Server(std::move(callback));
}
