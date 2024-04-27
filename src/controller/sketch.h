#pragma once

#include "controller/controlpoint.h"
#include "controller/node.h"
#include "controller/path.h"

#include "model/sketch.h"

#include "utilities/id.h"

#include <set>

namespace Controller
{

class UndoManager;

struct Selection
{
  void clear()
  {
    mPaths.clear();
    mNodes.clear();
    mControlPoints.clear();
  }

  bool contains(const ID<Model::Path>& id) const
  {
    return mPaths.count(id) > 0;
  }

  bool contains(const ID<Model::Node>& id) const
  {
    return mNodes.count(id) > 0;
  }

  bool contains(const ID<Model::ControlPoint>& id) const
  {
    return mControlPoints.count(id) > 0;
  }

  void add(const ID<Model::Path>& id, const Model::Sketch* sketch);
  void remove(const ID<Model::Path>& id, const Model::Sketch* sketch);

  void add(const ID<Model::Node>& id, const Model::Sketch* sketch);
  void remove(const ID<Model::Node>& id, const Model::Sketch* sketch);

  void add(const ID<Model::ControlPoint>& id);
  void remove(const ID<Model::ControlPoint>& id);

  bool operator==(const Selection& other) const
  {
    return mPaths == other.mPaths && mNodes == other.mNodes && mControlPoints == other.mControlPoints;
  }

  bool isEmpty() const
  {
    return mPaths.empty() && mNodes.empty() && mControlPoints.empty();
  }

  std::set<ID<Model::Path>> mPaths;
  std::set<ID<Model::Node>> mNodes;
  std::set<ID<Model::ControlPoint>> mControlPoints;
};

class Sketch : private ControlPoint::Accessor, private Node::Accessor, private Path::Accessor
{
public:
  Sketch(UndoManager* undoManager, Model::Sketch* model);
  virtual ~Sketch() { }

  ID<Model::Path> addPath();

  Path controllerForPath(const ID<Model::Path>& id);
  Node controllerForNode(const ID<Model::Node>& id);
  ControlPoint controllerForControlPoint(const ID<Model::ControlPoint>& id);

  void moveSelection(const Selection& selection, const Vector& offset);
  void bringPathForward(const ID<Model::Path>& id);
  void sendPathBackward(const ID<Model::Path>& id);
  void removeNode(const ID<Model::Node>& id);

private:
  friend class AddNodeCommand;
  friend class RemoveNodeCommand;
  friend class MoveSelectionCommand;

  // Node::Accessor and ControlPoint::Accessor
  Model::ControlPoint* getControlPoint(const ID<Model::ControlPoint>& id) override;
  Model::Node* getNode(const ID<Model::Node>& id) override;

  // Path::Accessor
  IDValue nextID() override;
  void createNode(const ID<Model::Node>& id, const Point& position, Model::Node::Type type) override;
  void destroyNode(const ID<Model::Node>& id) override;
  void createControlPoint(const ID<Model::ControlPoint>& id, const ID<Model::Node>& node,
    const Point& position) override;
  void destroyControlPoint(const ID<Model::ControlPoint>& id) override;
  Model::Path* getPath(const ID<Model::Path>& id) override;

  static Model::Sketch::NodeList& nodes(Model::Sketch* sketch);

  UndoManager* mUndoManager;
  Model::Sketch* mModel;
};

}
