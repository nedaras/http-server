set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(http VERSION 1.0)

set(HTTP_SRC_FILES
  http/http.cpp
  http/networking/server.cpp
  http/networking/request.cpp
  http/networking/chunk_packet.cpp
  http/parser/parser.cpp
  http/siphash/siphash.cpp
)

add_library(http ${HTTP_SRC_FILES})
target_include_directories(http
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
)
