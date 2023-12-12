#pragma once

#include <cstdint>
#include <string>

namespace Serialisation
{

class ChunkID
{
public:
  ChunkID(const char value[5])
    : mValue((value[3] << 24) | (value[2] << 16) | (value[1] << 8) | value[0])
  {}
  bool operator==(const ChunkID& other) const { return mValue == other.mValue; }

  uint32_t mValue;
};

struct Chunk
{
  std::char_traits<char>::pos_type mBodyStart;
  std::streamoff mBodySize;
};

struct Element
{
  std::char_traits<char>::pos_type mBodyStart;
  std::streamoff mBodySize;
};

}
