#include "chunk_packet.h"

#include <iostream>
#include <string_view>
#include <sys/socket.h>

#include "request.h"

void http::chunk_packet::copy_buffer(const char* buffer, std::size_t size, std::uint32_t chunk_size, std::uint8_t chunk_characters, std::size_t bytes_received)
{

  m_chunk_size = chunk_size;
  m_chunk_characters = chunk_characters;
  m_bytes_received = bytes_received;

  if (m_request->m_http_parser.chunk_size_parsed())
  {
    
    m_buffer.reserve(m_raw_size());
    m_buffer.insert(m_buffer.end(), buffer, buffer + size);

    return;
  }

  m_buffer.reserve(m_max_chunk_characters);
  m_buffer.insert(m_buffer.end(), buffer, buffer + size);

}


void http::chunk_packet::handle_chunk()
{
  std::string_view chunk(m_buffer.data() + m_chunk_characters + 2, m_chunk_size);
  handle_chunk(chunk);
}

void http::chunk_packet::handle_chunk(std::optional<std::string_view> data)
{
  m_callback(data);
  clear();
}

// the fact that we're storing that 0\r\n and \r\n is fucking dumb our array istead of 65536 becomes 65545
// if we allow payload of 0x1000 size
http::READ_RESPONSE http::chunk_packet::read()
{

  if (!m_request->m_http_parser.chunk_size_parsed())
  {

    if (m_chunk_characters > m_max_chunk_characters)
    {
      std::cout << "why didn't we handled the chunk characters overflow\n";
      return READ_RESPONSE_BUFFER_ERROR;
    }

    {
      
      char buffer[m_max_chunk_characters];
      std::uint8_t buffer_size = m_max_chunk_characters - m_chunk_characters;

      ssize_t bytes = recv(m_request->m_socket, buffer, buffer_size, 0);

      if (bytes == 0) return READ_RESPONSE_CLOSE;
      if (bytes == -1) return READ_RESPONSE_SOCKET_ERROR;

      // TODO: paste the recv directly to m_buffer, i dont fo it now cus maybe this buffer will be overflown
      m_buffer.insert(m_buffer.end(), buffer, buffer + bytes);

      // YEYE its ugly we will fix it when we will rwite directly to the m_buffer
      auto [ status, bytesRead ] = m_request->m_http_parser.parse_chunk(m_buffer.data() + m_buffer.size() - bytes, static_cast<std::size_t>(bytes), m_chunk_size, m_chunk_characters, m_bytes_received);

      switch (status)
      {
      case PARSER_RESPONSE_COMPLETE:
        // remove this magic 2 plus bullshit in a function
        // and we have a problem the end chunk is 0\r\n\r\n which is 5 chars like our m_max_chunk_characters
        // it means that we dont have a problem, but lets say we habe max chars set to 6 and we have this nessage to revv:
        // 0\r\n\r\n3\r\n123\r\n, its a dumb chunk but idk mb that how it has to be, by reading 6 chars we will read "0\r\n\r\n3"
        // and we will forget about that read '3' char so wehen we will try to recv we will get "\r\n123\r\n"
        // and this will result an error saying that its a bad chunk
        if (static_cast<std::size_t>(bytes) > bytesRead)
        {

          std::cout << "shit we have some problems\n";

        }
        return READ_RESPONSE_DONE;
      case PARSER_RESPONSE_PARSING:
        if (!m_request->m_http_parser.chunk_size_parsed())
        {
          return READ_RESPONSE_WAITING;
        }
        if (m_raw_size() > m_buffer.capacity()) m_buffer.reserve(m_raw_size());
        break;
      default:
        return READ_RESPONSE_PARSING_ERROR;
      }

    }

    if (!m_request->m_http_parser.chunk_size_parsed())
    {

      std::cout << "how can it not even be parsed?????????\n";
      return READ_RESPONSE_PARSING_ERROR;

    }

  }

  char* buffer = m_buffer.data() + m_buffer.size();
  ssize_t bytes = recv(m_request->m_socket, buffer, m_buffer.capacity() - m_buffer.size(), 0);

  if (bytes == 0) return READ_RESPONSE_CLOSE;
  if (bytes == -1) return READ_RESPONSE_SOCKET_ERROR;

  // TODO: we will make out own buffer like wtf why cant i just call resize() ha? this is copying memory that we already have
  // AND FUCK RHIS IS EVIL, we can use our own size and fucking ignore std::size, a hot fix or just make my own array
  m_buffer.insert(m_buffer.end(), buffer, buffer + bytes);

  auto [ status, bytes_read ] = m_request->m_http_parser.parse_chunk(buffer, static_cast<std::size_t>(bytes), m_chunk_size, m_chunk_characters, m_bytes_received);

  switch (status)
  {
  case PARSER_RESPONSE_COMPLETE:
    return READ_RESPONSE_DONE;
  case PARSER_RESPONSE_PARSING:
    return READ_RESPONSE_WAITING;
  default:
    return READ_RESPONSE_PARSING_ERROR;
  }

  return READ_RESPONSE_PARSING_ERROR;

}

void http::chunk_packet::clear()
{

  // clear a vector that will sooner or later be allocated to same size is kinda no no
  m_buffer.clear();
  m_request->m_http_parser.clear_chunk();

  m_bytes_received = 0;
  m_chunk_size = 0;
  m_chunk_characters = 0;

}
