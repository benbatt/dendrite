#include "serialisation/writer.h"

#include "model/node.h"
#include "model/controlpoint.h"
#include "model/sketch.h"
#include "utilities/geometry.h"
#include "utilities/id.h"

namespace Serialisation
{

Writer::Writer(Stream& stream)
  : mStream(stream)
{
}

void Writer::beginChunk(ChunkID id, Chunk* chunk)
{
  asUint32(&id.mValue);
  mStream.write("size", 4);

  chunk->mBodyStart = mStream.tellp();
}

void Writer::endChunk(const Chunk& chunk)
{
  Stream::pos_type position = mStream.tellp();
  std::streamoff size = position - chunk.mBodyStart;

  if (size % 2 != 0) {
    mStream.put(0);
    position = mStream.tellp();
    size = position - chunk.mBodyStart;
  }

  mStream.seekp(chunk.mBodyStart - std::streamoff(4));
  writeAs<uint32_t>(size);

  mStream.seekp(position);
}

void Writer::beginObject(Model::Sketch** sketch)
{
  mNodeIndices.clear();

  int nextIndex = 1;

  (*sketch)->forEachNode(
    [this, &nextIndex](const ID<Model::Node>& id, const Model::Node* node) {
      mNodeIndices[id] = nextIndex;
      ++nextIndex;
      return false;
    });

  mControlPointIndices.clear();

  nextIndex = 1;

  (*sketch)->forEachControlPoint(
    [this, &nextIndex](const ID<Model::ControlPoint>& id, const Model::ControlPoint* controlPoint) {
      mControlPointIndices[id] = nextIndex;
      ++nextIndex;
      return false;
    });
}

void Writer::endObject(Model::Sketch* sketch)
{
}

void Writer::beginElement(Element* element)
{
  mStream.write("size", 4);
  element->mBodyStart = mStream.tellp();
}

void Writer::beginFixedElement(Element* element)
{
  element->mBodyStart = mStream.tellp();
}

void Writer::endElement(Element* element)
{
  Stream::pos_type position = mStream.tellp();

  element->mBodySize = position - element->mBodyStart;

  mStream.seekp(element->mBodyStart - std::streamoff(4));
  writeAs<uint32_t>(element->mBodySize);

  mStream.seekp(position);
}

void Writer::endFixedElement(Element* element)
{
  Stream::pos_type size = mStream.tellp() - element->mBodyStart;
  assert(size == element->mBodySize);
}

void Writer::data(char* bytes, std::streamsize count)
{
  mStream.write(bytes, count);
}

}
