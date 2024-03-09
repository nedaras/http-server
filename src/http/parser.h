#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

// TODO: if node's http parser is fast we can just implement it here
//       we still dont handle querues and dont check if an url even is logical, but node's parser would handle that for use

namespace http_parser
{

  enum PARSER_RESPONSE
  {
    PARSER_RESPONSE_COMPLETE,
    PARSER_RESPONSE_PARSING,
    PARSER_RESPONSE_ERROR
  };

  struct Header
  { 

    std::string_view key;
    std::string_view value;

  };


  class Parser
  {

public:

    Parser() = default;
    Parser(char* buffer) : m_buffer(buffer), m_unhandledBuffer(buffer) {};

    using HeaderCallback = std::function<bool(std::string_view key, std::string_view value)>;
    std::tuple<PARSER_RESPONSE, std::size_t> parse_http(std::size_t bytes, std::string_view& method, std::string_view& path, const HeaderCallback& callback);

    void clear(char* buffer);

private:

    enum STATE : std::uint8_t 
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

    char* m_buffer;
    char* m_unhandledBuffer;

    std::string_view m_headerKey;

    STATE m_state = REQUEST_METHOD;

  };

}
