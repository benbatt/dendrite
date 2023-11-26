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
  class SketchAccessor
  {
  public:
    virtual Model::Node* getNode(int index) = 0;
  };

  Node(UndoManager* undoManager, SketchAccessor* sketchAccessor, int nodeIndex);

  enum HandleType
  {
    Position,
    ControlA,
    ControlB,
  };

  Point handlePosition(HandleType type) const;
  void setHandlePosition(HandleType type, const Point& position);

  using Type = Model::Node::Type;

  void setType(Type type);

private:
  friend class SetNodeTypeCommand;
  friend class SetNodePositionCommand;
  friend class SetNodeControlPointCommand;

  static Point& position(Model::Node* model);
  static Vector& controlA(Model::Node* model);
  static Vector& controlB(Model::Node* model);
  static Type& type(Model::Node* model);

  UndoManager* mUndoManager;
  SketchAccessor* mSketchAccessor;
  int mNodeIndex;
};

}
