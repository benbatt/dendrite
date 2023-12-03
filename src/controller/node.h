#pragma once

#include "utilities/geometry.h"

#include "model/node.h"

namespace Controller
{

class UndoManager;
class UndoCommand;

class Node
{
public:
  class Accessor
  {
  public:
    virtual Model::ControlPoint* getControlPoint(const ID<Model::ControlPoint>& id) = 0;
    virtual Model::Node* getNode(const ID<Model::Node>& id) = 0;
  };

  Node(UndoManager* undoManager, Accessor* accessor, const ID<Model::Node>& id);

  void setPosition(const Point& position);

  using Type = Model::Node::Type;

  void setType(Type type);

private:
  friend class SetNodePositionCommand;
  friend class SetNodeTypeCommand;
  friend class Sketch;

  static Point& position(Model::Node* model);
  static Type& type(Model::Node* model);
  static Model::Node::ControlPointList& controlPoints(Model::Node* model);

  UndoManager* mUndoManager;
  Accessor* mAccessor;
  ID<Model::Node> mID;
};

}
