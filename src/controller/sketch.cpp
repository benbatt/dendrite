#include "controller/sketch.h"

#include "controller/controlpoint.h"
#include "controller/node.h"
#include "controller/undo.h"
#include "model/controlpoint.h"
#include "model/document.h"

#include <cassert>
#include <map>

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

class MoveSelectionCommand : public UndoManager::AutoIDCommand<MoveSelectionCommand>
{
public:
  MoveSelectionCommand(Sketch* sketch, const Selection& selection, const Vector& offset)
    : mSketch(sketch)
    , mSelection(selection)
  {
    const int PointCount = mSelection.count(Model::Type::Node)
      + mSelection.count(Model::Type::ControlPoint);

    mOrigins.reserve(PointCount);
    mDestinations.reserve(PointCount);

    mSelection.forEachNode(mSketch->mModel,
      [this, &offset](Model::Node* node)
      {
        mOrigins.push_back(node->position());
        mDestinations.push_back(node->position() + offset);
      });

    mSelection.forEachControlPoint(mSketch->mModel,
      [this, &offset](Model::ControlPoint* point)
      {
        mOrigins.push_back(point->position());
        mDestinations.push_back(point->position() + offset);
      });
  }

  void redo() override
  {
    auto pointIterator = mDestinations.begin();

    mSelection.forEachNode(mSketch->mModel,
      [this, &pointIterator](Model::Node* node)
      {
        Node::position(node) = *pointIterator;
        ++pointIterator;
      });

    mSelection.forEachControlPoint(mSketch->mModel,
      [this, &pointIterator](Model::ControlPoint* point)
      {
        ControlPoint::position(point) = *pointIterator;
        ++pointIterator;
      });
  }

  void undo() override
  {
    auto pointIterator = mOrigins.begin();

    mSelection.forEachNode(mSketch->mModel,
      [this, &pointIterator](Model::Node* node)
      {
        Node::position(node) = *pointIterator;
        ++pointIterator;
      });

    mSelection.forEachControlPoint(mSketch->mModel,
      [this, &pointIterator](Model::ControlPoint* point)
      {
        ControlPoint::position(point) = *pointIterator;
        ++pointIterator;
      });
  }

  std::string description() override
  {
    return "Move path";
  }

  bool mergeWith(UndoCommand* other) override
  {
    MoveSelectionCommand* command = static_cast<MoveSelectionCommand*>(other);

    if (command->mSketch == mSketch && command->mSelection == mSelection) {
      mDestinations = std::move(command->mDestinations);
      return true;
    } else {
      return false;
    }
  }

private:
  Sketch* mSketch;
  Selection mSelection;
  std::vector<Point> mOrigins;
  std::vector<Point> mDestinations;
};

void Sketch::moveSelection(const Selection& selection, const Vector& offset)
{
  mUndoManager->pushCommand(new MoveSelectionCommand(this, selection, offset));
}

Model::Sketch::DrawOrder::iterator findDrawEntry(Model::Sketch::DrawOrder& drawOrder,
  const ID<Model::Path>& id)
{
  return std::find(drawOrder.begin(), drawOrder.end(), id);
}

Model::Sketch::DrawOrder::const_iterator findDrawEntry(const Model::Sketch::DrawOrder& drawOrder,
  const ID<Model::Path>& id)
{
  return std::find(drawOrder.begin(), drawOrder.end(), id);
}

Model::Sketch::DrawOrder::iterator findDrawEntry(Model::Sketch::DrawOrder& drawOrder,
  const ID<Model::Sketch>& id)
{
  return std::find(drawOrder.begin(), drawOrder.end(), id);
}

