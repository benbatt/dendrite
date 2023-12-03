#pragma once

#include "utilities/geometry.h"
#include "utilities/id.h"

namespace Model
{
  class ControlPoint;
  class Node;
}

namespace Controller
{

class UndoManager;

class ControlPoint
{
public:
  class Accessor
  {
  public:
    virtual Model::ControlPoint* getControlPoint(const ID<Model::ControlPoint>& id) = 0;
    virtual Model::Node* getNode(const ID<Model::Node>& id) = 0;
  };

  ControlPoint(UndoManager* undoManager, Accessor* accessor, const ID<Model::ControlPoint>& id);

  void setPosition(const Point& position);

private:
  friend class SetControlPointPositionCommand;
  friend class SetNodePositionCommand;

  static Point& position(Model::ControlPoint* model);

  UndoManager* mUndoManager;
  Accessor* mAccessor;
  const ID<Model::ControlPoint>& mID;
};

}
