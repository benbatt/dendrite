#include "serialisation/reader.h"

#include "model/node.h"
#include "model/controlpoint.h"
#include "model/sketch.h"
#include "utilities/geometry.h"
#include "utilities/id.h"

#include <cassert>

namespace Serialisation
{

Reader::Reader(Stream& stream)
  : mStream(stream)
{
}

void Reader::beginChunk(ChunkID id, Chunk* chunk)
{
  uint32_t actualID = 0;
  read(&actualID);

  assert(actualID == id.mValue);

  readAs<uint32_t>(&chunk->mBodySize);
  chunk->mBodyStart = mStream.tellg();
}

void Reader::endChunk(const Chunk& chunk)
{
  Stream::pos_type bodyEnd = chunk.mBodyStart + chunk.mBodySize;
  Stream::pos_type position = mStream.tellg();

  assert(position <= bodyEnd);

  mStream.seekg(bodyEnd);
}

void Reader::beginObject(Model::Sketch** sketch)
{
  *sketch = new Model::Sketch;
}

void Reader::endObject(Model::Sketch* sketch)
{
  sketch->mNextID = std::max(sketch->mNodes.size(), sketch->mControlPoints.size()) + 1;
}

void Reader::beginElement(Element* element)
{
  readAs<uint32_t>(&element->mBodySize);
  element->mBodyStart = mStream.tellg();
}

void Reader::beginFixedElement(Element* element)
{
  element->mBodyStart = mStream.tellg();
}

void Reader::endElement(Element* element)
{
  Stream::pos_type bodyEnd = element->mBodyStart + element->mBodySize;
  Stream::pos_type position = mStream.tellg();

  assert(position <= bodyEnd);

  mStream.seekg(bodyEnd);
}

void Reader::endFixedElement(Element* element)
{
  endElement(element);
}

void Reader::data(char* bytes, std::streamsize count)
{
  mStream.read(bytes, count);
}

}
