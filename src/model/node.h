#pragma once

#include "utilities/geometry.h"
#include "utilities/id.h"

#include <vector>

namespace Controller
{
  class Node;
}

namespace Serialisation
{
  class Layout;
}

namespace Model
{

class ControlPoint;

class Node
{
public:
  enum class Type
  {
    Symmetric,
    Smooth,
    Sharp,
  };

  Node()
    : Node({0, 0}, Type::Symmetric)
  {}

  Node(const Point& position, Type type)
    : mPosition(position)
    , mType(type)
  {}

  typedef std::vector<ID<ControlPoint>> ControlPointList;

  const Point& position() const { return mPosition; }
  Type type() const { return mType; }
  const ControlPointList& controlPoints() const { return mControlPoints; }

private:
  friend class Controller::Node;
  friend class Serialisation::Layout;

  ControlPointList mControlPoints;
  Point mPosition;
  Type mType;
};

}
