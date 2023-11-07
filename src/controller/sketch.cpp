#include "controller/sketch.h"

#include "controller/undo.h"

namespace Controller
{

Sketch::Sketch(UndoManager *undoManager, Model::Sketch* model)
  : mUndoManager(undoManager)
  , mModel(model)
{
}

void Sketch::addNode(const Point& position, const Vector& controlA, const Vector& controlB)
{
  mUndoManager->pushCommand(
    [=]() { mModel->mNodes.push_back(Model::Node(position, controlA, controlB)); },
    [=]() { mModel->mNodes.pop_back(); });
}

Node Sketch::controllerForNode(int index)
{
  return Node(mUndoManager, this, index);
}

Model::Node* Sketch::getNode(int index)
{
  return &mModel->mNodes[index];
}

}
