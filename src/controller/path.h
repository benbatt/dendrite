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
    template <class TModel>
    ID<TModel> nextID()
    {
      return ID<TModel>(nextID());
    }

    virtual IDValue nextID() = 0;
    virtual void createNode(const ID<Model::Node>& id, const Point& position, Model::Node::Type type) = 0;
    virtual void destroyNode(const ID<Model::Node>& id) = 0;
    virtual void createControlPoint(const ID<Model::ControlPoint>& id, const ID<Model::Node>& node,
      const Point& position) = 0;
    virtual void destroyControlPoint(const ID<Model::ControlPoint>& controlPoint) = 0;
    virtual Model::Path* getPath(const ID<Model::Path>& id) = 0;
  };

  Path(UndoManager* undoManager, Accessor* accessor, const ID<Model::Path>& id);

  void addSymmetricNode(int index, const Point& position, const Point& controlA);
  void addSmoothNode(int index, const Point& position, const Point& controlA, double lengthB);
  void addSharpNode(int index, const Point& position);

private:
  friend class AddNodeCommand;

  static Model::Path::EntryList& entries(Model::Path* path);

  UndoManager* mUndoManager;
  Accessor* mAccessor;
  ID<Model::Path> mID;
};

}
