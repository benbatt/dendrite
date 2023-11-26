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

  void addSymmetricNode(int index, const Point& position, const Vector& controlA);
  void addSmoothNode(int index, const Point& position, const Vector& controlA, double lengthB);
  void addSharpNode(int index, const Point& position);

  Node controllerForNode(int index);

private:
  friend class AddNodeCommand;

  Model::Node* getNode(int index) override;

  static Model::Sketch::NodeList& nodes(Model::Sketch* sketch);

  UndoManager* mUndoManager;
  Model::Sketch* mModel;
};

}
