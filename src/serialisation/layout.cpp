#include "serialisation/layout.h"

#include "model/controlpoint.h"
#include "model/node.h"
#include "model/path.h"
#include "model/sketch.h"
#include "serialisation/reader.h"
#include "serialisation/types.h"
#include "serialisation/writer.h"

#include <cassert>

namespace Serialisation
{

namespace
{

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
auto beginListChunk(TEndpoint& endpoint, ChunkID id, ChunkID listID)
{
  auto result = endpoint.beginChunk(id);

  ChunkID listIDActual = listID;
  simpleValue(endpoint, &listIDActual);

  checkValid(listIDActual == listID, "Unexpected list ID");

  return result;
}

template <class TEndpoint, class TModel, class TCallback>
void variableElements(TEndpoint& endpoint, std::unordered_map<ID<TModel>, TModel*>* map, TCallback callback)
{
  endpoint.modelMap(map, [&endpoint, callback](TModel* model) {
      Element element;
      endpoint.beginElement(&element);

      callback(endpoint, model);

      endpoint.endElement(&element);
    });
}

template <class TEndpoint, class TModel, class TCallback>
void fixedElements(TEndpoint& endpoint, std::unordered_map<ID<TModel>, TModel*>* map, TCallback callback)
{
  Element element;

  bool first = true;

  endpoint.modelMap(map, [&endpoint, callback, &element, &first](TModel* model) {
      if (first) {
        endpoint.beginElement(&element);
      } else {
        endpoint.beginFixedElement(&element);
      }

      callback(endpoint, model);

      if (first) {
        endpoint.endElement(&element);
      } else {
        endpoint.endFixedElement(&element);
      }

      first = false;
    });
}

template <class TEndpoint, class TValue, class TCallback>
void fixedElements(TEndpoint& endpoint, std::vector<TValue>* vector, TCallback callback)
{
  Element element;

  bool first = true;

  endpoint.collection(vector, [&endpoint, callback, &element, &first](TValue* value) {
      if (first) {
        endpoint.beginElement(&element);
      } else {
        endpoint.beginFixedElement(&element);
      }

      callback(endpoint, value);

      if (first) {
        endpoint.endElement(&element);
      } else {
        endpoint.endFixedElement(&element);
      }

      first = false;
    });
}

static const unsigned int CurrentVersion = 1;

}

template <class TEndpoint>
Model::Sketch* Layout::process(TEndpoint& endpoint, Model::Sketch* sketch)
{
  auto riffChunk = beginListChunk(endpoint, "RIFF", "SPLN");

  // Format info
  auto formatInfoChunk = endpoint.beginChunk("FRMT");

  unsigned int version = CurrentVersion;
  endpoint.asUint32(&version);

  endpoint.endChunk(formatInfoChunk);

  // Sketch
  auto sketchChunk = beginListChunk(endpoint, "LIST", "SKCH");

  endpoint.beginObject(&sketch);

  // Nodes
  auto nodesChunk = endpoint.beginChunk("NODS");

  variableElements(endpoint, &sketch->mNodes, processNode<TEndpoint>);

  endpoint.endChunk(nodesChunk);

  // Control points
  auto controlPointsChunk = endpoint.beginChunk("CPTS");

  fixedElements(endpoint, &sketch->mControlPoints, processControlPoint<TEndpoint>);

  endpoint.endChunk(controlPointsChunk);

  // Paths
  auto pathsChunk = endpoint.beginChunk("PTHS");

  variableElements(endpoint, &sketch->mPaths, processPath<TEndpoint>);

  endpoint.endChunk(pathsChunk);

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
void Layout::processPath(TEndpoint& endpoint, Model::Path* path)
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
