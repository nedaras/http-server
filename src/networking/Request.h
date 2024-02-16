#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include "../http/parser.h"

enum RESPONSE_STATUS : char // make into parser errors or sum
{
  REQUEST_SUCCESS,
  REQUEST_TIMEOUT,
  REQUEST_CLOSE
};

class Request
{

public:

  Request(int socket);

  std::optional<std::string_view> getHeader(std::string_view header) const;
  
  int constexpr getSocket() const
  {
    return m_socket;
  }

  int parse();

public:

  std::atomic<bool> dead = false;

private:

  int m_socket;
  http::Parser m_parser;// i dont like to refrence it 
  
  constexpr static std::size_t m_bufferSize = 8 * 1024;
  constexpr static std::size_t m_chunkSize = 1024;

  std::unique_ptr<char[]> m_buffer = std::make_unique<char[]>(m_bufferSize); // this has to be unique, aka deleted at class destructor
  std::size_t m_bufferLength = 0; 

  RESPONSE_STATUS m_status = REQUEST_SUCCESS;

};
