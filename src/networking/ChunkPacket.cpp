#include "ChunkPacket.h"

#include <cerrno>
#include <iostream>
#include <string_view>
#include <sys/socket.h>

#include "Request.h"

void ChunkPacket::copyBuffer(const char* buffer, std::size_t size, std::uint32_t chunkSize, std::uint8_t chunkCharacters)
{

  m_chunkSize = chunkSize;
  m_chunkCharacters = chunkCharacters;

  if (m_request->m_httpParser.chunkSizeParsed())
  {
    
    m_buffer.reserve(chunkCharacters + 2 + chunkSize + 2);
    m_buffer.insert(m_buffer.end(), buffer, buffer + size);
    m_bytesReceived = m_buffer.size() - chunkCharacters - 2;

    return;
  }

  std::cout.write(buffer, size);
  std::cout << "\nthis is the buffer we only have :(\n";
  
  m_buffer.reserve(6);
  m_buffer.insert(m_buffer.end(), buffer, buffer + size);

}


void ChunkPacket::handleChunk()
{

  std::string_view chunk(m_buffer.data() + m_chunkCharacters + 2, m_buffer.size() - 2);
  m_callback(chunk);

  clear(); 

}

READ_RESPONSE ChunkPacket::read()
{

  if (!m_request->m_httpParser.chunkSizeParsed())
  {

    std::cout << "we're not handling it yeet!\n";
    std::cout << "size: " << m_chunkSize << "\n";
    std::cout << "chars: " << int(m_chunkCharacters)<< "\n";

    m_callback(std::string_view(""));

    return READ_RESPONSE_BUFFER_ERROR;

  }

  char* buffer = m_buffer.data() + m_buffer.size();
  ssize_t bytes = ::recv(m_request->m_socket, buffer, m_buffer.capacity() - m_buffer.size(), 0);

  if (bytes == 0) return READ_RESPONSE_CLOSE;
  if (bytes == -1) return READ_RESPONSE_SOCKET_ERROR;

  // TODO: we will make out own buffer like wtf why cant i just call resize() ha? this is copying memory that we already have
  // AND FUCK RHIS IS EVIL, we can use our own size and fucking ignore std::size, a hot fix or just make my own array
  m_buffer.insert(m_buffer.end(), buffer, buffer + bytes);

  auto [ status, bytesRead ] = m_request->m_httpParser.parse_chunk(buffer, bytes, m_chunkSize, m_chunkCharacters, m_bytesReceived);

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

void ChunkPacket::clear()
{

  // clear a vector that will sooner or later be allocated to same size is kinda no no
  m_buffer.clear();
  m_request->m_httpParser.clearChunk();

  m_bytesReceived = 0;
  m_chunkSize = 0;
  m_chunkCharacters = 0;

}
