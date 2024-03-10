#include "Request.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <tuple>
#include <utility>
#include <vector>

#include "Server.h"
#include "../siphash/siphash.h"

// TODO: first we need to check if Chunked encoding is sent if not throw send empty optional
void Request::readData(const DataCallback& callback) const
{

  if (m_receivingData())
  {
    std::cout << "bro readData call is allready set ok\n";
    return;
  }

  std::optional<std::string_view> value = m_getHeadersValue("Transfer-Encoding");

  if (!value)
  {
    std::cout << "err this dude aint even sending data\n";
    return;
  }

  char* buffer = m_buffer->data() + m_httpSize;
  std::size_t bytes = m_bufferOffset - m_httpSize;

  if (bytes > 0) 
  {

    // we need to send chunked that are inside the buffer, handle chunks that didnot fit in buffer cpy it to like a buffer or sum
  
    // why dont wa pass current chunkSize and chunkCharsAmount into this function, it should be better no
    // ok so if we get incomplete it means that a whole chunk did not fit inside the buffer, so we will need to allocate chunk buffe
    // and copy some data
    //
    
    std::uint32_t size = 0;
    std::uint8_t characters = 0;

    // TODO: idk we can make Packet struct, tha would like contain buffer and a size, and like expected max size idk
    // pass max_chunk size 0x10000
    auto [ status, bytesRead ] = m_httpParser.parse_chunk(buffer, bytes, size, characters, 0);

    switch (status)
    {
    case PARSER_RESPONSE_COMPLETE:
      std::cout << "bytes: " << bytes << "\n";
      std::cout << "bytes_read: " << bytesRead << "\n";
      std::cout << "chunk_size: " << size << "\n";
      std::cout << "chars: " << int(characters) << "\n";
      std::cout << "bytes_left: " << (bytes - bytesRead) << "\n";

      callback(std::string_view(buffer + 2 + characters, size));
      m_httpParser.clearChunk();
      break;
    case PARSER_RESPONSE_PARSING:

      //m_chunkPacket = std::make_unique<ChunkPacket>();
      //m_chunkPacket->buffer = std::string(buffer, bytes); 
      //m_chunkPacket->bufferOffset = bytes;
      //m_chunkPacket->chunkSize = size;
      //m_chunkPacket->chunkCharacters = characters;

      // for size we will only allow 5 chars ye, 2 char for \r\n then size then we already have and \r\n for end
      //std::size_t minimal = characters + 2 + size + 2;
      break;
    default:
      m_httpParser.clearChunk();
      break;
    }


  }

  m_dataCallback = std::move(callback);

}

// TODO: make a function that would like get from 200 - OK or like 500 - Server Error messages
// TODO: make it throw error if status is already set
void Request::setStatus(std::uint16_t status) const
{

  if (m_response.statusSent) return;

  send(m_socket, "HTTP/1.1 ", strlen("HTTP/1.1 "), 0);
  send(m_socket, std::to_string(status).data(), std::to_string(status).size(), 0);
  send(m_socket, "\r\n", 2, 0);

  m_response.statusSent = true;

}

void Request::setHead(std::string_view key, std::string_view value) const
{

  std::uint64_t hash = m_hashString(key);

  // THREAD_LOCK
  if (m_headerSent(hash)) [[unlikely]]
  {

    std::cout << "why the fuck u sending same headers\n";

    return;

  }

  m_setHead(key, value, hash);

}

void Request::m_setHead(std::string_view key, std::string_view value, std::uint64_t hash) const
{

  setStatus(200);

  send(m_socket, key.data(), key.size(), 0);
  send(m_socket, ": ", 2, 0);
  send(m_socket, value.data(), value.size(), 0);
  send(m_socket, "\r\n", 2, 0);

  // THREAD_LOCK
  m_response.sentHeaders.push_back(hash);

}

void Request::writeBody(std::string_view buffer) const
{
  writeBody(buffer.data(), buffer.size());
}
// NOTE: mb add err handling, if we call write body twice?
void Request::writeBody(const char* buffer, std::size_t size) const
{

  m_setDate();
  m_setConnection();
  m_setKeepAlive();
  m_setContentLength(size);

  send(m_socket, "\r\n", 2, 0);
  send(m_socket, buffer, size, 0);

}

// THREAD_LOCK
void Request::end() const
{
  
  m_updateTimeout(5000);
  const_cast<Request*>(this)->m_reset(); // red fucking flag
  
}

void Request::m_setDate() const
{

  static std::uint64_t hash = m_hashString("Date");
  if (m_headerSent(hash)) return; 

  // for null termination
  char date[29 + 1];

  std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); 
  std::strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));

  m_setHead("Date", date, hash);

}

void Request::m_setContentLength(std::size_t length) const
{
  static std::uint64_t hash = m_hashString("Content-Length");
  if (!m_headerSent(hash)) m_setHead("Content-Length", std::to_string(length), hash);
}

void Request::m_setConnection() const
{
  static std::uint64_t hash = m_hashString("Connection");
  if (!m_headerSent(hash)) m_setHead("Connection", "keep-alive", hash);
}

// TODO: only send keep alive if connection is keep alive
void Request::m_setKeepAlive() const
{
  static std::uint64_t hash = m_hashString("Keep-Alive");
  if (!m_headerSent(hash)) m_setHead("Keep-Alive", "timeout=5", hash);
}

// THREAD_LOCK
void Request::m_updateTimeout(std::time_t milliseconds) const
{

  m_server->m_timeouts.erase(this);
  m_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) + std::chrono::milliseconds(milliseconds);
  m_server->m_timeouts.push(this);

}

