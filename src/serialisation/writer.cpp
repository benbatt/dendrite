#include "serialisation/writer.h"

#include "model/node.h"
#include "model/controlpoint.h"
#include "model/sketch.h"
#include "utilities/geometry.h"
#include "utilities/id.h"

#include <cassert>

namespace Serialisation
{

Writer::Writer(Stream& stream)
  : mStream(stream)
{
}

Writer::Stream::pos_type Writer::beginChunk(ChunkID id)
{
  asUint32(&id.mValue);
  mStream.write("size", 4);

  return mStream.tellp();
}

void Writer::endChunk(const Stream::pos_type& bodyStart)
{
  Stream::pos_type position = mStream.tellp();
  std::streamoff size = position - bodyStart;

  if (size % 2 != 0) {
    mStream.put(0);
    position = mStream.tellp();
    size = position - bodyStart;
  }

  mStream.seekp(bodyStart - std::streamoff(4));
  writeAs<uint32_t>(size);

  mStream.seekp(position);
}

template <class TRange, class TModel>
void buildIDMap(const TRange& range, std::unordered_map<ID<TModel>, IDValue>* map)
{
  map->clear();

  IDValue nextIndex = 1;

  for (auto current : range) {
    (*map)[current.first] = nextIndex;
    ++nextIndex;
  }
}

void Writer::beginObject(Model::Sketch** sketch)
{
  buildIDMap((*sketch)->nodes(), &mNodeIndices);
  buildIDMap((*sketch)->controlPoints(), &mControlPointIndices);
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
