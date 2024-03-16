#include "parser.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>

// TODO: make return values an error return values
// TODO: make some standards like what chars can be used idk utf-8 if im not bored
// chunked encoding to

// we need to bench mark these arrays couse making it 256 bytes altough would be bigger i think to acess it would be very fast
static constexpr const std::uint8_t tokens[32] = { // wait http allows utf-8, thats pizda, these tokens ar not the standard, btw we dont use toekns
  0 | 0 | 0 | 0 | 0  | 0  | 0  | 0, 
  0 | 0 | 0 | 0 | 0  | 0  | 0  | 0, 
  0 | 0 | 0 | 0 | 0  | 0  | 0  | 0,
  0 | 0 | 0 | 0 | 0  | 0  | 0  | 0,
  1 | 2 | 0 | 8 | 16 | 32 | 64 | 128,
  0 | 0 | 4 | 8 | 0  | 32 | 64 | 0,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 0 | 0 | 0  | 0  | 0  | 0,
  0 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 0 | 0  | 0  | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 0 | 16 | 0  | 64 | 0,
}; 

static constexpr const std::uint8_t url_tokens[32] = {
  0 | 0 | 0 | 0 | 0  | 0  | 0  | 0, 
  0 | 0 | 0 | 0 | 0  | 0  | 0  | 0, 
  0 | 0 | 0 | 0 | 0  | 0  | 0  | 0,
  0 | 0 | 0 | 0 | 0  | 0  | 0  | 0,
  0 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 128,
  1 | 2 | 4 | 8 | 16 | 32 | 64 | 0,
};

static constexpr bool IS_UPPER_ALPHA(char c)
{
  return c >= 'A' && c <= 'Z';
}

static constexpr bool IS_HEADER_CHAR(char c)
{
  return c == 9 || (static_cast<unsigned char>(c) > 31 && c != 127);
}

static constexpr bool IS_TOKEN(char c)
{
  return tokens[static_cast<std::uint8_t>(c) >> 3] & (1 << (static_cast<std::uint8_t>(c) & 7));
}

static constexpr bool IS_NUMBER(char c)
{
  return c >= '0' && c <= '9';
}

static constexpr bool IS_URL_CHAR(char c)
{
  return url_tokens[static_cast<std::uint8_t>(c) >> 3] & (1 << (static_cast<std::uint8_t>(c) & 7));
}

