#pragma once

#include "utilities/geometry.h"

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
  ControlPoint(Node* node, const Point& position)
    : mPosition(position)
    , mNode(node)
  {}

  Node* node() const { return mNode; }
  const Point& position() const { return mPosition; }

private:
  friend class Controller::ControlPoint;

  Point mPosition;
  Node* mNode;
};

}
