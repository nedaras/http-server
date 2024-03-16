#include "request.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <tuple>
#include <utility>
#include <vector>
#include "chunk_packet.h"
#include "server.h"
#include "../siphash/siphash.h"

// TODO: first we need to check if Chunked encoding is sent if not throw send empty optional
void http::request::read_data(const http::data_callback& callback) const
{

  if (m_receiving_data()) [[unlikely]]
  {
    std::cout << "bro readData call is allready set ok\n";
    return;
  }

  std::optional<std::string_view> value = m_get_headers_value("Transfer-Encoding");

  if (!value)
  {
    callback({});
    return;
  }

  char* buffer = m_buffer->data() + m_http_size;
  std::size_t bytes = m_buffer_offset - m_http_size;

  const char* max = &buffer[bytes];
  while (max > buffer)
  {

    std::uint32_t size = 0;
    std::uint8_t characters = 0;
    std::size_t bytes_received = 0;

    // TODO: idk we can make Packet struct, tha would like contain buffer and a size, and like expected max size idk
    // pass max_chunk size 0x10000
    auto [ status, bytes_read ] = m_http_parser.parse_chunk(buffer, bytes, size, characters, bytes_received);

    // after a loop we need to check if request is completed so we dont do stupid shit
    // while loop and shit, on last while loop make the chunk packet
    switch (status)
    {
    case PARSER_RESPONSE_COMPLETE:

      callback(std::string_view(buffer + characters + 2, size));
      m_http_parser.clear_chunk();

      break;
    case PARSER_RESPONSE_PARSING:
    {

      // it probably would be dirty af, but why not pass to a callback chunk that we only have now?
      // and then when we will recv more data chunk that to it would be dirty, but tbh we shouldnt rly need to allocate much data
      // just tu track where \r\n started and \r\n ended
      m_chunk_packet = std::make_unique<http::chunk_packet>(this, std::move(callback));
      m_chunk_packet->copy_buffer(buffer, bytes_read, size, characters, bytes_received);

      break;
    }
    case PARSER_RESPONSE_ERROR:

      // how to like make SERVER close connection ater this
      callback({});
      m_http_parser.clear_chunk();
      return;
    }

    buffer += bytes_read;
    bytes -= bytes_read;

  }

}

// TODO: make a function that would like get from 200 - OK or like 500 - Server Error messages
// TODO: make it throw error if status is already set
void http::request::set_status(std::uint16_t status) const
{

  if (m_response.status_sent) return;

  send(m_socket, "HTTP/1.1 ", strlen("HTTP/1.1 "), 0);
  send(m_socket, std::to_string(status).data(), std::to_string(status).size(), 0);
  send(m_socket, "\r\n", 2, 0);

  m_response.status_sent = true;

}

void http::request::set_head(std::string_view key, std::string_view value) const
{

  std::uint64_t hash = m_hash_string(key);

  // THREAD_LOCK
  if (m_header_sent(hash)) [[unlikely]]
  {

    std::cout << "why the fuck u sending same headers\n";

    return;

  }

  m_set_head(key, value, hash);

}

void http::request::m_set_head(std::string_view key, std::string_view value, std::uint64_t hash) const
{

  set_status(200);

  send(m_socket, key.data(), key.size(), 0);
  send(m_socket, ": ", 2, 0);
  send(m_socket, value.data(), value.size(), 0);
  send(m_socket, "\r\n", 2, 0);

  // THREAD_LOCK
  m_response.sent_headers.push_back(hash);

}

void http::request::write_body(std::string_view buffer) const
{
  write_body(buffer.data(), buffer.size());
}
// NOTE: mb add err handling, if we call write body twice?
void http::request::write_body(const char* buffer, std::size_t size) const
{

  m_set_date();
  m_set_connection();
  m_set_keep_alive();
  m_set_content_length(size);

  send(m_socket, "\r\n", 2, 0);
  send(m_socket, buffer, size, 0);

}

// THREAD_LOCK
void http::request::end() const
{
  
  m_update_timeout(5000);
  const_cast<http::request*>(this)->m_reset(); // red fucking flag
  
}

