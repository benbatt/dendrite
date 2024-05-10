#pragma once

#include "utilities/id.h"

namespace Serialisation
{
  class Layout;
}

namespace Model
{

class ControlPoint;
class Node;
class Path;
class Sketch;

class Reference
{
public:
  enum class Type
  {
    Path,
    Node,
    ControlPoint,
    Sketch,
    Null,
  };

  Reference(Type type, int id)
    : mType(type)
    , mID(id)
  {}

  Reference()
    : Reference(Type::Null, 0)
  {}

  Reference(const ID<ControlPoint>& id)
    : Reference(Type::ControlPoint, id.value())
  {}

  Reference(const ID<Node>& id)
    : Reference(Type::Node, id.value())
  {}

  Reference(const ID<Path>& id)
    : Reference(Type::Path, id.value())
  {}

  Reference(const ID<Sketch>& id)
    : Reference(Type::Sketch, id.value())
  {}

  Type type() const { return mType; }

  template<class TModel>
  ID<TModel> id() const { return ID<TModel>(mID); }

  bool isValid() const { return mID > 0; }
  bool refersTo(Type type) const { return mID > 0 && mType == type; }
  ControlPoint* controlPoint(const Sketch* sketch) const;
  Node* node(const Sketch* sketch) const;

  bool operator==(const Reference& other) const { return mType == other.mType && mID == other.mID; }
  bool operator!=(const Reference& other) const { return !(*this == other); }
  bool operator<(const Reference& other) const;

private:
  friend class Serialisation::Layout;

  Type mType;
  IDValue mID;
};

}
