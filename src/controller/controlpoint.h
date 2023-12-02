#pragma once

#include "utilities/geometry.h"

namespace Model
{
  class ControlPoint;
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
    virtual Model::ControlPoint* getControlPoint(int index) = 0;
  };

  ControlPoint(UndoManager* undoManager, Accessor* accessor, int index);

  void setPosition(const Point& position);

private:
  friend class SetControlPointPositionCommand;
  friend class SetNodePositionCommand;

  static Point& position(Model::ControlPoint* model);

  UndoManager* mUndoManager;
  Accessor* mAccessor;
  int mIndex;
};

}
