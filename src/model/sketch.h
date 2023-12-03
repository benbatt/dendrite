#pragma once

#include "utilities/id.h"

#include <unordered_map>

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
  Sketch();

  template<class TAction>
  void forEachControlPoint(const TAction& action)
  {
    for (const auto& current : mControlPoints) {
      if (action(current.first, current.second)) {
        return;
      }
    }
  }

  template<class TAction>
  void forEachNode(const TAction& action)
  {
    for (const auto& current : mNodes) {
      if (action(current.first, current.second)) {
        return;
      }
    }
  }

  template<class TAction>
  void forEachPath(const TAction& action)
  {
    for (const auto& current : mPaths) {
      if (action(current.first, current.second)) {
        return;
      }
    }
  }

  Path* firstPath() const { return mPaths.begin()->second; }
  ID<Path> firstPathID() const { return mPaths.begin()->first; }

  ControlPoint* controlPoint(const ID<ControlPoint>& id) const;
  Node* node(const ID<Node>& id) const;
  Path* path(const ID<Path>& id) const;

private:
  friend class Controller::Sketch;

  typedef std::unordered_map<ID<ControlPoint>, ControlPoint*> ControlPointList;
  typedef std::unordered_map<ID<Node>, Node*> NodeList;
  typedef std::unordered_map<ID<Path>, Path*> PathList;

  ControlPointList mControlPoints;
  NodeList mNodes;
  PathList mPaths;
  IDValue mNextID;
};

}
