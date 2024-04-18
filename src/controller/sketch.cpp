#include "controller/sketch.h"

#include "controller/controlpoint.h"
#include "controller/node.h"
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
    [this, id]() {
      mModel->mPaths[id] = new Model::Path;
      mModel->mDrawOrder.push_back(id);
    },
    [this, id]() {
      delete mModel->mPaths.at(id);
      mModel->mPaths.erase(id);
      mModel->mDrawOrder.pop_back();
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

class MovePathCommand : public UndoManager::AutoIDCommand<MovePathCommand>
{
public:
  MovePathCommand(Sketch* sketch, const ID<Model::Path>& id, const Vector& offset)
    : mSketch(sketch)
    , mID(id)
  {
    Model::Path* path = mSketch->getPath(mID);

    // node, precontrol, postcontrol
    const int PointCount = path->entries().size() * 3;

    mOrigins.reserve(PointCount);
    mDestinations.reserve(PointCount);

    for (auto& entry : path->entries()) {
      Model::Node* node = mSketch->getNode(entry.mNode);
      Model::ControlPoint* preControl = mSketch->getControlPoint(entry.mPreControl);
      Model::ControlPoint* postControl = mSketch->getControlPoint(entry.mPostControl);

      mOrigins.push_back(node->position());
      mOrigins.push_back(preControl->position());
      mOrigins.push_back(postControl->position());

      mDestinations.push_back(node->position() + offset);
      mDestinations.push_back(preControl->position() + offset);
      mDestinations.push_back(postControl->position() + offset);
    }
  }

  void redo() override
  {
    Model::Path* path = mSketch->getPath(mID);

    auto pointIterator = mDestinations.begin();

    for (auto& entry : path->entries()) {
      Node::position(mSketch->getNode(entry.mNode)) = *pointIterator;
      ++pointIterator;
      ControlPoint::position(mSketch->getControlPoint(entry.mPreControl)) = *pointIterator;
      ++pointIterator;
      ControlPoint::position(mSketch->getControlPoint(entry.mPostControl)) = *pointIterator;
      ++pointIterator;
    }
  }

  void undo() override
  {
    Model::Path* path = mSketch->getPath(mID);

    auto pointIterator = mOrigins.begin();

    for (auto& entry : path->entries()) {
      Node::position(mSketch->getNode(entry.mNode)) = *pointIterator;
      ++pointIterator;
      ControlPoint::position(mSketch->getControlPoint(entry.mPreControl)) = *pointIterator;
      ++pointIterator;
      ControlPoint::position(mSketch->getControlPoint(entry.mPostControl)) = *pointIterator;
      ++pointIterator;
    }
  }

  std::string description() override
  {
    return "Move path";
  }

  bool mergeWith(UndoCommand* other) override
  {
    MovePathCommand* command = static_cast<MovePathCommand*>(other);

    if (command->mSketch == mSketch && command->mID == mID) {
      mDestinations = std::move(command->mDestinations);
      return true;
    } else {
      return false;
    }
  }

private:
  Sketch* mSketch;
  ID<Model::Path> mID;
  std::vector<Point> mOrigins;
  std::vector<Point> mDestinations;
};

void Sketch::movePath(const ID<Model::Path>& id, const Vector& offset)
{
  mUndoManager->pushCommand(new MovePathCommand(this, id, offset));
}

void Sketch::bringPathForward(const ID<Model::Path>& id)
{
  Model::Sketch::DrawOrder& drawOrder = mModel->mDrawOrder;
  auto it = std::find(drawOrder.begin(), drawOrder.end(), id);

  assert(it != drawOrder.end());

  int index = std::distance(drawOrder.begin(), it);

  if (index == drawOrder.size() - 1) {
    return;
  }

  mUndoManager->pushCommand(
    [=]() { std::swap(mModel->mDrawOrder[index], mModel->mDrawOrder[index + 1]); },
    [=]() { std::swap(mModel->mDrawOrder[index], mModel->mDrawOrder[index + 1]); },
    "Bring path forward");
}

void Sketch::sendPathBackward(const ID<Model::Path>& id)
{
  Model::Sketch::DrawOrder& drawOrder = mModel->mDrawOrder;
  auto it = std::find(drawOrder.begin(), drawOrder.end(), id);

  assert(it != drawOrder.end());

  int index = std::distance(drawOrder.begin(), it);

  if (index == 0) {
    return;
  }

  mUndoManager->pushCommand(
    [=]() { std::swap(mModel->mDrawOrder[index], mModel->mDrawOrder[index - 1]); },
    [=]() { std::swap(mModel->mDrawOrder[index], mModel->mDrawOrder[index - 1]); },
    "Send path backward");
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
