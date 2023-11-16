#pragma once

#include "controller/node.h"

#include "model/sketch.h"

namespace Controller
{

class UndoManager;

class Sketch : private Node::SketchAccessor
{
public:
  Sketch(UndoManager* undoManager, Model::Sketch* model);

  void addNode(int index, const Point& position, const Vector& controlA, const Vector& controlB);
  Node controllerForNode(int index);

private:
  Model::Node* getNode(int index) override;

  UndoManager* mUndoManager;
  Model::Sketch* mModel;
};

}
