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

class AddEntryCommand : public UndoCommand
{
public:
  AddEntryCommand(Path::Accessor* accessor, const ID<Model::Path>& id, int addIndex, const Model::Path::Entry& entry)
    : mAccessor(accessor)
    , mEntry(entry)
    , mID(id)
    , mAddIndex(addIndex)
  {}

  void redo() override
  { 
    Model::Path* model = mAccessor->getPath(mID);
    Model::Path::EntryList& entries = Path::entries(model);

    if (mAddIndex < entries.size()) {
      entries.insert(std::next(entries.begin(), mAddIndex), mEntry);
    } else {
      entries.push_back(mEntry);
    }
  }

  void undo() override
  { 
    Model::Path* model = mAccessor->getPath(mID);
    Model::Path::EntryList& entries = Path::entries(model);

    entries.erase(std::next(entries.begin(), mAddIndex));
  }

  std::string description() override
  {
    return "Add node";
  }

private:
  Path::Accessor* mAccessor;
  Model::Path::Entry mEntry;
  ID<Model::Path> mID;
  int mAddIndex;
};

void Path::addEntry(int index, const Model::Path::Entry& entry)
{
  mUndoManager->pushCommand(new AddEntryCommand(mAccessor, mID, index, entry));
}

class RemoveEntryCommand : public UndoCommand
{
public:
  RemoveEntryCommand(Path::Accessor* accessor, const ID<Model::Path>& id, int removeIndex)
    : mAccessor(accessor)
    , mID(id)
    , mRemoveIndex(removeIndex)
  {
    mEntry = Path::entries(mAccessor->getPath(mID))[mRemoveIndex];
  }

  void redo() override
  { 
    Model::Path* model = mAccessor->getPath(mID);
    Model::Path::EntryList& entries = Path::entries(model);

    entries.erase(std::next(entries.begin(), mRemoveIndex));
  }

  void undo() override
  { 
    Model::Path* model = mAccessor->getPath(mID);
    Model::Path::EntryList& entries = Path::entries(model);

    entries.insert(std::next(entries.begin(), mRemoveIndex), mEntry);
  }

  std::string description() override
  {
    return "Remove path entry";
  }

private:
  Path::Accessor* mAccessor;
  Model::Path::Entry mEntry;
  ID<Model::Path> mID;
  int mRemoveIndex;
};

void Path::removeEntry(int index)
{
  mUndoManager->pushCommand(new RemoveEntryCommand(mAccessor, mID, index));
}

class SetTranslationCommand : public UndoManager::AutoIDCommand<SetTranslationCommand>
{
public:
  SetTranslationCommand(Path::Accessor* pathAccessor, const ID<Model::Path>& id, const Vector& translation)
    : mAccessor(pathAccessor)
    , mID(id)
    , mTranslation(translation)
    , mOldTranslation(pathAccessor->getPath(id)->translation())
  {
  }

  void redo() override
  { 
    Model::Path* path = mAccessor->getPath(mID);
    Path::translation(path) = mTranslation;
  }

  void undo() override
  { 
    Model::Path* path = mAccessor->getPath(mID);
    Path::translation(path) = mOldTranslation;
  }

  std::string description() override
  {
    return "Move path";
  }

  bool mergeWith(UndoCommand* other) override
  {
    SetTranslationCommand* command = static_cast<SetTranslationCommand*>(other);

    if (command->mAccessor == mAccessor && command->mID == mID) {
      mTranslation = command->mTranslation;
      return true;
    } else {
      return false;
    }
  }

private:
  Path::Accessor* mAccessor;
  ID<Model::Path> mID;
  Vector mTranslation;
  Vector mOldTranslation;
};

void Path::setTranslation(const Vector& translation)
{
  mUndoManager->pushCommand(new SetTranslationCommand(mAccessor, mID, translation));
}

void Path::setStrokeColour(const Colour& colour)
{
  auto accessor = mAccessor;
  auto id = mID;
  auto oldColour = accessor->getPath(id)->mStrokeColour;

  mUndoManager->pushCommand(
      [=]() { accessor->getPath(id)->mStrokeColour = colour; },
      [=]() { accessor->getPath(id)->mStrokeColour = oldColour; },
      "Set stroke colour");
}

void Path::setFillColour(const Colour& colour)
{
  auto accessor = mAccessor;
  auto id = mID;
  auto oldColour = accessor->getPath(id)->mFillColour;
  auto oldFlags = accessor->getPath(id)->mFlags;

  mUndoManager->pushCommand(
      [=]() {
        Model::Path* path = accessor->getPath(id);
        path->mFillColour = colour;
        path->mFlags |= Model::Path::Flag_Filled;
      },
      [=]() {
        Model::Path* path = accessor->getPath(id);
        path->mFillColour = oldColour;
        path->mFlags = oldFlags;
      },
      "Set fill colour");
}

void Path::setClosed(bool closed)
{
  Model::Path* path = mAccessor->getPath(mID);

  const unsigned int Flag_Closed = Model::Path::Flag_Closed;

  unsigned int oldFlags = path->mFlags;
  unsigned int newFlags = closed ? (path->mFlags | Flag_Closed) : (path->mFlags & ~Flag_Closed);
  auto accessor = mAccessor;
  auto id = mID;

  mUndoManager->pushCommand(
      [newFlags, accessor, id]() { accessor->getPath(id)->mFlags = newFlags; },
      [oldFlags, accessor, id]() { accessor->getPath(id)->mFlags = oldFlags; },
      "Close path");
}

Model::Path::EntryList& Path::entries(Model::Path* path)
{
  return path->mEntries;
}

Vector& Path::translation(Model::Path* path)
{
  return path->mTranslation;
}

}
