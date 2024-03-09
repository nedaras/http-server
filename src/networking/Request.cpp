#include "Request.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <tuple>

#include "Server.h"
#include "../siphash/siphash.h"

// TODO: first we need to check if Chunked encoding is sent if not throw send empty optional
void Request::readData(const DataCallback& callback) const
{

  if (m_receivingData())
  {

    std::cout << "bro readData call is allready set ok\n";

    return;
  }

  char* chunk = m_buffer->data() + m_httpSize;
  std::size_t chunkSize = m_bufferOffset - m_httpSize;

  // we have to parse chunks and stuff

  if (chunkSize > 0) 
  {
    callback(std::string_view(chunk, chunkSize));
  }

  m_dataCallback = std::move(callback);

}

// TODO: make a function that would like get from 200 - OK or like 500 - Server Error messages
// TODO: make it throw error if status is already set
void Request::setStatus(std::uint16_t status) const
{

  if (m_response.statusSent) return;

  send(m_socket, "HTTP/1.1 ", strlen("HTTP/1.1 "), 0);
  send(m_socket, std::to_string(status).data(), std::to_string(status).size(), 0);
  send(m_socket, "\r\n", 2, 0);

  m_response.statusSent = true;

}

void Request::setHead(std::string_view key, std::string_view value) const
{

  std::uint64_t hash = m_hashString(key);

  // THREAD_LOCK
  if (m_headerSent(hash)) [[unlikely]]
  {

    std::cout << "why the fuck u sending same headers\n";

    return;

  }

  m_setHead(key, value, hash);

}

void Request::m_setHead(std::string_view key, std::string_view value, std::uint64_t hash) const
{

  setStatus(200);

  send(m_socket, key.data(), key.size(), 0);
  send(m_socket, ": ", 2, 0);
  send(m_socket, value.data(), value.size(), 0);
  send(m_socket, "\r\n", 2, 0);

  // THREAD_LOCK
  m_response.sentHeaders.push_back(hash);

}

void Request::writeBody(std::string_view buffer) const
{
  writeBody(buffer.data(), buffer.size());
}
// NOTE: mb add err handling, if we call write body twice?
void Request::writeBody(const char* buffer, std::size_t size) const
{

  m_setDate();
  m_setConnection();
  m_setKeepAlive();
  m_setContentLength(size);

  send(m_socket, "\r\n", 2, 0);
  send(m_socket, buffer, size, 0);

}

// THREAD_LOCK
void Request::end() const
{
  
  m_updateTimeout(5000);
  const_cast<Request*>(this)->m_reset(); // red fucking flag
  
}

void Request::m_setDate() const
{

  static std::uint64_t hash = m_hashString("Date");
  if (m_headerSent(hash)) return; 

  // for null termination
  char date[29 + 1];

  std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); 
  std::strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));

  m_setHead("Date", date, hash);

}

void Request::m_setContentLength(std::size_t length) const
{
  static std::uint64_t hash = m_hashString("Content-Length");
  if (!m_headerSent(hash)) m_setHead("Content-Length", std::to_string(length), hash);
}

void Request::m_setConnection() const
{
  static std::uint64_t hash = m_hashString("Connection");
  if (!m_headerSent(hash)) m_setHead("Connection", "keep-alive", hash);
}

// TODO: only send keep alive if connection is keep alive
void Request::m_setKeepAlive() const
{
  static std::uint64_t hash = m_hashString("Keep-Alive");
  if (!m_headerSent(hash)) m_setHead("Keep-Alive", "timeout=5", hash);
}

// THREAD_LOCK
void Request::m_updateTimeout(std::time_t milliseconds) const
{

  m_server->m_timeouts.erase(this);
  m_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) + std::chrono::milliseconds(milliseconds);
  m_server->m_timeouts.push(this);

}

std::uint64_t Request::m_hashString(std::string_view string) const
{
  std::uintptr_t key = reinterpret_cast<std::uintptr_t>(m_server->m_callback.target<void>());
  return siphash::siphash_xy(string.data(), string.size(), 1, 3, reinterpret_cast<std::uintptr_t>(m_server), key);
}

// THREAD_LOCK
bool Request::m_headerSent(std::uint64_t headerHash) const
{

  for (std::uint64_t hash : m_response.sentHeaders) if (headerHash == hash) return true;
  
  return false;

}

bool Request::m_receivingData() const
{
  return m_dataCallback.has_value();
}

// return like a bool if connection needs to be closed
int Request::m_recv()
{

  // {HTTP}9\r\n123456789\r\n5\r\n12345\r\n0\r\n\r\n

  if (m_receivingData())
  {

    char buf[512];

    ssize_t bytes = recv(m_socket, buf, sizeof(buf), 0);
    
    if (bytes == 0)
    {
      std::cout << "CLOSE\n";
      return -1;
    }

    if (bytes == -1)
    {
      std::cout << "ERR\n";
      return -1;
    }

    (*m_dataCallback)(std::string_view(buf, bytes)); 

    return 2;

  }

  if (m_bufferOffset >= m_buffer->size())
  {

  }

  ssize_t bytes = recv(m_socket, m_buffer->data() + m_bufferOffset, m_buffer->size() - m_bufferOffset, 0);

  if (bytes == 0)
  {
    std::cout << "CLOSE\n";
    return -1;
  }

  if (bytes == -1)
  {
    std::cout << "ERR\n";
    return -1;
  }

  if (m_response.completed)
  {

    m_response.completed = false;

  }

  auto [ a, b ] = m_httpParser.parse_http(bytes, method, path, [this](std::string_view key, std::string_view value) {
    
    headers.push_back(std::make_tuple(key, value));

    return true;

  });

  m_bufferOffset += bytes;
  m_httpSize += b;  

  std::cout << "recv: " << bytes << "\n";
  std::cout << "read: " << b << "\n";

  std::cout << "res: " << a << "\n";

  return a == http_parser::PARSER_RESPONSE_COMPLETE ? 0 : a == http_parser::PARSER_RESPONSE_PARSING ? 1 : -1;

}

void Request::m_reset()
{

  m_response = Response();
  m_response.completed = true;
  m_dataCallback.reset();

  headers.clear();

  m_httpParser.clear(m_buffer->data());
  m_bufferOffset = 0;
  m_httpSize = 0;

}
