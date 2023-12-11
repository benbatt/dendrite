#include "serialisation/layout.h"

#include "model/controlpoint.h"
#include "model/node.h"
#include "model/path.h"
#include "model/sketch.h"
#include "serialisation/endpoint.h"
#include "serialisation/reader.h"
#include "serialisation/writer.h"

namespace Serialisation
{

static const unsigned int CurrentVersion = 1;

template <class TEndpoint>
Model::Sketch* Layout::process(TEndpoint& endpoint, Model::Sketch* sketch)
{
  Chunk riffChunk;
  endpoint.beginListChunk("RIFF", "SPLN", &riffChunk);

  // Format info
  Chunk formatInfoChunk;
  endpoint.beginChunk("FRMT", &formatInfoChunk);

  unsigned int version = CurrentVersion;
  endpoint.simpleValue(&version);

  endpoint.endChunk(formatInfoChunk);

  // Sketch
  Chunk sketchChunk;
  endpoint.beginListChunk("LIST", "SKCH", &sketchChunk);

  endpoint.beginObject(&sketch);

  // Nodes
  Chunk nodesChunk;
  endpoint.beginChunk("NODS", &nodesChunk);

  endpoint.variableElements(&sketch->mNodes, processNode<TEndpoint>);

  endpoint.endChunk(nodesChunk);

  // Control points
  Chunk controlPointsChunk;
  endpoint.beginChunk("CPTS", &controlPointsChunk);

  endpoint.fixedElements(&sketch->mControlPoints, processControlPoint<TEndpoint>);

  endpoint.endChunk(controlPointsChunk);

  // Paths
  Chunk pathsChunk;
  endpoint.beginChunk("PTHS", &pathsChunk);

  endpoint.variableElements(&sketch->mPaths, processPath<TEndpoint>);

  endpoint.endChunk(pathsChunk);

  endpoint.endObject(sketch);

  endpoint.endChunk(sketchChunk);

  endpoint.endChunk(riffChunk);

  return sketch;
}

template <class TEndpoint>
void Layout::processNode(TEndpoint& endpoint, Model::Node* node)
{
  endpoint.simpleValue(&node->mPosition);
  endpoint.simpleValue(&node->mType);

  endpoint.fixedElements(&node->mControlPoints,
    [](TEndpoint& endpoint, ID<Model::ControlPoint>* id) {
      endpoint.id(id);
    });
}

template <class TEndpoint>
void Layout::processControlPoint(TEndpoint& endpoint, Model::ControlPoint* controlPoint)
{
  endpoint.simpleValue(&controlPoint->mPosition);
  endpoint.id(&controlPoint->mNode);
}

template <class TEndpoint>
void Layout::processPath(TEndpoint& endpoint, Model::Path* path)
{
  endpoint.simpleValue(&path->mFlags);

  endpoint.fixedElements(&path->mEntries,
    [](TEndpoint& endpoint, Model::Path::Entry* entry) {
      endpoint.id(&entry->mNode);
      endpoint.id(&entry->mPreControl);
      endpoint.id(&entry->mPostControl);
    });
}

template Model::Sketch* Layout::process(Endpoint<Writer>& endpoint, Model::Sketch* sketch);
template Model::Sketch* Layout::process(Endpoint<Reader>& endpoint, Model::Sketch* sketch);

}
