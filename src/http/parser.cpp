#include "parser.h"
#include <cstdint>

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

static constexpr bool IS_URL_CHAR(char c)
{
  return url_tokens[static_cast<std::uint8_t>(c) >> 3] & (1 << (static_cast<std::uint8_t>(c) & 7));
}

// if would want to add like some special state of headers, when using POST requests we will have to refactor this code
// to make it more simple to obtain
int http::Parser::parse(std::size_t bytes) // mb build unit tests
{

  char* end = &m_buffer[bytes];
 
  while (m_buffer != end)
  {
    switch (m_state)
    {
    case REQUEST_METHOD:
      if (*m_buffer != ' ') 
      {
        if (!IS_UPPER_ALPHA(*m_buffer)) return -1; // mb validate if request string is exsisting
        break;
      }
      method = std::string_view(m_unhandledBuffer, m_buffer - m_unhandledBuffer);
      m_state = REQUEST_PATH_BEGIN;
      break;
    case REQUEST_PATH_BEGIN:
      if (*m_buffer != '/') return -1;
      m_state = REQUEST_PATH;
      m_unhandledBuffer = m_buffer;
      break;
    case REQUEST_PATH:
      if (*m_buffer != ' ')
      {
        if (!IS_URL_CHAR(*m_buffer)) return -1;// mb just mb parse url too
        break;
      }
      path = std::string_view(m_unhandledBuffer, m_buffer - m_unhandledBuffer);
      m_state = REQUEST_H;
      break;
    case REQUEST_H:
      if (*m_buffer != 'H') return -1;
      m_state = REQUEST_HT;
      break;
    case REQUEST_HT: 
      if (*m_buffer != 'T') return -1;
      m_state = REQUEST_HTT;
      break;
    case REQUEST_HTT: 
      if (*m_buffer != 'T') return -1;
      m_state = REQUEST_HTTP;
      break;
    case REQUEST_HTTP:
      if (*m_buffer != 'P') return -1;
      m_state = REQUEST_HTTP_DASH; 
      break;
    case REQUEST_HTTP_DASH:
      if (*m_buffer != '/') return -1;
      m_state = REQUEST_HTTP_MINOR; 
      break;
    case REQUEST_HTTP_MINOR:
      if (*m_buffer != '1') return -1; // we need other versions aa:w
      m_state = REQUEST_HTTP_DOT; 
      break;
    case REQUEST_HTTP_DOT:
      if (*m_buffer != '.') return -1;
      m_state = REQUEST_HTTP_MAJOR; 
      break;
    case REQUEST_HTTP_MAJOR:
      if (*m_buffer != '1') return -1; // we will only accept 1.1 for now
      m_state = REQUEST_HTTP_ALMOST_END; 
      break;
    case REQUEST_HTTP_ALMOST_END:
      if (*m_buffer != '\r') return -1;
      m_state = REQUEST_HTTP_END;
      break;
    case REQUEST_HTTP_END:
      if (*m_buffer != '\n') return -1;
      m_state = REQUEST_HEADER_KEY_BEGIN;
      break;
    case REQUEST_HEADER_KEY_BEGIN:
      if (*m_buffer == '\r')
      {
        m_state = REQUEST_EOF;
        break;
      }

      // make diffrent struct that accepts A-Z;a-z;0-9;-;_;
      if (!IS_TOKEN(*m_buffer)) return -1;
      m_state = REQUEST_HEADER_KEY;
      m_unhandledBuffer = m_buffer;
      break;
    case REQUEST_HEADER_KEY:
      if (*m_buffer != ':') 
      {
        if (!IS_TOKEN(*m_buffer)) return -1; // is token kinda idk idk sucks
        break; 
      }
      m_header.key = std::string_view(m_unhandledBuffer, m_buffer - m_unhandledBuffer);
      m_state = REQUEST_HEADER_KEY_END;
      break;
    case REQUEST_HEADER_KEY_END:
      if (*m_buffer != ' ') return -1;
      m_state = REQUEST_HEADER_VALUE_BEGIN;
      break;
    case REQUEST_HEADER_VALUE_BEGIN:
      if (!IS_HEADER_CHAR(*m_buffer)) return -1;
      m_state = REQUEST_HEADER_VALUE;
      m_unhandledBuffer = m_buffer;
      break;
    case REQUEST_HEADER_VALUE:
      if (*m_buffer == '\r')
      {
        m_header.value = std::string_view(m_unhandledBuffer, m_buffer - m_unhandledBuffer);
        headers.push_back(m_header);
        m_state = REQUEST_HEADER_END;

        break;
      }
      
      if (!IS_HEADER_CHAR(*m_buffer)) return -1;
      break;
    case REQUEST_HEADER_END:
      if (*m_buffer != '\n') return -1;
      m_state = REQUEST_HEADER_KEY_BEGIN;
      break;
    case REQUEST_EOF:
      return *m_buffer == '\n' ? 0 : -1;
    }
  
    m_buffer++;

  }

  return 1;

}
