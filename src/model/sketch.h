#pragma once

#include "utilities/id.h"

#include <unordered_map>

namespace Controller
{
  class Sketch;
}

namespace Serialisation
{
  class Layout;
  class Reader;
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

  template <class TModel>
  class Accessor
  {
  public:
    auto begin() const { return mCollection.begin(); }
    auto end() const { return mCollection.end(); }

  private:
    friend class Sketch;

    Accessor(const std::unordered_map<ID<TModel>, TModel*>& collection)
      : mCollection(collection)
    {}

    const std::unordered_map<ID<TModel>, TModel*>& mCollection;
  };

  Accessor<Model::Node> nodes() const { return Accessor(mNodes); }
  Accessor<Model::ControlPoint> controlPoints() const { return Accessor(mControlPoints); }
  Accessor<Model::Path> paths() const { return Accessor(mPaths); }

  ControlPoint* controlPoint(const ID<ControlPoint>& id) const;
  Node* node(const ID<Node>& id) const;
  Path* path(const ID<Path>& id) const;

private:
  friend class Controller::Sketch;
  friend class Serialisation::Layout;
  friend class Serialisation::Reader;

  typedef std::unordered_map<ID<ControlPoint>, ControlPoint*> ControlPointList;
  typedef std::unordered_map<ID<Node>, Node*> NodeList;
  typedef std::unordered_map<ID<Path>, Path*> PathList;

  ControlPointList mControlPoints;
  NodeList mNodes;
  PathList mPaths;
  IDValue mNextID;
};

}
