#include "http.h"

http::server http::create_server(const http::request_handler& callback)
{
  return http::server(std::move(callback));
}
