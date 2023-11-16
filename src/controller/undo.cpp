#include "undo.h"

#include <list>

namespace Controller
{

int AutoID::sNextID = UndoCommand::InvalidID + 1;

AutoID::AutoID()
  : mID(sNextID)
{
  ++sNextID;
}

class UndoGroup : public UndoCommand
{
public:
  void undo() override
  {
    for (auto it = mChildren.crbegin(); it != mChildren.crend(); ++it) {
      (*it)->undo();
    }
  }

  void redo() override
  {
    for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it) {
      (*it)->redo();
    }
  }

  std::string description() override
  {
    return "Group";
  }

  std::list<UndoCommand*> mChildren;
};

UndoManager::UndoManager()
  : mCurrentGroup(nullptr)
  , mEnableMerge(false)
{
}

void UndoManager::pushCommand(UndoCommand* command)
{
  command->redo();

  if (mEnableMerge && command->id() != UndoCommand::InvalidID) {
    UndoCommand* latest = mCurrentGroup
      ? (!mCurrentGroup->mChildren.empty() ? mCurrentGroup->mChildren.back() : nullptr)
      : (!mUndoCommands.empty() ? mUndoCommands.top() : nullptr);

    if (latest && latest->id() == command->id() && latest->mergeWith(command)) {
      delete command;
      return;
    }
  }

  if (mCurrentGroup) {
    mCurrentGroup->mChildren.push_back(command);
  } else {
    mUndoCommands.push(command);
  }

  mEnableMerge = true;

  while (!mRedoCommands.empty()) {
    delete mRedoCommands.top();
    mRedoCommands.pop();
  }
}

void UndoManager::undo()
{
  if (!mUndoCommands.empty()) {
    UndoCommand* command = mUndoCommands.top();
    mUndoCommands.pop();

    command->undo();

    mRedoCommands.push(command);

    mSignalChanged.emit();
  }

  mEnableMerge = false;
}

void UndoManager::redo()
{
  if (!mRedoCommands.empty()) {
    UndoCommand* command = mRedoCommands.top();
    mRedoCommands.pop();

    command->redo();

    mUndoCommands.push(command);

    mSignalChanged.emit();
  }

  mEnableMerge = false;
}

void UndoManager::beginGroup()
{
  if (!mCurrentGroup) {
    UndoGroup *group = new UndoGroup;
    pushCommand(group);

    mCurrentGroup = group;
  }
}

void UndoManager::cancelGroup()
{
  if (mCurrentGroup) {
    mCurrentGroup->undo();
    mCurrentGroup = nullptr;

    mUndoCommands.pop();
  }
}

void UndoManager::endGroup()
{
  if (mCurrentGroup) {
    mCurrentGroup = nullptr;
  }
}

}
