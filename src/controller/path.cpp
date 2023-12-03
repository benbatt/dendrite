#include "controller/path.h"

#include "controller/undo.h"

namespace Controller
{

Path::Path(UndoManager* undoManager, Accessor* accessor, const ID<Model::Path>& id)
  : mUndoManager(undoManager)
  , mAccessor(accessor)
  , mID(id)
{
}

using NodeType = Model::Node::Type;

class AddNodeCommand : public UndoCommand
{
public:
  enum class Position
  {
    Start,
    End,
  };

  AddNodeCommand(Path::Accessor* accessor, const ID<Model::Path>& id, int entryIndex, const Point& position,
    const Point& preControl, const Point& postControl, NodeType type)
    : mAccessor(accessor)
    , mAddPosition()
    , mPosition(position)
    , mPreControlPosition(preControl)
    , mPostControlPosition(postControl)
    , mType(type)
    , mID(id)
    , mNodeID(accessor->nextID<Model::Node>())
    , mPreControlID(accessor->nextID<Model::ControlPoint>())
    , mPostControlID(accessor->nextID<Model::ControlPoint>())
  {
    Model::Path* model = mAccessor->getPath(mID);

    if (entryIndex == 0) {
      mAddPosition = Position::Start;
    } else if (entryIndex == model->entries().size()) {
      mAddPosition = Position::End;
    }
  }

  void redo() override
  { 
    mAccessor->createNode(mNodeID, mPosition, mType);
    mAccessor->createControlPoint(mPreControlID, mNodeID, mPreControlPosition);
    mAccessor->createControlPoint(mPostControlID, mNodeID, mPostControlPosition);

    Model::Path::Entry entry {
      .mNode = mNodeID,
      .mPreControl = mPreControlID,
      .mPostControl = mPostControlID,
    };

    Model::Path* model = mAccessor->getPath(mID);
    Model::Path::EntryList& entries = Path::entries(model);

    if (mAddPosition == Position::Start) {
      entries.insert(entries.begin(), entry);
    } else if (mAddPosition == Position::End) {
      entries.push_back(entry);
    }
  }

  void undo() override
  { 
    Model::Path* model = mAccessor->getPath(mID);
    Model::Path::EntryList& entries = Path::entries(model);

    Model::Path::Entry entry;

    if (mAddPosition == Position::Start) {
      entry = entries.front();
      entries.erase(entries.begin());
    } else if (mAddPosition == Position::End) {
      entry = entries.back();
      entries.pop_back();
    }

    mAccessor->destroyNode(mNodeID);
    mAccessor->destroyControlPoint(mPreControlID);
    mAccessor->destroyControlPoint(mPostControlID);
  }

  std::string description() override
  {
    return "Add node";
  }

private:
  Path::Accessor* mAccessor;
  Position mAddPosition;
  Point mPosition;
  Point mPreControlPosition;
  Point mPostControlPosition;
  NodeType mType;
  ID<Model::Path> mID;
  ID<Model::Node> mNodeID;
  ID<Model::ControlPoint> mPreControlID;
  ID<Model::ControlPoint> mPostControlID;
};

void Path::addSymmetricNode(int index, const Point& position, const Point& controlA)
{
  Vector offsetA = controlA - position;

  mUndoManager->pushCommand(
    new AddNodeCommand(mAccessor, mID, index, position, controlA, position - offsetA, NodeType::Symmetric));
}

void Path::addSmoothNode(int index, const Point& position, const Point& controlA, double lengthB)
{
  Vector offsetA = controlA - position;

  mUndoManager->pushCommand(
      new AddNodeCommand(mAccessor, mID, index, position, controlA, position - offsetA.normalised() * lengthB,
        NodeType::Smooth));
}

void Path::addSharpNode(int index, const Point& position)
{
  mUndoManager->pushCommand(new AddNodeCommand(mAccessor, mID, index, position, position, position,
        NodeType::Sharp));
}

Model::Path::EntryList& Path::entries(Model::Path* path)
{
  return path->mEntries;
}

}
