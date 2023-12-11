#pragma once

namespace Model
{
  class ControlPoint;
  class Node;
  class Path;
  class Sketch;
}

namespace Serialisation
{

class Layout
{
public:
  template <class TEndpoint>
  static Model::Sketch* process(TEndpoint& endpoint, Model::Sketch* sketch);

private:
  template <class TEndpoint>
  static void processNode(TEndpoint& endpoint, Model::Node* node);
  template <class TEndpoint>
  static void processControlPoint(TEndpoint& endpoint, Model::ControlPoint* controlPoint);
  template <class TEndpoint>
  static void processPath(TEndpoint& endpoint, Model::Path* path);
};

}
