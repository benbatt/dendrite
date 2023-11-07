#include "undo.h"

namespace Controller
{

int AutoID::sNextID = UndoCommand::InvalidID + 1;

AutoID::AutoID()
  : mID(sNextID)
{
  ++sNextID;
}

UndoManager::UndoManager()
  : mEnableMerge(false)
{
}

void UndoManager::pushCommand(UndoCommand* command)
{
  command->redo();

  if (mEnableMerge && command->id() != UndoCommand::InvalidID && !mUndoCommands.empty()) {
    UndoCommand* latest = mUndoCommands.top();

    if (latest->id() == command->id() && latest->mergeWith(command)) {
      delete command;
      return;
    }
  }

  mUndoCommands.push(command);

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

}
