#include "http.h"

http::server http::createServer(const http::request_handler& callback)
{
  return http::server(std::move(callback));
}
