#include "parser.h"
#include <cstdint>
#include <iostream>
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

#define TUPLE(i) std::make_tuple(i, bytesRead);

// it only parsed the headers which is cool but u know
std::tuple<PARSER_RESPONSE, std::size_t> 
http_parser::Parser::parse_http(std::size_t bytes, std::string_view& method, std::string_view& path, const HeaderCallback& callback)
{

  char* end = &m_buffer[bytes];
  std::size_t bytesRead = 0;

  while (m_buffer != end)
  {

    bytesRead++;

    switch (m_http_state)
    {
    case REQUEST_METHOD:
      if (*m_buffer != ' ') 
      {
        if (!IS_UPPER_ALPHA(*m_buffer)) return TUPLE(PARSER_RESPONSE_ERROR);
        break;
      }
      method = std::string_view(m_unhandledBuffer, m_buffer - m_unhandledBuffer);
      m_http_state = REQUEST_PATH_BEGIN;
      break;
    case REQUEST_PATH_BEGIN:
      if (*m_buffer != '/') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_PATH;
      m_unhandledBuffer = m_buffer;
      break;
    case REQUEST_PATH:
      if (*m_buffer != ' ')
      {
        if (!IS_URL_CHAR(*m_buffer)) return TUPLE(PARSER_RESPONSE_ERROR);// mb just mb parse url too
        break;
      }
      path = std::string_view(m_unhandledBuffer, m_buffer - m_unhandledBuffer);
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
      m_unhandledBuffer = m_buffer;
      break;
    case REQUEST_HEADER_KEY:
      if (*m_buffer != ':') 
      {
        if (!IS_TOKEN(*m_buffer)) return TUPLE(PARSER_RESPONSE_ERROR); // is token kinda idk idk sucks
        break; 
      }
      m_headerKey = std::string_view(m_unhandledBuffer, m_buffer - m_unhandledBuffer);
      m_http_state = REQUEST_HEADER_KEY_END;
      break;
    case REQUEST_HEADER_KEY_END:
      if (*m_buffer != ' ') return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HEADER_VALUE_BEGIN;
      break;
    case REQUEST_HEADER_VALUE_BEGIN:
      if (!IS_HEADER_CHAR(*m_buffer)) return TUPLE(PARSER_RESPONSE_ERROR);
      m_http_state = REQUEST_HEADER_VALUE;
      m_unhandledBuffer = m_buffer;
      break;
    case REQUEST_HEADER_VALUE:
      if (*m_buffer == '\r')
      {
        std::string_view headerValue(m_unhandledBuffer, m_buffer - m_unhandledBuffer);
        if (!callback(m_headerKey, headerValue)) return TUPLE(PARSER_RESPONSE_ERROR);
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
std::tuple<PARSER_RESPONSE, std::size_t> 
http_parser::Parser::parse_chunk(char* buffer, std::size_t bytes, std::uint32_t& size, std::uint8_t& characters, std::size_t& bytesReceived)
{

  char* end = &buffer[bytes];
  std::size_t bytesRead = 0;

  while (buffer != end)
  {

    bytesRead++;

    switch (m_chunk_state)
    {
    case CHUNK_SIZE:
      if (unhex(*buffer) != -1)
      {

        // magic number ye, it means that we will only parse five chunk size characters, why?
        // so we dont let chuks that are over 0x99999 pass this, fax it that the point is to only allow
        // 0x10000 chunk sizes which is 5 chars
        if (characters == 5) return TUPLE(PARSER_RESPONSE_ERROR);

        size = size * 16 + unhex(*buffer);
        characters++;

        break;
      }

      if (*buffer != '\r') return TUPLE(PARSER_RESPONSE_ERROR);

      m_chunkSizeParsed = true;
      m_chunk_state = CHUNK_BEGIN;

      break;
    case CHUNK_BEGIN:
      if (*buffer != '\n') return TUPLE(PARSER_RESPONSE_ERROR);
      m_chunk_state = CHUNK_BODY;
      break;
    case CHUNK_BODY:
    {

      bytesRead--;
      buffer--;

      std::size_t bytesToSkip = size - bytesReceived;
      std::size_t bytesSkipped = std::min(bytes - bytesRead, bytesToSkip);

      bytesRead += bytesSkipped;
      buffer += bytesSkipped;

      bytesReceived += bytesSkipped;

      if (bytesToSkip == bytesSkipped) m_chunk_state = CHUNK_END;

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

void http_parser::Parser::clearChunk()
{

  m_chunk_state = CHUNK_SIZE;
  m_chunkSizeParsed = false;

}

void http_parser::Parser::clear(char* buffer)
{

  m_buffer = buffer;
  m_unhandledBuffer = buffer;

  m_http_state = REQUEST_METHOD;
  
  clearChunk();

}
