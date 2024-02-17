#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include "../http/parser.h"

enum REQUEST_STATUS
{
  REQUEST_SUCCESS,
  REQUEST_INCOMPLETE,
  REQUEST_ERROR,
  REQUEST_CLOSE,
  REQUEST_HTTP_ERROR,
  REQUEST_HTTP_BUFFER_ERROR,
  REQUEST_CHUNK_ERROR 
};

// can we make this dude hold everything
class Request
{

public:

  Request(int socket);

  std::optional<std::string_view> getHeader(std::string_view header) const;

  REQUEST_STATUS parse();

private:

  friend class Server;
  friend class Response;

  std::atomic<int> m_socket;

  bool m_parsed = false;

  http::Parser m_parser;
  
  constexpr static std::size_t m_bufferSize = 8 * 1024;
  constexpr static std::size_t m_chunkSize = 1024;

  std::unique_ptr<char[]> m_buffer = std::make_unique<char[]>(m_bufferSize); // this has to be unique, aka deleted at class destructor
  std::size_t m_bufferLength = 0; 

};
