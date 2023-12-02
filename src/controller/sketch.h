#pragma once

#include "controller/controlpoint.h"
#include "controller/node.h"
#include "controller/path.h"

#include "model/sketch.h"

namespace Controller
{

class UndoManager;

class Sketch : private ControlPoint::Accessor, private Node::Accessor, private Path::Accessor
{
public:
  Sketch(UndoManager* undoManager, Model::Sketch* model);

  void addPath();

  Path controllerForPath(int index);
  Node controllerForNode(int index);
  Node controllerForNode(const Model::Node* model);
  ControlPoint controllerForControlPoint(int index);

private:
  friend class AddNodeCommand;

  // Node::Accessor
  Model::Node* getNode(int index) override;

  // ControlPoint::Accessor
  Model::ControlPoint* getControlPoint(int index) override;

  // Path::Accessor
  Model::Node* createNode(const Point& position, Model::Node::Type type) override;
  void destroyNode(Model::Node* node) override;
  Model::ControlPoint* createControlPoint(Model::Node* node, const Point& position) override;
  void destroyControlPoint(Model::ControlPoint* controlPoint) override;
  Model::Path* getPath(int index) override;

  static Model::Sketch::NodeList& nodes(Model::Sketch* sketch);

  UndoManager* mUndoManager;
  Model::Sketch* mModel;
};

}
