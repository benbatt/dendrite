#pragma once

#include "utilities/id.h"

#include <istream>
#include <unordered_map>

namespace Model
{
  class Sketch;
}

namespace Serialisation
{

class Reader
{
public:
  typedef std::basic_istream<char> Stream;

  Reader(Stream& stream);

  struct Element
  {
    Stream::pos_type mBodyStart;
    Stream::off_type mBodySize;
  };

  Element beginChunk(uint32_t expectedID);
  void endChunk(const Element& element);
  void beginObject(Model::Sketch** sketch);
  void endObject(Model::Sketch* sketch);

  template<class TModel, class TCallback>
  void modelMap(std::unordered_map<ID<TModel>, TModel*>* map, TCallback callback)
  {
    uint32_t size = 0;
    read(&size);

    map->reserve(size);

    for (uint32_t i = 0; i < size; ++i) {
      TModel* model = new TModel;
      (*map)[ID<TModel>(i + 1)] = model;

      callback(model);
    }
  }

  template<class TCollection, class TCallback>
  void collection(TCollection* collection, TCallback callback)
  {
    uint32_t size = 0;
    read(&size);

    collection->reserve(size);

    for (uint32_t i = 0; i < size; ++i) {
      auto& value = collection->emplace_back();
      callback(&value);
    }
  }

  Element beginElement();
  Element beginFixedElement(const Element& definition);
  Element endElement(const Element& element);
  void endFixedElement(const Element& element);

  template <class TModel>
  void id(ID<TModel>* id)
  {
    IDValue value = 0;
    read(&value);
    *id = ID<TModel>(value);
  }

  template <class TValue> void asUint32(TValue* value) { readAs<uint32_t>(value); }
  template <class TValue> void asDouble(TValue* value) { readAs<double>(value); }

  template <class TValue>
  void read(TValue* value)
  {
    data(reinterpret_cast<char*>(value), sizeof(*value));
  }

  template <class TSerialized, class TValue>
  void readAs(TValue* value)
  {
    TSerialized serialized;
    read(&serialized);
    *value = static_cast<TValue>(serialized);
  }

  void data(char* bytes, std::streamsize count);

private:
  Stream& mStream;
};

}
