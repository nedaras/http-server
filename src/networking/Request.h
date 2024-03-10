#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "../http/parser.h"

// TODO: bro dont do request as a const, not cool

class Server;
class Request
{

public:

  using DataCallback = std::function<bool(std::optional<std::string_view>)>;
  void readData(const DataCallback& callback) const;

  void setStatus(std::uint16_t status) const;

  void setHead(std::string_view key, std::string_view value) const;

  void writeBody(std::string_view buffer) const;
  void writeBody(const char* buffer, std::size_t size) const;

  void end() const;

private:

  Request(int socket, Server* server) : m_socket(socket), m_server(server), m_httpParser(m_buffer->data()) {};

  void m_setHead(std::string_view key, std::string_view value, std::uint64_t hash) const;

  void m_setDate() const;

  void m_setContentLength(std::size_t length) const;

  void m_setConnection() const;

  void m_setKeepAlive() const;

  void m_updateTimeout(std::time_t milliseconds) const;

  int m_recv();

  int m_recvChunks();

  void m_reset();

  std::uint64_t m_hashString(std::string_view string) const;

  bool m_headerSent(std::uint64_t headerHash) const;

  bool m_receivingData() const;

  std::optional<std::string_view> m_getHeadersValue(std::string_view key) const;

public:

  std::string_view method;
  std::string_view path;

  std::vector<std::tuple<std::string_view, std::string_view>> headers; // set doesent like string_view idk why

private:

  struct Response
  {
    bool completed : 1;
    bool statusSent : 1;
    std::vector<std::uint64_t> sentHeaders;
  };

  struct ChunkPacket
  {

    void reserveAndCopy(const char* buf, std::size_t size)
    {
      if (size > chunkCharacters + 2 + chunkSize + 2)
      {
        std::cout << "we cant copy couse bro size is to small.\n";
        return;
      }

      buffer.reserve(chunkCharacters + 2 + chunkSize + 2);
      buffer.append(buf, size);
    }

    char* offseted()
    {
      return buffer.data() + bufferOffset;
    }

    std::size_t left() const
    {
      return buffer.capacity() - buffer.size();
    }

    std::string buffer;
    std::size_t bufferOffset = 0;
    std::uint32_t chunkSize = 0;
    std::uint8_t chunkCharacters = 0;
  };

  friend class Server;

  int m_socket;
  Server* m_server;

  std::size_t m_bufferOffset = 0;
  std::size_t m_httpSize = 0;

  std::unique_ptr<std::array<char, 8 * 1024>> m_buffer = std::make_unique<std::array<char, 8 * 1024>>();
  mutable std::unique_ptr<ChunkPacket> m_chunkPacket;

  mutable http_parser::Parser m_httpParser;
  mutable std::chrono::milliseconds m_timeout;
  mutable Response m_response {};
  mutable std::optional<DataCallback> m_dataCallback;

};
