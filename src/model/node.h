#pragma once

#include "utilities/geometry.h"

#include <vector>

namespace Controller
{
  class Node;
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

  Node(const Point& position, Type type)
    : mPosition(position)
    , mType(type)
  {}

  typedef std::vector<ControlPoint*> ControlPointList;

  const Point& position() const { return mPosition; }
  Type type() const { return mType; }
  const ControlPointList& controlPoints() const { return mControlPoints; }

private:
  friend class Controller::Node;

  ControlPointList mControlPoints;
  Point mPosition;
  Type mType;
};

}
