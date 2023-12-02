#pragma once

#include <vector>

namespace Controller
{
  class Sketch;
}

namespace Model
{

class ControlPoint;
class Node;
class Path;

class Sketch
{
public:
  typedef std::vector<ControlPoint*> ControlPointList;
  typedef std::vector<Node*> NodeList;
  typedef std::vector<Path*> PathList;

  const ControlPointList& controlPoints() const { return mControlPoints; }
  const NodeList& nodes() const { return mNodes; }
  const PathList& paths() const { return mPaths; }

private:
  friend class Controller::Sketch;

  ControlPointList mControlPoints;
  NodeList mNodes;
  PathList mPaths;
};

}
