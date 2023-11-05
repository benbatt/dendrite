#include "controller/sketch.h"

namespace Controller
{

Sketch::Sketch(Model::Sketch* model)
  : mModel(model)
{
}

void Sketch::addNode(const Point& position, const Vector& controlA, const Vector& controlB)
{
  mModel->mNodes.push_back(Model::Node(position, controlA, controlB));
}

Node Sketch::controllerForNode(int index)
{
  return Node(&mModel->mNodes[index]);
}

}
