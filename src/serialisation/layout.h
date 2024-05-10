#pragma once

namespace Model
{
  class ControlPoint;
  class Document;
  class Node;
  class Path;
}

namespace Serialisation
{

class Layout
{
public:
  template <class TEndpoint>
  static Model::Document* process(TEndpoint& endpoint, Model::Document* document);

private:
  template <class TEndpoint>
  static void processNode(TEndpoint& endpoint, Model::Node* node);
  template <class TEndpoint>
  static void processControlPoint(TEndpoint& endpoint, Model::ControlPoint* controlPoint);
  template <class TEndpoint>
  static void processPathChunk(TEndpoint& endpoint, Model::Path* path);
  template <class TEndpoint>
  static void processPathElement(TEndpoint& endpoint, Model::Path* path);
};

}
