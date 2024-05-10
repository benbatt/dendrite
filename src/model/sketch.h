#pragma once

#include "utilities/id.h"

#include <unordered_map>
#include <vector>

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
class Document;
class Node;
class Path;

class Sketch
{
public:
  Sketch(Document* parent);

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

  Accessor<Node> nodes() const { return Accessor(mNodes); }
  Accessor<ControlPoint> controlPoints() const { return Accessor(mControlPoints); }
  Accessor<Path> paths() const { return Accessor(mPaths); }
  Accessor<Sketch> sketches() const { return Accessor(mSketches); }

  Document* parent() const { return mParent; }

  ControlPoint* controlPoint(const ID<ControlPoint>& id) const;
  Node* node(const ID<Node>& id) const;
  Path* path(const ID<Path>& id) const;
  Sketch* sketch(const ID<Sketch>& id) const;

  struct DrawEntry
  {
    enum Type
    {
      Path,
      Sketch,
    };

    DrawEntry()
      : mType(Path)
      , mID(0)
    {}

    DrawEntry(const ID<Model::Path>& id)
      : mType(Path)
      , mID(id.value())
    {}

    DrawEntry(const ID<Model::Sketch>& id)
      : mType(Sketch)
      , mID(id.value())
    {}

    Type mType;
    IDValue mID;
  };

  typedef std::vector<DrawEntry> DrawOrder;
  const DrawOrder& drawOrder() const { return mDrawOrder; }

private:
  friend class Controller::Sketch;
  friend class Serialisation::Layout;
  friend class Serialisation::Reader;

  typedef std::unordered_map<ID<ControlPoint>, ControlPoint*> ControlPointList;
  typedef std::unordered_map<ID<Node>, Node*> NodeList;
  typedef std::unordered_map<ID<Path>, Path*> PathList;
  typedef std::unordered_map<ID<Sketch>, Sketch*> SketchList;

  ControlPointList mControlPoints;
  NodeList mNodes;
  PathList mPaths;
  SketchList mSketches;
  DrawOrder mDrawOrder;
  Document* mParent;
};

}
