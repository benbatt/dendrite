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
  : mEnableMerge(false)
{
}

void clearStack(std::stack<UndoCommand*>& stack)
{
  while (!stack.empty()) {
    delete stack.top();
    stack.pop();
  }
}

void UndoManager::pushCommand(UndoCommand* command)
{
  command->redo();

  UndoGroup* currentGroup = !mGroups.empty() ? mGroups.top() : nullptr;

  if (mEnableMerge && command->id() != UndoCommand::InvalidID) {
    UndoCommand* latest = currentGroup
      ? (!currentGroup->mChildren.empty() ? currentGroup->mChildren.back() : nullptr)
      : (!mUndoCommands.empty() ? mUndoCommands.top() : nullptr);

    if (latest && latest->id() == command->id() && latest->mergeWith(command)) {
      delete command;
      return;
    }
  }

  if (currentGroup) {
    currentGroup->mChildren.push_back(command);
  } else {
    mUndoCommands.push(command);
  }

  mEnableMerge = true;

  clearStack(mRedoCommands);
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
  UndoGroup *group = new UndoGroup;
  pushCommand(group);

  mGroups.push(group);
}

void UndoManager::cancelGroup()
{
  if (!mGroups.empty()) {
    mGroups.top()->undo();
    mGroups.pop();
    mUndoCommands.pop();
  }
}

void UndoManager::endGroup()
{
  if (!mGroups.empty()) {
    mGroups.pop();
  }
}

void UndoManager::clear()
{
  cancelGroup();

  clearStack(mUndoCommands);
  clearStack(mRedoCommands);
}

}
