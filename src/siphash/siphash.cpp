#include "siphash.h"

/* ******************************************************************************************************
Siphash implemented from: https://github.com/boazsegev/facil.io/blob/master/lib/facil/fiobj/fio_siphash.c
************************************************************************************************ */

static constexpr std::uint64_t lrot64(std::uint64_t i, std::uint8_t bits)
{
  return (i << bits) | (i >> (64 - bits));
}

std::uint64_t siphash::siphash_xy(const void* data, std::size_t size, std::uint8_t x, std::uint8_t y, std::uint64_t key1, std::uint64_t key2)
{

  /* initialize the 4 words */
  std::uint64_t v0 = (0x0706050403020100ULL ^ 0x736f6d6570736575ULL) ^ key1;
  std::uint64_t v1 = (0x0f0e0d0c0b0a0908ULL ^ 0x646f72616e646f6dULL) ^ key2;
  std::uint64_t v2 = (0x0706050403020100ULL ^ 0x6c7967656e657261ULL) ^ key1;
  std::uint64_t v3 = (0x0f0e0d0c0b0a0908ULL ^ 0x7465646279746573ULL) ^ key2;
  const std::uint64_t* w64 = static_cast<const std::uint64_t*>(data);
  std::uint8_t len_mod = size & 255;
  union {
    std::uint64_t i;
    uint8_t str[8];
  } word;

#define hash_map_SipRound                                                      \
  do {                                                                         \
    v2 += v3;                                                                  \
    v3 = lrot64(v3, 16) ^ v2;                                                  \
    v0 += v1;                                                                  \
    v1 = lrot64(v1, 13) ^ v0;                                                  \
    v0 = lrot64(v0, 32);                                                       \
    v2 += v1;                                                                  \
    v0 += v3;                                                                  \
    v1 = lrot64(v1, 17) ^ v2;                                                  \
    v3 = lrot64(v3, 21) ^ v0;                                                  \
    v2 = lrot64(v2, 32);                                                       \
  } while (0);

  while (size >= 8) {
    word.i = *w64;
    v3 ^= word.i;
    /* Sip Rounds */
    for (std::size_t i = 0; i < x; ++i) {
      hash_map_SipRound;
    }
    v0 ^= word.i;
    w64 += 1;
    size -= 8;
  }
  word.i = 0;
  std::uint8_t* pos = word.str;
  const std::uint8_t* w8 = reinterpret_cast<const std::uint8_t*>(w64);
  switch (size) { /* fallthrough is intentional */
    case 7:
      pos[6] = w8[6];
      /* fallthrough */
    case 6:
      pos[5] = w8[5];
      /* fallthrough */
    case 5:
      pos[4] = w8[4];
      /* fallthrough */
    case 4:
      pos[3] = w8[3];
      /* fallthrough */
    case 3:
      pos[2] = w8[2];
      /* fallthrough */
    case 2:
      pos[1] = w8[1];
      /* fallthrough */
    case 1:
      pos[0] = w8[0];
  }
  word.str[7] = len_mod;

  /* last round */
  v3 ^= word.i;
  hash_map_SipRound;
  hash_map_SipRound;
  v0 ^= word.i;
  /* Finalization */
  v2 ^= 0xff;
  /* d iterations of SipRound */
  for (std::size_t i = 0; i < y; ++i) {
    hash_map_SipRound;
  }
  hash_map_SipRound;
  hash_map_SipRound;
  hash_map_SipRound;
  hash_map_SipRound;
  /* XOR it all together */
  v0 ^= v1 ^ v2 ^ v3;
#undef hash_map_SipRound
  return v0;
}
