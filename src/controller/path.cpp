#include "controller/path.h"

#include "controller/undo.h"

namespace Controller
{

Path::Path(UndoManager* undoManager, Accessor* accessor, int pathIndex)
  : mUndoManager(undoManager)
  , mAccessor(accessor)
  , mPathIndex(pathIndex)
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

  AddNodeCommand(Path::Accessor* accessor, int pathIndex, int entryIndex, const Point& position,
    const Point& preControl, const Point& postControl, NodeType type)
    : mAccessor(accessor)
    , mAddPosition()
    , mPosition(position)
    , mPreControl(preControl)
    , mPostControl(postControl)
    , mType(type)
    , mPathIndex(pathIndex)
  {
    Model::Path* model = mAccessor->getPath(mPathIndex);

    if (entryIndex == 0) {
      mAddPosition = Position::Start;
    } else if (entryIndex == model->entries().size()) {
      mAddPosition = Position::End;
    }
  }

  void redo() override
  { 
    Model::Node* node = mAccessor->createNode(mPosition, mType);

    Model::Path::Entry entry {
      .mNode = node,
      .mPreControl = mAccessor->createControlPoint(node, mPreControl),
      .mPostControl = mAccessor->createControlPoint(node, mPostControl),
    };

    Model::Path* model = mAccessor->getPath(mPathIndex);
    Model::Path::EntryList& entries = Path::entries(model);

    if (mAddPosition == Position::Start) {
      entries.insert(entries.begin(), entry);
    } else if (mAddPosition == Position::End) {
      entries.push_back(entry);
    }
  }

  void undo() override
  { 
    Model::Path* model = mAccessor->getPath(mPathIndex);
    Model::Path::EntryList& entries = Path::entries(model);

    Model::Path::Entry entry;

    if (mAddPosition == Position::Start) {
      entry = entries.front();
      entries.erase(entries.begin());
    } else if (mAddPosition == Position::End) {
      entry = entries.back();
      entries.pop_back();
    }

    mAccessor->destroyNode(entry.mNode);
    mAccessor->destroyControlPoint(entry.mPreControl);
    mAccessor->destroyControlPoint(entry.mPostControl);
  }

  std::string description() override
  {
    return "Add node";
  }

private:
  Path::Accessor* mAccessor;
  Position mAddPosition;
  Point mPosition;
  Point mPreControl;
  Point mPostControl;
  NodeType mType;
  int mPathIndex;
};

void Path::addSymmetricNode(int index, const Point& position, const Point& controlA)
{
  Vector offsetA = controlA - position;

  mUndoManager->pushCommand(
    new AddNodeCommand(mAccessor, mPathIndex, index, position, controlA, position - offsetA, NodeType::Symmetric));
}

void Path::addSmoothNode(int index, const Point& position, const Point& controlA, double lengthB)
{
  Vector offsetA = controlA - position;

  mUndoManager->pushCommand(
      new AddNodeCommand(mAccessor, mPathIndex, index, position, controlA, position - offsetA.normalised() * lengthB,
        NodeType::Smooth));
}

void Path::addSharpNode(int index, const Point& position)
{
  mUndoManager->pushCommand(new AddNodeCommand(mAccessor, mPathIndex, index, position, position, position,
        NodeType::Sharp));
}

Model::Path::EntryList& Path::entries(Model::Path* path)
{
  return path->mEntries;
}

}
