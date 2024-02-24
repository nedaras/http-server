#include "Request.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <sys/socket.h>

Request::Request(int socket)
{

  m_socket = socket;
  m_parser = http::Parser(m_buffer.get());

}

void Request::m_updateTimeout(unsigned long milliseconds)
{

  m_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) + std::chrono::milliseconds(milliseconds);

}

// we using same patterns not cool
ParserResponse Request::m_parse() // make even more states and rename them what are these names
{
   
  while (true)
  {

    if (m_state == REQUEST_READING_BODY)
    {
         
      if (body.size() >= body.capacity())
      {

        char byte;
        ssize_t bytes = recv(m_socket, &byte, 1, 0);

        // we aint checking if its -1 couse we dont care 
        return bytes == 0 ? ParserResponse{ REQUEST_CLOSE, m_firstRequest() } : ParserResponse{ REQUEST_HTTP_BUFFER_ERROR, m_firstRequest() };

      }

      ssize_t bytes = recv(m_socket, body.data() + body.size(), body.capacity() - body.size(), 0);

      if (bytes == 0) return { REQUEST_CLOSE, m_firstRequest() };
      if (bytes == -1) 
      {

        if (errno == EWOULDBLOCK)
        {

          ParserResponse response { REQUEST_INCOMPLETE, m_firstRequest() }; 

          return response;

        }

        return ParserResponse{ REQUEST_ERROR, m_firstRequest() };

      }

      body.resize(body.size() + bytes);

      // bad bad bad bad bad
      if (body.capacity() == body.size())
      {

        ParserResponse response { REQUEST_SUCCESS, m_firstRequest() };
        m_state = REQUEST_WAITING_FOR_DATA;

        return response;

      }

      continue;

    }

    if (m_bufferSize >= m_bufferLength) // no way it can be bigger then length
    {

      char byte;
      ssize_t bytes = recv(m_socket, &byte, 1, 0);

      // we aint checking if its -1 couse we dont care 
      return bytes == 0 ? ParserResponse{ REQUEST_CLOSE, m_firstRequest() } : ParserResponse{ REQUEST_HTTP_BUFFER_ERROR, m_firstRequest() };

    }
   
    ssize_t bytes = recv(m_socket, m_buffer.get() + m_bufferSize, m_bufferLength - m_bufferSize, 0);

    if (bytes == 0) return { REQUEST_CLOSE, m_firstRequest() };
    if (bytes == -1) 
    {

      if (errno == EWOULDBLOCK)
      {

        ParserResponse response { REQUEST_INCOMPLETE, m_firstRequest() };
        m_state = REQUEST_READING_HTTP;

        return response;

      }

      return ParserResponse{ REQUEST_ERROR, m_firstRequest() };

    }

    m_bufferSize += bytes;

    switch (m_parser.parse(bytes)) // make this dude read body, parser should return how many bytes we have read till eof
    {
    case 0: // EOF REACHED
      {

        if (m_parser.bodyLength > 0)
        {

          body.reserve(m_parser.bodyLength);
          body.resize(bytes - m_parser.bytesRead);

          // bro aint cool
          char* bodyStart = m_buffer.get() + m_bufferSize - bytes + m_parser.bytesRead;

          std::memcpy(body.data(), bodyStart, body.size());

          if (m_parser.bodyLength > body.size())
          {

            m_state = REQUEST_READING_BODY;  
            break;

          }

        }

        ParserResponse response { REQUEST_SUCCESS, m_firstRequest() };
        m_state = REQUEST_WAITING_FOR_DATA;

        return response;
      }
    case 1: // EOF NOT REACHED
      break;
    default: return { REQUEST_HTTP_ERROR, m_firstRequest() };
    }

  }

}

std::optional<std::string_view> Request::getHeader(std::string_view header) const
{
  
  for (auto& [ key, value ] : m_parser.headers)
  {

    if (key == header) return value;

  }
  
  return {};

}
