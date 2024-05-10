#pragma once

#include "utilities/id.h"

#include <set>

namespace Model
{

class ControlPoint;
class Node;
class Path;
class Reference;
class Sketch;

}

namespace Controller
{

struct Selection
{
  void clear()
  {
    mPaths.clear();
    mNodes.clear();
    mControlPoints.clear();
  }

  bool contains(const Model::Reference& reference) const;
  void add(const Model::Reference& reference, const Model::Sketch* sketch);
  void remove(const Model::Reference& reference, const Model::Sketch* sketch);

  bool contains(const ID<Model::Path>& id) const
  {
    return mPaths.count(id) > 0;
  }

  bool contains(const ID<Model::Node>& id) const
  {
    return mNodes.count(id) > 0;
  }

  bool contains(const ID<Model::ControlPoint>& id) const
  {
    return mControlPoints.count(id) > 0;
  }

  void add(const ID<Model::Path>& id, const Model::Sketch* sketch);
  void remove(const ID<Model::Path>& id, const Model::Sketch* sketch);

  void add(const ID<Model::Node>& id, const Model::Sketch* sketch);
  void remove(const ID<Model::Node>& id, const Model::Sketch* sketch);

  void add(const ID<Model::ControlPoint>& id);
  void remove(const ID<Model::ControlPoint>& id);

  bool operator==(const Selection& other) const
  {
    return mPaths == other.mPaths && mNodes == other.mNodes && mControlPoints == other.mControlPoints;
  }

  bool isEmpty() const
  {
    return mPaths.empty() && mNodes.empty() && mControlPoints.empty();
  }

  std::set<ID<Model::Path>> mPaths;
  std::set<ID<Model::Node>> mNodes;
  std::set<ID<Model::ControlPoint>> mControlPoints;
};

}
