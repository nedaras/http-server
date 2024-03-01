#include "Request.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <memory>
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

// we need correct porams for events
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

  return std::make_tuple(REQUEST_SUCCESS, bytes);

}

// we using same patterns not cool
ParserResponse Request::m_parse() // make even more states and rename them what are these names
{
   
  while (true)
  {

    if (m_state == REQUEST_READING_BODY)
    {
         
      auto [ status, bytes ] = m_safeRecv(body.data(), body.size(), body.capacity());

      if (status != REQUEST_SUCCESS) return { status, m_firstRequest() };

      body.resize(body.size() + bytes);

      if (body.capacity() == body.size())
      {

        ParserResponse response { REQUEST_SUCCESS, m_firstRequest() };
        m_state = REQUEST_WAITING_FOR_DATA;

        return response;

      }

      continue;

    }

    auto [ status, bytes ] = m_safeRecv(m_buffer.get(), m_bufferSize, m_bufferLength);

    if (status != REQUEST_SUCCESS) return { status, m_firstRequest() };

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