void http::request::m_set_date() const
{

  static std::uint64_t hash = m_hash_string("Date");
  if (m_header_sent(hash)) return; 

  // for null termination
  char date[29 + 1];

  std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); 
  std::strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));

  m_set_head("Date", date, hash);

}

void http::request::m_set_content_length(std::size_t length) const
{
  static std::uint64_t hash = m_hash_string("Content-Length");
  if (!m_header_sent(hash)) m_set_head("Content-Length", std::to_string(length), hash);
}

void http::request::m_set_connection() const
{
  static std::uint64_t hash = m_hash_string("Connection");
  if (!m_header_sent(hash)) m_set_head("Connection", "keep-alive", hash);
}

// TODO: only send keep alive if connection is keep alive
void http::request::m_set_keep_alive() const
{
  static std::uint64_t hash = m_hash_string("Keep-Alive");
  if (!m_header_sent(hash)) m_set_head("Keep-Alive", "timeout=5", hash);
}

// THREAD_LOCK
void http::request::m_update_timeout(std::time_t milliseconds) const
{

  m_server->m_timeouts.erase(this);
  m_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) + std::chrono::milliseconds(milliseconds);
  m_server->m_timeouts.push(this);

}

std::uint64_t http::request::m_hash_string(std::string_view string) const
{
  std::uintptr_t key = reinterpret_cast<std::uintptr_t>(m_server->m_callback.target<void>());
  return siphash::siphash_xy(string.data(), string.size(), 1, 3, reinterpret_cast<std::uintptr_t>(m_server), key);
}

// THREAD_LOCK
bool http::request::m_header_sent(std::uint64_t header_hash) const
{

  for (std::uint64_t hash : m_response.sent_headers) if (header_hash == hash) return true;
  
  return false;

}

bool http::request::m_receiving_data() const
{
  return m_chunk_packet ? true : false;
}


std::optional<std::string_view> http::request::m_get_headers_value(std::string_view key) const
{

  using header = std::tuple<std::string_view, std::string_view>;
  std::vector<header>::const_iterator iterator = std::find_if(headers.begin(), headers.end(), [&](const header& head) {
    return std::get<0>(head) == key;
  });

  if  (iterator == headers.end()) return {};
  return std::get<1>(*iterator);

}

// return like a bool if connection needs to be closed
http::READ_RESPONSE http::request::m_read()
{

  // {HTTP}9\r\n123456789\r\n5\r\n12345\r\n0\r\n\r\n
  // dont forget to check if there is EWOULDBLOCK

  if (m_receiving_data()) return m_chunk_packet->read();

  if (m_buffer_offset >= m_buffer->size()) 
  {

    char byte;
    ssize_t bytes = recv(m_socket, &byte, 1, 0);

    return bytes == 0 ? READ_RESPONSE_CLOSE : READ_RESPONSE_BUFFER_ERROR;
  }

  ssize_t bytes = recv(m_socket, m_buffer->data() + m_buffer_offset, m_buffer->size() - m_buffer_offset, 0);

  if (bytes == 0) return READ_RESPONSE_CLOSE;
  if (bytes == -1) return READ_RESPONSE_SOCKET_ERROR;

  // idk how to feel that its here
  if (m_response.completed)
  {
    m_response.completed = false;
  }

  m_buffer_offset += static_cast<std::size_t>(bytes);

  auto [ status, bytes_read ] = m_http_parser.parse_http(static_cast<std::size_t>(bytes), method, path, [this](std::string_view key, std::string_view value) {
    
    headers.push_back(std::make_pair(key, value));

    return true;

  });

  m_http_size += bytes_read;

  switch (status)
  {
    case PARSER_RESPONSE_COMPLETE:
      return READ_RESPONSE_DONE;
    case PARSER_RESPONSE_PARSING:
      return READ_RESPONSE_WAITING;
    default:
      return READ_RESPONSE_PARSING_ERROR;
  }

  return READ_RESPONSE_PARSING_ERROR;

}

void http::request::m_reset()
{

  m_response = Response();
  m_response.completed = true;
  m_chunk_packet.reset();

  headers.clear();

  m_http_parser.clear(m_buffer->data());
  m_buffer_offset = 0;
  m_http_size = 0;

}
