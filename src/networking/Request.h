#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>

class Server;
class Request
{

public:

  void setStatus(std::uint16_t status) const;

  void setHead(std::string_view key, std::string_view value) const;

  void writeBody(std::string_view buffer) const;
  void writeBody(const char* buffer, std::size_t size) const;

  void end() const;

private:

  Request(int socket, Server* server) : m_socket(socket), m_server(server) {};

  void m_setDate() const;

  void m_setContentLength(std::size_t length) const;

  void m_setKeepAlive() const;

  void m_updateTimeout(std::chrono::milliseconds::rep milliseconds) const;

  int m_recv();

  std::uint64_t m_hashString(std::string_view string) const;

private:

  struct Response
  {
    bool statusSent : 1;
  };

  friend class Server;

  int m_socket;
  Server* m_server;

  std::size_t m_bufferOffset = 0;
  std::unique_ptr<std::array<char, 8 * 1024>> m_buffer = std::make_unique<std::array<char, 8 * 1024>>();

  mutable std::chrono::milliseconds m_timeout;

  mutable Response m_response;

};
