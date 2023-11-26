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
  enum class Type
  {
    Symmetric,
    Smooth,
    Sharp,
  };

  Node(const Point& position, const Vector& controlA, const Vector& controlB, Type type)
    : mPosition(position)
    , mControlA(controlA)
    , mControlB(controlB)
    , mType(type)
  {
  }

  const Point& position() const { return mPosition; }
  const Vector& controlA() const { return mControlA; }
  const Vector& controlB() const { return mControlB; }
  Type type() const { return mType; }

private:
  friend class Controller::Node;

  Point mPosition;
  Vector mControlA; // relative to mPosition
  Vector mControlB; // relative to mPosition
  Type mType;
};

}
