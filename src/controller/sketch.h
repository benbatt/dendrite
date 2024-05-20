#pragma once

#include "controller/controlpoint.h"
#include "controller/node.h"
#include "controller/path.h"
#include "controller/selection.h"

#include "model/sketch.h"

#include "utilities/id.h"

namespace Controller
{

class UndoManager;

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
  ID<Model::Sketch> createSubSketch(const Selection& selection);

private:
  friend class AddNodeCommand;
  friend class RemoveNodeCommand;
  friend class MoveSelectionCommand;
  friend class CreateSubSketchCommand;

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

  static Model::Sketch::ControlPointList& controlPoints(Model::Sketch* sketch);
  static Model::Sketch::DrawOrder& drawOrder(Model::Sketch* sketch);
  static Model::Sketch::NodeList& nodes(Model::Sketch* sketch);
  static Model::Sketch::PathList& paths(Model::Sketch* sketch);
  static Model::Sketch::SketchList& sketches(Model::Sketch* sketch);
  static Point& position(Model::Sketch* sketch);

  UndoManager* mUndoManager;
  Model::Sketch* mModel;
};

}
