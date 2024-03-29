#include "serialisation/layout.h"

#include "model/controlpoint.h"
#include "model/node.h"
#include "model/path.h"
#include "model/sketch.h"
#include "serialisation/reader.h"
#include "serialisation/writer.h"

#include <cassert>

namespace Serialisation
{

namespace
{

class ChunkID
{
public:
  using ValueString = const char[5];

  ChunkID(ValueString &value)
    : mValue((value[3] << 24) | (value[2] << 16) | (value[1] << 8) | value[0])
  {}
  bool operator==(const ChunkID& other) const { return mValue == other.mValue; }

  uint32_t mValue;
};

void checkValid(bool valid, std::string message)
{
  assert(valid);
}

template <class TEndpoint, class TValue>
void data(TEndpoint& endpoint, TValue* value)
{
  endpoint.data(reinterpret_cast<char*>(value), sizeof(*value));
}

template <class TEndpoint>
void simpleValue(TEndpoint& endpoint, ChunkID* value)
{
  data(endpoint, &value->mValue);
}

template <class TEndpoint>
void simpleValue(TEndpoint& endpoint, Point* value)
{
  endpoint.asDouble(&value->x);
  endpoint.asDouble(&value->y);
}

template <class TEndpoint>
auto beginChunk(TEndpoint& endpoint, ChunkID id)
{
  return endpoint.beginChunk(id.mValue);
}

template <class TEndpoint>
auto beginListChunk(TEndpoint& endpoint, ChunkID id, ChunkID listID)
{
  auto result = endpoint.beginChunk(id.mValue);

  ChunkID listIDActual = listID;
  simpleValue(endpoint, &listIDActual);

  checkValid(listIDActual == listID, "Unexpected list ID");

  return result;
}

template <class TEndpoint, class TModel, class TCallback>
void variableElements(TEndpoint& endpoint, std::unordered_map<ID<TModel>, TModel*>* map, TCallback callback)
{
  endpoint.modelMap(map, [&endpoint, callback](TModel* model) {
      auto element = endpoint.beginElement();

      callback(endpoint, model);

      endpoint.endElement(element);
    });
}

template <class TEndpoint, class TModel, class TCallback>
void fixedElements(TEndpoint& endpoint, std::unordered_map<ID<TModel>, TModel*>* map, TCallback callback)
{
  typename TEndpoint::Element element;

  bool first = true;

  endpoint.modelMap(map, [&endpoint, callback, &element, &first](TModel* model) {
      if (first) {
        element = endpoint.beginElement();
      } else {
        element = endpoint.beginFixedElement(element);
      }

      callback(endpoint, model);

      if (first) {
        element = endpoint.endElement(element);
      } else {
        endpoint.endFixedElement(element);
      }

      first = false;
    });
}

template <class TEndpoint, class TValue, class TCallback>
void fixedElements(TEndpoint& endpoint, std::vector<TValue>* vector, TCallback callback)
{
  typename TEndpoint::Element element;

  bool first = true;

  endpoint.collection(vector, [&endpoint, callback, &element, &first](TValue* value) {
      if (first) {
        element = endpoint.beginElement();
      } else {
        element = endpoint.beginFixedElement(element);
      }

      callback(endpoint, value);

      if (first) {
        element = endpoint.endElement(element);
      } else {
        endpoint.endFixedElement(element);
      }

      first = false;
    });
}

template <class TEndpoint, class TModel, class TCallback>
void chunks(TEndpoint& endpoint, std::unordered_map<ID<TModel>, TModel*>* map, ChunkID listID,
  TCallback callback)
{
  auto listChunk = beginListChunk(endpoint, "LIST", listID);

  endpoint.modelMapChunks(map, ChunkID("HEAD").mValue, ChunkID("ELEM").mValue,
    [&endpoint, callback](TModel* model) {
      callback(endpoint, model);
    });

  endpoint.endChunk(listChunk);
}

}

namespace Version
{

static const unsigned int Initial = 1;
static const unsigned int PathChunks = 2;
static const unsigned int DrawOrder = 3;
static const unsigned int Current = 3;

}

template <class TEndpoint>
Model::Sketch* Layout::process(TEndpoint& endpoint, Model::Sketch* sketch)
{
  auto riffChunk = beginListChunk(endpoint, "RIFF", "SPLN");

  // Format info
  auto formatInfoChunk = beginChunk(endpoint, "FRMT");

  unsigned int version = Version::Current;
  endpoint.asUint32(&version);
  endpoint.setVersion(version);

  endpoint.endChunk(formatInfoChunk);

  // Sketch
  auto sketchChunk = beginListChunk(endpoint, "LIST", "SKCH");

  endpoint.beginObject(&sketch);

  // Nodes
  auto nodesChunk = beginChunk(endpoint, "NODS");

  variableElements(endpoint, &sketch->mNodes, processNode<TEndpoint>);

  endpoint.endChunk(nodesChunk);

  // Control points
  auto controlPointsChunk = beginChunk(endpoint, "CPTS");

  fixedElements(endpoint, &sketch->mControlPoints, processControlPoint<TEndpoint>);

  endpoint.endChunk(controlPointsChunk);

  // Paths
  if (endpoint.version() >= Version::PathChunks) {
    chunks(endpoint, &sketch->mPaths, "PTHS", processPathChunk<TEndpoint>);
  } else {
    auto pathsChunk = beginChunk(endpoint, "PTHS");

    variableElements(endpoint, &sketch->mPaths, processPathElement<TEndpoint>);

    endpoint.endChunk(pathsChunk);
  }

  // Draw order
  if (endpoint.version() >= Version::DrawOrder) {
    auto drawOrderChunk = beginChunk(endpoint, "ORDR");

    fixedElements(endpoint, &sketch->mDrawOrder,
      [](TEndpoint& endpoint, ID<Model::Path>* id) {
        endpoint.id(id);
      });

    endpoint.endChunk(drawOrderChunk);
  }

  endpoint.endObject(sketch);

  endpoint.endChunk(sketchChunk);

  endpoint.endChunk(riffChunk);

  return sketch;
}

template <class TEndpoint>
void Layout::processNode(TEndpoint& endpoint, Model::Node* node)
{
  simpleValue(endpoint, &node->mPosition);
  endpoint.asUint32(&node->mType);

  fixedElements(endpoint, &node->mControlPoints,
    [](TEndpoint& endpoint, ID<Model::ControlPoint>* id) {
      endpoint.id(id);
    });
}

template <class TEndpoint>
void Layout::processControlPoint(TEndpoint& endpoint, Model::ControlPoint* controlPoint)
{
  simpleValue(endpoint, &controlPoint->mPosition);
  endpoint.id(&controlPoint->mNode);
}

template <class TEndpoint>
void Layout::processPathChunk(TEndpoint& endpoint, Model::Path* path)
{
  auto headElement = endpoint.beginElement();

  endpoint.asUint32(&path->mFlags);
  endpoint.asUint32(&path->mStrokeColour.mValue);
  endpoint.asUint32(&path->mFillColour.mValue);

  endpoint.endElement(headElement);

  fixedElements(endpoint, &path->mEntries,
    [](TEndpoint& endpoint, Model::Path::Entry* entry) {
      endpoint.id(&entry->mNode);
      endpoint.id(&entry->mPreControl);
      endpoint.id(&entry->mPostControl);
    });
}

template <class TEndpoint>
void Layout::processPathElement(TEndpoint& endpoint, Model::Path* path)
{
  endpoint.asUint32(&path->mFlags);

  fixedElements(endpoint, &path->mEntries,
    [](TEndpoint& endpoint, Model::Path::Entry* entry) {
      endpoint.id(&entry->mNode);
      endpoint.id(&entry->mPreControl);
      endpoint.id(&entry->mPostControl);
    });
}

template Model::Sketch* Layout::process(Writer& endpoint, Model::Sketch* sketch);
template Model::Sketch* Layout::process(Reader& endpoint, Model::Sketch* sketch);

}
