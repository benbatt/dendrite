#pragma once

#include "model/node.h"
#include "model/path.h"
#include "utilities/geometry.h"

namespace Controller
{

class UndoManager;

class Path
{
public:
  class Accessor
  {
  public:
    virtual Model::Node* createNode(const Point& position, Model::Node::Type type) = 0;
    virtual void destroyNode(Model::Node* node) = 0;
    virtual Model::ControlPoint* createControlPoint(Model::Node* node, const Point& position) = 0;
    virtual void destroyControlPoint(Model::ControlPoint* controlPoint) = 0;
    virtual Model::Path* getPath(int index) = 0;
  };

  Path(UndoManager* undoManager, Accessor* accessor, int pathIndex);

  void addSymmetricNode(int index, const Point& position, const Point& controlA);
  void addSmoothNode(int index, const Point& position, const Point& controlA, double lengthB);
  void addSharpNode(int index, const Point& position);

private:
  friend class AddNodeCommand;

  static Model::Path::EntryList& entries(Model::Path* path);

  UndoManager* mUndoManager;
  Accessor* mAccessor;
  int mPathIndex;
};

}
