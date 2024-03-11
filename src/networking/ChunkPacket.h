#pragma once

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

using DataCallback = std::function<bool(std::optional<std::string_view>)>;

enum READ_RESPONSE : std::uint8_t
{
  READ_RESPONSE_DONE,
  READ_RESPONSE_WAITING,
  READ_RESPONSE_SOCKET_ERROR,
  READ_RESPONSE_BUFFER_ERROR,
  READ_RESPONSE_PARSING_ERROR,
  READ_RESPONSE_CLOSE
};

class Request;
class ChunkPacket
{

public:

  ChunkPacket(const Request* request, const DataCallback& callback) : m_request(request), m_callback(std::move(callback)) {};

  void copyBuffer(const char* buffer, std::size_t size, std::uint32_t chunkSize, std::uint8_t chunkCharacters, std::size_t bytesReceived);

  void handleChunk();
  void handleChunk(std::optional<std::string_view> data);

  READ_RESPONSE read();

  void clear();

private:

  std::size_t m_rawSize() const
  {
    return m_chunkCharacters + 2 + m_chunkSize + 2;
  }

private:

  std::vector<char> m_buffer;
  std::size_t m_bytesReceived = 0;

  std::uint32_t m_chunkSize = 0;
  std::uint8_t m_chunkCharacters = 0;
  static constexpr std::uint8_t m_maxChunkCharacters = 5; // would be nice if we could allow users to modify

  const Request* m_request;
  DataCallback m_callback;

};