std::uint64_t Request::m_hashString(std::string_view string) const
{
  std::uintptr_t key = reinterpret_cast<std::uintptr_t>(m_server->m_callback.target<void>());
  return siphash::siphash_xy(string.data(), string.size(), 1, 3, reinterpret_cast<std::uintptr_t>(m_server), key);
}

// THREAD_LOCK
bool Request::m_headerSent(std::uint64_t headerHash) const
{

  for (std::uint64_t hash : m_response.sentHeaders) if (headerHash == hash) return true;
  
  return false;

}

bool Request::m_receivingData() const
{
  return m_dataCallback.has_value();
}


std::optional<std::string_view> Request::m_getHeadersValue(std::string_view key) const
{

  using Header = std::tuple<std::string_view, std::string_view>;
  std::vector<Header>::const_iterator iterator = std::find_if(headers.begin(), headers.end(), [&](const Header& header) {
    return std::get<0>(header) == key;
  });

  if  (iterator == headers.end()) return {};
  return std::get<1>(*iterator);

}

// now this is ugly
int Request::m_recvChunks()
{

  std::cout << "packet\n";

  if (m_chunkPacket)
  {

    return 0;

  }


  {

    char buffer[5 + 1]; // to get chunk size max 5 chars for a chunk and 6 to see it request is not too large

    ssize_t bytes = recv(m_socket, buffer, sizeof(buffer), 0); 

    if (bytes == 0)
    {
      std::cout << "CLOSE\n";
      return -1;
    }

    if (bytes == -1)
    {
      std::cout << "ERR\n";
      return -1;
    }

    m_chunkPacket = std::make_unique<ChunkPacket>();

    auto [ status, bytesRead ] = m_httpParser.parse_chunk(buffer, bytes, m_chunkPacket->chunkSize, m_chunkPacket->chunkCharacters, 0);

    switch (status)
    {
    case PARSER_RESPONSE_COMPLETE:
      // SHOULD WE ALLOCATE A STRING
      (*m_dataCallback)(std::string_view(buffer + m_chunkPacket->chunkCharacters + 2, m_chunkPacket->chunkSize)); 
      m_chunkPacket.reset();
      return 1;
    case PARSER_RESPONSE_PARSING:
      if (bytesRead == sizeof(buffer))
      {

        m_chunkPacket->bufferOffset += bytes;
        m_chunkPacket->reserveAndCopy(buffer, bytes);

        break;

      }

      std::cout << "bro this dude is wtf fucked\n";

      break;
    case PARSER_RESPONSE_ERROR:
      return -1;
    }

  }

  ssize_t bytes = recv(m_socket, m_chunkPacket->offseted(), m_chunkPacket->left(), 0);
  std::size_t offset = m_chunkPacket->buffer.size() - m_chunkPacket->chunkCharacters - 2;
  m_chunkPacket->buffer.resize(m_chunkPacket->buffer.size() + offset);

  auto [ status, bytesRead ] = m_httpParser.parse_chunk(m_chunkPacket->offseted(), bytes, m_chunkPacket->chunkSize, m_chunkPacket->chunkCharacters, offset);

  m_chunkPacket->bufferOffset += bytesRead;

  return 0;

}


// return like a bool if connection needs to be closed
int Request::m_recv()
{

  // {HTTP}9\r\n123456789\r\n5\r\n12345\r\n0\r\n\r\n

  if (m_receivingData()) return m_recvChunks();

  if (m_receivingData())
  {
    char buf[512];

    ssize_t bytes = recv(m_socket, buf, sizeof(buf), 0);
    
    if (bytes == 0)
    {
      std::cout << "CLOSE\n";
      return -1;
    }

    if (bytes == -1)
    {
      std::cout << "ERR\n";
      return -1;
    }

    std::uint32_t size = 0;
    std::uint8_t chars = 0;

    auto [ status, bytesRead ] = m_httpParser.parse_chunk(buf, bytes, size, chars, 0);

    switch (status)
    {
    default:
      break;
    }

    std::cout << "bytes: " << bytes << "\n";
    std::cout << "bytes_read: " << bytesRead << "\n";
    std::cout << "chunk_size: " << size << "\n";
    std::cout << "chars: " << int(chars) << "\n";
    std::cout << "bytes_left: " << (bytes - bytesRead) << "\n";

    m_httpParser.clearChunk();

    (*m_dataCallback)(std::string_view(buf, bytes)); 

    return 2;

  }

  if (m_bufferOffset >= m_buffer->size())
  {

  }

  ssize_t bytes = recv(m_socket, m_buffer->data() + m_bufferOffset, m_buffer->size() - m_bufferOffset, 0);

  if (bytes == 0)
  {
    std::cout << "CLOSE\n";
    return -1;
  }

  if (bytes == -1)
  {
    std::cout << "ERR\n";
    return -1;
  }

  if (m_response.completed)
  {

    m_response.completed = false;

  }

  auto [ a, b ] = m_httpParser.parse_http(bytes, method, path, [this](std::string_view key, std::string_view value) {
    
    headers.push_back(std::make_pair(key, value));

    return true;

  });

  m_bufferOffset += bytes;
  m_httpSize += b;  

  //std::cout << "recv: " << bytes << "\n";
  //std::cout << "read: " << b << "\n";

  //std::cout << "res: " << a << "\n";

  return a == PARSER_RESPONSE_COMPLETE ? 0 : a == PARSER_RESPONSE_PARSING ? 1 : -1;

}

void Request::m_reset()
{

  m_response = Response();
  m_response.completed = true;
  m_dataCallback.reset();

  headers.clear();

  m_httpParser.clear(m_buffer->data());
  m_bufferOffset = 0;
  m_httpSize = 0;

}
