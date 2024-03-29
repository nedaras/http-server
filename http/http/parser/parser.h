#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

// TODO: if node's http parser is fast we can just implement it here
//       we still dont handle querues and dont check if an url even is logical, but node's parser would handle that for use

namespace http
{

  enum PARSER_RESPONSE
  {
    PARSER_RESPONSE_COMPLETE,
    PARSER_RESPONSE_PARSING,
    PARSER_RESPONSE_ERROR
  };

  class parser
  {

  public:

    parser() = default;
    parser(char* buffer) : m_buffer(buffer), m_unhandled_buffer(buffer) {};

    using header_callback = std::function<bool(std::string_view key, std::string_view value)>;
    std::tuple<PARSER_RESPONSE, std::size_t> parse_http(std::size_t bytes, std::string_view& method, std::string_view& path, const header_callback& callback);

    std::tuple<PARSER_RESPONSE, std::size_t> parse_chunk(char* buffer, std::size_t bytes, std::uint32_t& size, std::uint8_t& characters, std::size_t& bytesReceived);

    constexpr bool chunk_size_parsed()
    {
      return m_chunk_size_parsed;
    }

    void clear_chunk();
    void clear(char* buffer);

  private:

    enum HTTP_PARSER_STATE : std::uint8_t 
    {
      REQUEST_METHOD,
      REQUEST_PATH_BEGIN,
      REQUEST_PATH,
      REQUEST_H,
      REQUEST_HT,
      REQUEST_HTT,
      REQUEST_HTTP,
      REQUEST_HTTP_DASH,
      REQUEST_HTTP_MINOR,
      REQUEST_HTTP_DOT,
      REQUEST_HTTP_MAJOR,
      REQUEST_HTTP_ALMOST_END,
      REQUEST_HTTP_END,
      REQUEST_HEADER_KEY_BEGIN,
      REQUEST_HEADER_KEY,
      REQUEST_HEADER_KEY_END,
      REQUEST_HEADER_VALUE_BEGIN, 
      REQUEST_HEADER_VALUE,
      REQUEST_HEADER_END,
      REQUEST_EOF
    };

    enum CHUNK_PARSER_STATE : std::uint8_t
    {
      CHUNK_SIZE,
      CHUNK_BEGIN,
      CHUNK_BODY,
      CHUNK_END,
      CHUNK_EOF
    };

    char* m_buffer;
    char* m_unhandled_buffer;

    std::string_view m_header_key;

    HTTP_PARSER_STATE m_http_state = REQUEST_METHOD;
    CHUNK_PARSER_STATE m_chunk_state = CHUNK_SIZE;

    bool m_chunk_size_parsed = false;

  };
}

