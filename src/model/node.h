#pragma once

#include "utilities/geometry.h"

namespace Controller
{
  class Node;
}

namespace Model
{

class Node
{
public:
  Node(const Point& position, const Vector& controlA, const Vector& controlB)
    : mPosition(position)
    , mControlA(controlA)
    , mControlB(controlB)
  {
  }

  const Point& position() const { return mPosition; }
  const Vector& controlA() const { return mControlA; }
  const Vector& controlB() const { return mControlB; }

private:
  friend class Controller::Node;

  Point mPosition;
  Vector mControlA; // relative to mPosition
  Vector mControlB; // relative to mPosition
};

}
