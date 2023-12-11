#pragma once

#include "model/node.h"
#include "utilities/id.h"
#include "utilities/geometry.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Model
{
  class Sketch;
}

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

template <class TDelegate>
class Endpoint
{
public:
  Endpoint(TDelegate& delegate)
    : mDelegate(delegate)
  {}

  void beginListChunk(ChunkID id, ChunkID listID, Chunk* chunk)
  {
    mDelegate.beginChunk(id, chunk);

    ChunkID listIDActual = listID;
    simpleValue(&listIDActual);

    checkValid(listIDActual == listID, "Unexpected list ID");
  }

  void beginChunk(ChunkID id, Chunk* chunk)
  {
    mDelegate.beginChunk(id, chunk);
  }

  template<class TModel>
  void beginObject(TModel** model)
  {
    mDelegate.beginObject(model);
  }

  template<class TModel>
  void endObject(TModel* model)
  {
    mDelegate.endObject(model);
  }

  void endChunk(const Chunk& chunk)
  {
    mDelegate.endChunk(chunk);
  }

  template<class TModel, class TCallback>
  void variableElements(std::unordered_map<ID<TModel>, TModel*>* map, TCallback callback)
  {
    mDelegate.modelMap(map, [this, callback](TModel* model) {
        Element element;
        mDelegate.beginElement(&element);

        callback(*this, model);

        mDelegate.endElement(&element);
      });
  }

  template<class TModel, class TCallback>
  void fixedElements(std::unordered_map<ID<TModel>, TModel*>* map, TCallback callback)
  {
    Element element;

    bool first = true;

    mDelegate.modelMap(map, [this, callback, &element, &first](TModel* model) {
        if (first) {
          mDelegate.beginElement(&element);
        } else {
          mDelegate.beginFixedElement(&element);
        }

        callback(*this, model);

        if (first) {
          mDelegate.endElement(&element);
        } else {
          mDelegate.endFixedElement(&element);
        }

        first = false;
      });
  }

  template<class TValue, class TCallback>
  void fixedElements(std::vector<TValue>* vector, TCallback callback)
  {
    Element element;

    bool first = true;

    mDelegate.collection(vector, [this, callback, &element, &first](TValue* value) {
        if (first) {
          mDelegate.beginElement(&element);
        } else {
          mDelegate.beginFixedElement(&element);
        }

        callback(*this, value);

        if (first) {
          mDelegate.endElement(&element);
        } else {
          mDelegate.endFixedElement(&element);
        }

        first = false;
      });
  }

  void simpleValue(ChunkID* value)
  {
    data(&value->mValue);
  }

  void simpleValue(Point* value)
  {
    mDelegate.asDouble(&value->x);
    mDelegate.asDouble(&value->y);
  }

  void simpleValue(Model::Node::Type* value)
  {
    mDelegate.asUint32(value);
  }

  void simpleValue(unsigned int* value)
  {
    mDelegate.asUint32(value);
  }

  template <class TModel>
  void id(ID<TModel>* id)
  {
    mDelegate.id(id);
  }

  template<class TValue>
  void data(TValue* value)
  {
    mDelegate.data(reinterpret_cast<char*>(value), sizeof(*value));
  }

private:
  void checkValid(bool valid, std::string message)
  {
    assert(valid);
  }

  TDelegate& mDelegate;
};

}