void Sketch::bringPathForward(const ID<Model::Path>& id)
{
  Model::Sketch::DrawOrder& drawOrder = mModel->mDrawOrder;
  auto it = findDrawEntry(drawOrder, id);
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
  auto it = findDrawEntry(drawOrder, id);

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

class CreateSubSketchCommand : public UndoCommand
{
public:
  CreateSubSketchCommand(Sketch* sketch, const Selection& selection, const ID<Model::Sketch>& id)
    : mSketch(sketch)
    , mSelection(selection)
    , mID(id)
  {
    const Model::Sketch::DrawOrder& drawOrder = mSketch->mModel->drawOrder();

    mSelection.forEachPathID(
      [this, &drawOrder](const ID<Model::Path>& id)
      {
        auto it = findDrawEntry(drawOrder, id);
        int index = std::distance(drawOrder.begin(), it);
        mOldDrawOrder[index] = id;
      });
  }

  void redo() override
  {
    Model::Sketch* subSketch = new Model::Sketch(mSketch->mModel->parent());
    Sketch::sketches(mSketch->mModel)[mID] = subSketch;

    Model::Sketch::DrawOrder& drawOrder = Sketch::drawOrder(mSketch->mModel);
    int drawIndex = !mOldDrawOrder.empty() ? (mOldDrawOrder.rbegin()->first + 1) : drawOrder.size();
    drawOrder.insert(drawOrder.begin() + drawIndex, mID);

    auto addControlPoint = [this, subSketch](const ID<Model::ControlPoint>& id)
    {
      if (Sketch::controlPoints(subSketch).count(id) == 0) {
        Model::ControlPoint* controlPoint = mSketch->getControlPoint(id);
        Sketch::controlPoints(subSketch)[id] = controlPoint;
      }
    };

    auto addNode = [this, subSketch, &addControlPoint](const ID<Model::Node>& id)
    {
      if (Sketch::nodes(subSketch).count(id) == 0) {
        Model::Node* node = mSketch->getNode(id);
        Sketch::nodes(subSketch)[id] = node;

        for (auto controlPointID : node->controlPoints()) {
          addControlPoint(controlPointID);
        }
      }
    };

    auto movePath = [this, subSketch, &addNode](const ID<Model::Path>& id)
    {
      Model::Path* path = mSketch->mModel->path(id);

      Sketch::paths(subSketch)[id] = path;
      Sketch::paths(mSketch->mModel).erase(id);

      Sketch::drawOrder(subSketch).push_back(id);
      Model::Sketch::DrawOrder& drawOrder = Sketch::drawOrder(mSketch->mModel);
      drawOrder.erase(findDrawEntry(drawOrder, id));

      for (auto entry: path->entries()) {
        addNode(entry.mNode);
      }
    };

    for (auto& [index, id] : mOldDrawOrder) {
      movePath(id);
    }

    for (auto& reference : mSelection.mReferences) {
      if (reference.type() == Model::Type::Node) {
        addNode(reference.id<Model::Node>());
      } else if (reference.type() == Model::Type::ControlPoint) {
        addControlPoint(reference.id<Model::ControlPoint>());
      }
    }
  }

  void undo() override
  {
    Model::Sketch* subSketch = mSketch->mModel->sketch(mID);

    Model::Sketch::DrawOrder& drawOrder = Sketch::drawOrder(mSketch->mModel);
    drawOrder.erase(findDrawEntry(drawOrder, mID));

    mSelection.forEachPathID(
      [this, subSketch](const ID<Model::Path>& id)
      {
        Sketch::paths(mSketch->mModel)[id] = subSketch->path(id);
      });

    for (auto [index, id] : mOldDrawOrder) {
      drawOrder.insert(drawOrder.begin() + index, id);
    }

    delete subSketch;
    Sketch::sketches(mSketch->mModel).erase(mID);
  }

  std::string description() override
  {
    return "Create sub-sketch";
  }

private:
  Sketch* mSketch;
  Selection mSelection;
  std::map<int, ID<Model::Path>> mOldDrawOrder;
  ID<Model::Sketch> mID;
};

ID<Model::Sketch> Sketch::createSubSketch(const Selection& selection)
{
  ID<Model::Sketch> id(nextID());

  mUndoManager->pushCommand(new CreateSubSketchCommand(this, selection, id));

  return id;
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
  IDValue value = mModel->mParent->mNextID;
  ++mModel->mParent->mNextID;

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

Model::Sketch::ControlPointList& Sketch::controlPoints(Model::Sketch* sketch)
{
  return sketch->mControlPoints;
}

Model::Sketch::DrawOrder& Sketch::drawOrder(Model::Sketch* sketch)
{
  return sketch->mDrawOrder;
}

Model::Sketch::NodeList& Sketch::nodes(Model::Sketch* sketch)
{
  return sketch->mNodes;
}

Model::Sketch::PathList& Sketch::paths(Model::Sketch* sketch)
{
  return sketch->mPaths;
}

Model::Sketch::SketchList& Sketch::sketches(Model::Sketch* sketch)
{
  return sketch->mSketches;
}

}
