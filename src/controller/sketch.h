#pragma once

#include "controller/node.h"

#include "model/sketch.h"

namespace Controller
{

class Sketch
{
public:
  Sketch(Model::Sketch* model);

  void addNode(const Point& position, const Vector& controlA, const Vector& controlB);
  Node controllerForNode(int index);

private:
  Model::Sketch* mModel;
};

}
