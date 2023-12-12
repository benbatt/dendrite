#pragma once

#include "serialisation/types.h"
#include "utilities/id.h"

#include <ostream>
#include <unordered_map>

namespace Model
{
  class Node;
  class ControlPoint;
  class Sketch;
}

namespace Serialisation
{

class Writer
{
public:
  typedef std::basic_ostream<char> Stream;

  Writer(Stream& stream);

  Stream::pos_type beginChunk(ChunkID id);
  void endChunk(const Stream::pos_type& bodyStart);
  void beginObject(Model::Sketch** sketch);
  void endObject(Model::Sketch* sketch);

  template<class TModel, class TCallback>
  void modelMap(std::unordered_map<ID<TModel>, TModel*>* map, TCallback callback)
  {
    collection(map, [callback](const auto& current) { callback(current->second); });
  }

  template<class TCollection, class TCallback>
  void collection(TCollection* collection, TCallback callback)
  {
    writeAs<uint32_t>(collection->size());

    for (auto& current : *collection) {
      callback(&current);
    }
  }

  void beginElement(Element* element);
  void beginFixedElement(Element* element);
  void endElement(Element* element);
  void endFixedElement(Element* element);

  template <class TModel>
  void id(ID<TModel>* id)
  {
    IDValue value = remap(id);
    data(reinterpret_cast<char*>(&value), sizeof(value));
  }

  template <class TModel>
  IDValue remap(ID<TModel>* id)
  {
    return id->value();
  }

  IDValue remap(ID<Model::Node>* id)
  {
    return mNodeIndices.at(*id);
  }

  IDValue remap(ID<Model::ControlPoint>* id)
  {
    return mControlPointIndices.at(*id);
  }

  template <class TValue> void asUint32(TValue* value) { writeAs<uint32_t>(*value); }
  template <class TValue> void asDouble(TValue* value) { writeAs<double>(*value); }

  template <class TSerialized, class TValue>
  void writeAs(const TValue& value)
  {
    TSerialized serialized = static_cast<TSerialized>(value);
    data(reinterpret_cast<char*>(&serialized), sizeof(serialized));
  }

  void data(char* bytes, std::streamsize count);

private:
  Stream& mStream;
  std::unordered_map<ID<Model::Node>, IDValue> mNodeIndices;
  std::unordered_map<ID<Model::ControlPoint>, IDValue> mControlPointIndices;
};

}
