#include "Request.h"

#include <cstring>
#include <string>
#include <sys/socket.h>

#include "Server.h"

// TODO: make a function that would like get from 200 - OK or like 500 - Server Error messages
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

  setStatus(200);

  send(m_socket, key.data(), key.size(), 0);
  send(m_socket, ": ", 2, 0);
  send(m_socket, value.data(), value.size(), 0);
  send(m_socket, "\r\n", 2, 0);

}

void Request::writeBody(std::string_view buffer) const
{
  writeBody(buffer.data(), buffer.size());
}
// NOTE: mb add err handling, if we call write body twice?
void Request::writeBody(const char* buffer, std::size_t size) const
{

  m_setDate();
  m_setContentLength(size);
  m_setKeepAlive();
  // m_setTimeout();

  send(m_socket, "\r\n", 2, 0);
  send(m_socket, buffer, size, 0);

}

void Request::end() const
{

  m_updateTimeout(5000);
  m_response = {};

}

void Request::m_setDate() const
{
  setHead("Date", "pizda_naxui");
}

// intresting idea to store hashes of headers we have sent
void Request::m_setContentLength(std::size_t length) const
{
  setHead("Content-Length", std::to_string(length));
}

void Request::m_setKeepAlive() const
{
  setHead("Connection", "keep-alive");
}
// THREAD_LOCK
void Request::m_updateTimeout(std::chrono::milliseconds::rep milliseconds) const
{

  m_server->m_timeouts.erase(this);
  m_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) + std::chrono::milliseconds(milliseconds);
  m_server->m_timeouts.push(this);

}
