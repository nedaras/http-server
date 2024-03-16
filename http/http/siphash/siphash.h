#pragma once

#include <cstdint>

namespace siphash
{

  std::uint64_t siphash_xy(const void* data, std::size_t size, std::uint8_t x, std::uint8_t y, std::uint64_t key1, std::uint64_t key2);

};
