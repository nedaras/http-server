#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "../parser/parser.h"
#include "chunk_packet.h"

// TODO: bro dont do request as a const, not cool

namespace http
{

  class server;
  class request
  {

    public:

      void read_data(const http::data_callback& callback) const;

      void set_status(std::uint16_t status) const;

      void set_head(std::string_view key, std::string_view value) const;

      void write_body(std::string_view buffer) const;
      void write_body(const char* buffer, std::size_t size) const;

      void end() const;

    private:

      request(int socket, http::server* server) : m_socket(socket), m_server(server), m_http_parser(m_buffer->data()) {};

      void m_set_head(std::string_view key, std::string_view value, std::uint64_t hash) const;

      void m_set_date() const;

      void m_set_content_length(std::size_t length) const;

      void m_set_connection() const;

      void m_set_keep_alive() const;

      void m_update_timeout(std::time_t milliseconds) const;

      READ_RESPONSE m_read();

      void m_reset();

      std::uint64_t m_hash_string(std::string_view string) const;

      bool m_header_sent(std::uint64_t headerHash) const;

      bool m_receiving_data() const;

      std::optional<std::string_view> m_get_headers_value(std::string_view key) const;

    public:

      std::string_view method;
      std::string_view path;

      std::vector<std::tuple<std::string_view, std::string_view>> headers; // set doesent like string_view idk why

    private:

      struct Response
      {
        bool completed : 1;
        bool status_sent : 1;
        std::vector<std::uint64_t> sent_headers;
      };

      friend class http::server;
      friend class http::chunk_packet;

      int m_socket;
      http::server* m_server;

      std::size_t m_buffer_offset = 0;
      std::size_t m_http_size = 0;

      std::unique_ptr<std::array<char, 8 * 1024>> m_buffer = std::make_unique<std::array<char, 8 * 1024>>();

      mutable std::unique_ptr<http::chunk_packet> m_chunk_packet;
      mutable http::parser m_http_parser;
      mutable std::chrono::milliseconds m_timeout;
      mutable Response m_response {};

  };
}

