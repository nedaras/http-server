#include "ChunkPacket.h"

#include <iostream>
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

    std::cout << "copacity: " << m_buffer.capacity() << "\n";
    std::cout << "size: " << m_buffer.size() << "\n";

    return;
  }

  std::cout.write(buffer, size);
  std::cout << "\nthis is the buffer we only have :(\n";
  
  m_buffer.reserve(6);
  m_buffer.insert(m_buffer.end(), buffer, buffer + size);

}


int ChunkPacket::recv()
{

  if (!m_request->m_httpParser.chunkSizeParsed())
  {

  }

  return 1;

}
