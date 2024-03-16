#pragma once

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

namespace http
{

  using data_callback = std::function<bool(std::optional<std::string_view>)>;

  enum READ_RESPONSE : std::uint8_t
  {
    READ_RESPONSE_DONE,
    READ_RESPONSE_WAITING,
    READ_RESPONSE_SOCKET_ERROR,
    READ_RESPONSE_BUFFER_ERROR,
    READ_RESPONSE_PARSING_ERROR,
    READ_RESPONSE_CLOSE
  };

  class request;
  class chunk_packet 
  {

    public:

      chunk_packet(const request* request, const data_callback& callback) : m_request(request), m_callback(std::move(callback)) {};

      void copy_buffer(const char* buffer, std::size_t size, std::uint32_t chunk_size, std::uint8_t chunk_characters, std::size_t bytes_received);

      void handle_chunk();
      void handle_chunk(std::optional<std::string_view> data);

      READ_RESPONSE read();

      void clear();

    private:

      std::size_t m_raw_size() const
      {
        return m_chunk_characters + 2 + m_chunk_size + 2;
      }

    private:

      std::vector<char> m_buffer;
      std::size_t m_bytes_received = 0;

      std::uint32_t m_chunk_size = 0;
      std::uint8_t m_chunk_characters = 0;
      static constexpr std::uint8_t m_max_chunk_characters = 5; // would be nice if we could allow users to modify

      const request* m_request;
      data_callback m_callback;

  };
}

