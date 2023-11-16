#pragma once

#include "utilities/geometry.h"

namespace Model
{
  class Node;
}

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

  enum class SetPositionMode
  {
    Smooth,
    Symmetrical,
  };

  Point handlePosition(HandleType type) const;
  void setHandlePosition(HandleType type, const Point& position, SetPositionMode mode);

private:
  friend class SetNodePositionCommand;
  friend class SetNodeControlPointCommand;

  static Point& position(Model::Node* model);
  static Vector& controlA(Model::Node* model);
  static Vector& controlB(Model::Node* model);

  UndoManager* mUndoManager;
  SketchAccessor* mSketchAccessor;
  int mNodeIndex;
};

}
