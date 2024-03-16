#pragma once

#include "networking/server.h"

namespace http
{

  http::server create_server(const http::request_handler& callback);

}
