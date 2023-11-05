#pragma once

#include "model/node.h"

namespace Controller
{

class Node
{
public:
  Node(Model::Node* model);

  enum HandleType
  {
    Position,
    ControlA,
    ControlB,
  };

  Point handlePosition(HandleType type) const;
  void setHandlePosition(HandleType type, const Point& position);

private:
  Model::Node* mModel;
};

}
