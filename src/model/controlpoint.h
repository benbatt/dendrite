#pragma once

#include "utilities/geometry.h"
#include "utilities/id.h"

namespace Controller
{
  class ControlPoint;
}

namespace Model
{

class Node;

class ControlPoint
{
public:
  ControlPoint(const ID<Node>& node, const Point& position)
    : mPosition(position)
    , mNode(node)
  {}

  const ID<Node>& node() const { return mNode; }
  const Point& position() const { return mPosition; }

private:
  friend class Controller::ControlPoint;

  Point mPosition;
  ID<Node> mNode;
};

}
