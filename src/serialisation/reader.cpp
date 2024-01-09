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
  , mVersion(0)
{
}

Reader::Element Reader::beginChunk(uint32_t expectedID)
{
  uint32_t actualID = 0;
  read(&actualID);

  assert(actualID == expectedID);

  return beginElement();
}

void Reader::endChunk(const Element& element)
{
  Stream::pos_type bodyEnd = element.mBodyStart + element.mBodySize;
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

  if (sketch->mDrawOrder.empty()) {
    sketch->mDrawOrder.reserve(sketch->mPaths.size());

    for (auto current : sketch->mPaths) {
      sketch->mDrawOrder.push_back(current.first);
    }
  }
}

Reader::Element Reader::beginElement()
{
  Element element;
  readAs<uint32_t>(&element.mBodySize);
  element.mBodyStart = mStream.tellg();

  return element;
}

Reader::Element Reader::beginFixedElement(const Element& definition)
{
  Element element = definition;
  element.mBodyStart = mStream.tellg();

  return element;
}

Reader::Element Reader::endElement(const Element& element)
{
  Stream::pos_type bodyEnd = element.mBodyStart + element.mBodySize;
  Stream::pos_type position = mStream.tellg();

  assert(position <= bodyEnd);

  mStream.seekg(bodyEnd);

  return element;
}

void Reader::endFixedElement(const Element& element)
{
  endElement(element);
}

void Reader::data(char* bytes, std::streamsize count)
{
  mStream.read(bytes, count);
}

}
