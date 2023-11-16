#include "controller/sketch.h"

#include "controller/undo.h"

namespace Controller
{

Sketch::Sketch(UndoManager *undoManager, Model::Sketch* model)
  : mUndoManager(undoManager)
  , mModel(model)
{
}

void Sketch::addNode(int index, const Point& position, const Vector& controlA, const Vector& controlB)
{
  if (index == 0) {
    mUndoManager->pushCommand(
      [=]() { mModel->mNodes.insert(mModel->mNodes.begin(), Model::Node(position, controlA, controlB)); },
      [=]() { mModel->mNodes.erase(mModel->mNodes.begin()); },
      "Add node");
  } else if (index == mModel->nodes().size()) {
    mUndoManager->pushCommand(
      [=]() { mModel->mNodes.push_back(Model::Node(position, controlA, controlB)); },
      [=]() { mModel->mNodes.pop_back(); },
      "Add node");
  }
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
