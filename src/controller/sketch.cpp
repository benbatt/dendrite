#include "controller/sketch.h"

#include "controller/undo.h"
#include "model/controlpoint.h"

#include <cassert>

namespace Controller
{

Sketch::Sketch(UndoManager *undoManager, Model::Sketch* model)
  : mUndoManager(undoManager)
  , mModel(model)
{
}

ID<Model::Path> Sketch::addPath()
{
  ID<Model::Path> id = Path::Accessor::nextID<Model::Path>();

  mUndoManager->pushCommand(
    [this, id]() { mModel->mPaths[id] = new Model::Path; },
    [this, id]() {
      delete mModel->mPaths.at(id);
      mModel->mPaths.erase(id);
    },
    "Add path");

  return id;
}

Path Sketch::controllerForPath(const ID<Model::Path>& id)
{
  return Path(mUndoManager, this, id);
}

Node Sketch::controllerForNode(const ID<Model::Node>& id)
{
  return Node(mUndoManager, this, id);
}

ControlPoint Sketch::controllerForControlPoint(const ID<Model::ControlPoint>& id)
{
  return ControlPoint(mUndoManager, this, id);
}

void Sketch::removeNode(const ID<Model::Node>& nodeID)
{
  const Model::Node* node = mModel->node(nodeID);

  mUndoManager->beginGroup();

  // Destroy path entries
  for (auto current : mModel->paths()) {
    const Model::Path::EntryList& entries = current.second->entries();

    for (int i = 0; i < entries.size(); ) {
      if (entries[i].mNode == nodeID) {
        controllerForPath(current.first).removeEntry(i);
      } else {
        ++i;
      }
    }
  }

  // Destroy control points
  for (auto controlPointID : node->controlPoints()) {
    Point position = mModel->controlPoint(controlPointID)->position();

    mUndoManager->pushCommand(
      [=]() { destroyControlPoint(controlPointID); },
      [=]() { createControlPoint(controlPointID, nodeID, position); },
      "Destroy control point");
  }

  // Destroy node
  {
    Point position = node->position();
    Model::Node::Type type = node->type();

    mUndoManager->pushCommand(
      [=]() { destroyNode(nodeID); },
      [=]() { createNode(nodeID, position, type); },
      "Destroy node");
  }

  mUndoManager->endGroup();
}

Model::Node* Sketch::getNode(const ID<Model::Node>& id)
{
  return mModel->node(id);
}

Model::ControlPoint* Sketch::getControlPoint(const ID<Model::ControlPoint>& id)
{
  return mModel->controlPoint(id);
}

IDValue Sketch::nextID()
{
  IDValue value = mModel->mNextID;
  ++mModel->mNextID;

  return value;
}

void Sketch::createNode(const ID<Model::Node>& id, const Point& position, Model::Node::Type type)
{
  assert(mModel->mNodes.find(id) == mModel->mNodes.end());

  Model::Node* node = new Model::Node(position, type);
  mModel->mNodes[id] = node;
}

void Sketch::destroyNode(const ID<Model::Node>& id)
{
  delete mModel->node(id);
  mModel->mNodes.erase(id);
}

void Sketch::createControlPoint(const ID<Model::ControlPoint>& id, const ID<Model::Node>& nodeID, const Point& position)
{
  assert(mModel->mControlPoints.find(id) == mModel->mControlPoints.end());

  Model::ControlPoint* controlPoint = new Model::ControlPoint(nodeID, position);

  Node::controlPoints(mModel->node(nodeID)).push_back(id);
  mModel->mControlPoints[id] = controlPoint;
}

void Sketch::destroyControlPoint(const ID<Model::ControlPoint>& id)
{
  delete mModel->controlPoint(id);
  mModel->mControlPoints.erase(id);
}

Model::Path* Sketch::getPath(const ID<Model::Path>& id)
{
  return mModel->path(id);
}

Model::Sketch::NodeList& Sketch::nodes(Model::Sketch* sketch)
{
  return sketch->mNodes;
}

}
