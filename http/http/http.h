#pragma once

#include "networking/Server.h"

namespace http
{

  Server createServer(const RequestHandler& callback);

}
