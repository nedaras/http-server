#pragma once

#include "networking/server.h"

namespace http
{

  http::server createServer(const http::request_handler& callback);

}
