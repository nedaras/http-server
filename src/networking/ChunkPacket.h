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

  void copyBuffer(const char* buffer, std::size_t size, std::uint32_t chunkSize, std::uint8_t chunkCharacters);

  void handleChunk();

  READ_RESPONSE read();

  void clear();


private:

  std::vector<char> m_buffer;
  std::size_t m_bytesReceived = 0;

  std::uint32_t m_chunkSize = 0;
  std::uint8_t m_chunkCharacters = 0;

  const Request* m_request;
  DataCallback m_callback;

};
