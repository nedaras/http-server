#include "Request.h"

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <tuple>

Request::Request(int socket)
{

  m_socket = socket;
  m_parser = http::Parser(m_buffer.get());

}

void Request::m_updateTimeout(unsigned long milliseconds)
{
  
  m_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) + std::chrono::milliseconds(milliseconds);

}

void Request::on(REQUEST_EVENTS event, const EventFunction& function) const
{
  m_events[event] = std::make_unique<EventFunction>(function);
}

// we need correct params for events
void Request::m_callEvent(REQUEST_EVENTS event, std::string_view data) const
{
  if (m_events[event]) (*m_events[event])(data);
}

std::tuple<REQUEST_STATUS, ssize_t> Request::m_safeRecv(char* buffer, std::size_t bufferSize, std::size_t bufferCapacity)
{

  if (bufferSize >= bufferCapacity)
  {

    char byte;
    ssize_t bytes = recv(m_socket, &byte, 1, 0);

    return bytes == 0 ? std::make_tuple(REQUEST_CLOSE, bytes) : std::make_tuple(REQUEST_HTTP_BUFFER_ERROR, bytes);

  }

  ssize_t bytes = recv(m_socket, buffer + bufferSize, bufferCapacity - bufferSize, 0);

  if (bytes == 0) return std::make_tuple(REQUEST_CLOSE, bytes);
  if (bytes == -1)
  {

    if (errno == EWOULDBLOCK) return std::make_tuple(REQUEST_INCOMPLETE, bytes);
    return std::make_tuple(REQUEST_ERROR, bytes);

  }

  return std::make_tuple(REQUEST_HTTP_COMPLETE, bytes); // it just means that ye we did it

}

static constexpr std::int8_t unhex[256] = {
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
  -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
  -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

// we using same patterns not cool
ParserResponse Request::m_parse() // make even more states and rename them what are these names
{

LOOP:

  switch (m_state)
  {
  case REQUEST_WAITING_FOR_DATA:
  case REQUEST_READING_HTTP:
  {

    auto [ status, bytes ] = m_safeRecv(m_buffer.get(), m_bufferSize, m_bufferLength);

    if (status != REQUEST_HTTP_COMPLETE) return { status, m_firstRequest() };

    std::string_view unhandledBytes = std::string_view(m_buffer.get() + m_bufferSize + m_parser.bytesRead, bytes - m_parser.bytesRead);
    m_bufferSize += bytes;

    switch (m_parser.parse(bytes))
    {
    case 0: // EOF REACHED
      {

        if (m_parser.bodyLength > 0)
        {

          body.reserve(m_parser.bodyLength);
          body.append(unhandledBytes);

          if (m_parser.bodyLength > body.size())
          {

            m_state = REQUEST_READING_BODY;
            goto LOOP;

          }

        }

        if (m_parser.chunked)
        {

          std::size_t size = bytes - m_parser.bytesRead;
          char* chunkStart = m_buffer.get() + m_bufferSize - bytes + m_parser.bytesRead;

          if (size == 0) break;

          std::uint32_t chunkSize = 0;
          std::uint8_t i = 0;

          while (i < 5)
          {

            std::int8_t number = unhex[static_cast<std::uint8_t>(*(chunkStart + i))];

            if (number != -1)
            {

              chunkSize *= 16;
              chunkSize += number;

              i++;

              continue;

            }

            if (i == 0) return { REQUEST_HTTP_ERROR, m_firstRequest() };

            if (*(chunkStart + i) != '\r' || *(chunkStart + i + 1) != '\n') return { REQUEST_HTTP_ERROR, m_firstRequest() };
            if (chunkSize > 0x10000) return { REQUEST_HTTP_BUFFER_ERROR, m_firstRequest() };

            break;

          }

          m_chunk.append(chunkStart + i + 2, chunkSize);

          ParserResponse response { REQUEST_CHUNK_COMPLETE, m_firstRequest() };
          m_state = REQUEST_READING_CHUNKS; 

          return response; 

        }

        ParserResponse response { REQUEST_HTTP_COMPLETE, m_firstRequest() };
        m_state = REQUEST_WAITING_FOR_DATA;

        return response;
      }
    case 1:
      m_state = REQUEST_READING_HTTP;
      goto LOOP;
    default: return { REQUEST_HTTP_ERROR, m_firstRequest() };
    }

  }
  break;
  case REQUEST_READING_BODY:
  {
    auto [ status, bytes ] = m_safeRecv(body.data(), body.size(), body.capacity());

    if (status != REQUEST_HTTP_COMPLETE) return { status, m_firstRequest() };

    body.resize(body.size() + bytes);

    if (body.capacity() == body.size())
    {

      ParserResponse response { REQUEST_HTTP_COMPLETE, m_firstRequest() };
      m_state = REQUEST_WAITING_FOR_DATA;

      return response;

    }
    goto LOOP;
  }
  case REQUEST_READING_CHUNKS:

    //we need to handle this shit

    char buf[512];
    ssize_t bytes = recv(m_socket, buf, 512, 0);

    if (bytes == -1) return { REQUEST_INCOMPLETE, m_firstRequest() };

    ParserResponse response = { REQUEST_CHUNK_END, m_firstRequest() };
    m_state = REQUEST_WAITING_FOR_DATA;
 
    std::cout << "f idk what todo: " << bytes << "\n";

    return response;
  }
  

  std::cout << "how are we here?\n";
  return { REQUEST_CLOSE, m_firstRequest() };

}

std::optional<std::string_view> Request::getHeader(std::string_view header) const
{
  
  for (auto& [ key, value ] : m_parser.headers)
  {

    if (key == header) return value;

  }
  
  return {};

}