static constexpr std::int8_t unhex_table[256] = {
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
  -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

static constexpr std::int8_t unhex(char c)
{
  return unhex_table[static_cast<std::uint8_t>(c)];
}

// if would want to add like some special state of headers, when using POST requests we will have to refactor this code
// to make it more simple to obtain

#define TUPLE(i) std::make_tuple(i, bytes_read);

// it only parsed the headers which is cool but u know
std::tuple<http::PARSER_RESPONSE, std::size_t> http::parser::parse_http(std::size_t bytes, std::string_view& method, std::string_view& path, const header_callback& callback)
{

  char* end = &m_buffer[bytes];
  std::size_t bytes_read = 0;

  while (m_buffer != end)
  {

    bytes_read++;

    switch (m_http_state)
    {
    case REQUEST_METHOD:
      if (*m_buffer != ' ') 
      {
        if (!IS_UPPER_ALPHA(*m_buffer)) return TUPLE(PARSER_RESPONSE_ERROR);
        break;
      }
      method = std::string_view(m_unhandled_buffer, static_cast<std::size_t>(m_buffer - m_unhandled_buffer));
      m_http_state = REQUEST_PATH_BEGIN;
      break;
    case REQUEST_PATH_BEGIN:
      if (*m_buffer != '/') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_PATH;
      m_unhandled_buffer = m_buffer;
      break;
    case REQUEST_PATH:
      if (*m_buffer != ' ')
      {
        if (!IS_URL_CHAR(*m_buffer)) return TUPLE(PARSER_RESPONSE_ERROR);// mb just mb parse url too
        break;
      }
      path = std::string_view(m_unhandled_buffer, static_cast<std::size_t>(m_buffer - m_unhandled_buffer));
      m_http_state = REQUEST_H;
      break;
    case REQUEST_H:
      if (*m_buffer != 'H') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HT;
      break;
    case REQUEST_HT: 
      if (*m_buffer != 'T') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HTT;
      break;
    case REQUEST_HTT: 
      if (*m_buffer != 'T') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HTTP;
      break;
    case REQUEST_HTTP:
      if (*m_buffer != 'P') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HTTP_DASH; 
      break;
    case REQUEST_HTTP_DASH:
      if (*m_buffer != '/') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HTTP_MINOR; 
      break;
    case REQUEST_HTTP_MINOR:
      if (*m_buffer != '1') return TUPLE(PARSER_RESPONSE_ERROR); // we need other versions aa:w
      m_http_state = REQUEST_HTTP_DOT; 
      break;
    case REQUEST_HTTP_DOT:
      if (*m_buffer != '.') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HTTP_MAJOR; 
      break;
    case REQUEST_HTTP_MAJOR:
      if (*m_buffer != '1') return TUPLE(PARSER_RESPONSE_ERROR); // we will only accept 1.1 for now
      m_http_state = REQUEST_HTTP_ALMOST_END; 
      break;
    case REQUEST_HTTP_ALMOST_END:
      if (*m_buffer != '\r') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HTTP_END;
      break;
    case REQUEST_HTTP_END:
      if (*m_buffer != '\n') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HEADER_KEY_BEGIN;
      break;
    case REQUEST_HEADER_KEY_BEGIN:
      if (*m_buffer == '\r')
      {
        m_http_state = REQUEST_EOF;
        break;
      }

      // make diffrent struct that accepts A-Z;a-z;0-9;-;_;
      if (!IS_TOKEN(*m_buffer)) return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HEADER_KEY;
      m_unhandled_buffer = m_buffer;
      break;
    case REQUEST_HEADER_KEY:
      if (*m_buffer != ':') 
      {
        if (!IS_TOKEN(*m_buffer)) return TUPLE(PARSER_RESPONSE_ERROR); // is token kinda idk idk sucks
        break; 
      }
      m_header_key = std::string_view(m_unhandled_buffer, static_cast<std::size_t>(m_buffer - m_unhandled_buffer));
      m_http_state = REQUEST_HEADER_KEY_END;
      break;
    case REQUEST_HEADER_KEY_END:
      if (*m_buffer != ' ') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HEADER_VALUE_BEGIN;
      break;
    case REQUEST_HEADER_VALUE_BEGIN:
      if (!IS_HEADER_CHAR(*m_buffer)) return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HEADER_VALUE;
      m_unhandled_buffer = m_buffer;
      break;
    case REQUEST_HEADER_VALUE:
      if (*m_buffer == '\r')
      {
        std::string_view headerValue(m_unhandled_buffer, static_cast<std::size_t>(m_buffer - m_unhandled_buffer));
        if (!callback(m_header_key, headerValue)) return TUPLE(PARSER_RESPONSE_ERROR);
        m_http_state = REQUEST_HEADER_END;
        break;
      }
      if (!IS_HEADER_CHAR(*m_buffer)) return TUPLE(PARSER_RESPONSE_ERROR);
      break;
    case REQUEST_HEADER_END:
      if (*m_buffer != '\n') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HEADER_KEY_BEGIN;
      break;
    case REQUEST_EOF:
      return TUPLE(*m_buffer == '\n' ? PARSER_RESPONSE_COMPLETE : PARSER_RESPONSE_ERROR);
    }
  
    m_buffer++;

  }

  return TUPLE(PARSER_RESPONSE_PARSING);

}

// TODO: we need like max characters broski, mb add a callback function that would call with updates size,
//       if we dont like size we can like return false on callback
std::tuple<http::PARSER_RESPONSE, std::size_t> http::parser::parse_chunk(char* buffer, std::size_t bytes, std::uint32_t& size, std::uint8_t& characters, std::size_t& bytes_received)
{

  char* end = &buffer[bytes];
  std::size_t bytes_read = 0;

  while (buffer != end)
  {

    bytes_read++;

    switch (m_chunk_state)
    {
    case CHUNK_SIZE:
      if (unhex(*buffer) != -1)
      {

        // magic number ye, it means that we will only parse five chunk size characters, why?
        // so we dont let chuks that are over 0x99999 pass this, fax it that the point is to only allow
        // 0x10000 chunk sizes which is 5 chars
        if (characters == 5) return TUPLE(PARSER_RESPONSE_ERROR);

        size = size * 16 + static_cast<std::uint32_t>(unhex(*buffer));
        characters++;

        break;
      }

      if (*buffer != '\r') return TUPLE(PARSER_RESPONSE_ERROR);

      m_chunk_size_parsed = true;
      m_chunk_state = CHUNK_BEGIN;

      break;
    case CHUNK_BEGIN:
      if (*buffer != '\n') return TUPLE(PARSER_RESPONSE_ERROR);
      m_chunk_state = CHUNK_BODY;
      break;
    case CHUNK_BODY:
    {

      bytes_read--;
      buffer--;

      std::size_t bytes_to_skip = size - bytes_received;
      std::size_t bytes_skipped = std::min(bytes - bytes_read, bytes_to_skip);

      bytes_read += bytes_skipped;
      buffer += bytes_skipped;

      bytes_received += bytes_skipped;

      if (bytes_to_skip == bytes_skipped) m_chunk_state = CHUNK_END;

      break;
    }
    case CHUNK_END:
      if (*buffer != '\r') return TUPLE(PARSER_RESPONSE_ERROR);
      m_chunk_state = CHUNK_EOF;
      break;
    case CHUNK_EOF:
      return TUPLE(*buffer == '\n' ? PARSER_RESPONSE_COMPLETE : PARSER_RESPONSE_ERROR);
    }

    buffer++;

  }

  // would be nice to know if chunkSize is completed
  return TUPLE(PARSER_RESPONSE_PARSING);

}

void http::parser::clear_chunk()
{

  m_chunk_state = CHUNK_SIZE;
  m_chunk_size_parsed = false;

}

void http::parser::clear(char* buffer)
{

  m_buffer = buffer;
  m_unhandled_buffer = buffer;

  m_http_state = REQUEST_METHOD;
  
  clear_chunk();

}
