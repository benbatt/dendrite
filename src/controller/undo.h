#pragma once

#include <sigc++/sigc++.h>
#include <stack>

namespace Controller
{

class UndoCommand
{
public:
  static const int InvalidID = 0;

  virtual void undo() = 0;
  virtual void redo() = 0;
  virtual int id() const { return InvalidID; }
  virtual bool mergeWith(UndoCommand* other) { return false; }
};

class UndoManager
{
public:
  UndoManager();

  template <class T_Derived>
  class AutoIDCommand : public UndoCommand
  {
  public:
    AutoIDCommand()
    {
      if (sID == InvalidID) {
        sID = UndoManager::sNextAutoID;
        ++UndoManager::sNextAutoID;
      }
    }

    int id() const override { return sID; }

  private:
    static int sID;
  };

  template <class T_Redo, class T_Undo>
  class LambdaCommand : public UndoCommand
  {
  public:
    LambdaCommand(const T_Redo& redo, const T_Undo& undo)
      : mRedo(redo)
      , mUndo(undo)
    {
    }

    void undo() override
    {
      mUndo();
    }

    void redo() override
    {
      mRedo();
    }

  private:
    T_Redo mRedo;
    T_Undo mUndo;
  };

  template <class T_Redo, class T_Undo>
  static UndoCommand* createCommand(const T_Redo& redo, const T_Undo& undo)
  {
    return new LambdaCommand<T_Redo, T_Undo>(redo, undo);
  }

  template <class T_Redo, class T_Undo>
  void pushCommand(const T_Redo& redo, const T_Undo& undo)
  {
    pushCommand(createCommand(redo, undo));
  }

  void pushCommand(UndoCommand* command);
  void setEnableMerge(bool enable) { mEnableMerge = enable; }

  void undo();
  void redo();

  using Signal = sigc::signal<void()>;

  Signal signalChanged() { return mSignalChanged; }

private:
  static int sNextAutoID;

  std::stack<UndoCommand*> mUndoCommands;
  std::stack<UndoCommand*> mRedoCommands;
  Signal mSignalChanged;
  bool mEnableMerge;
};

template <class T_Derived>
int UndoManager::AutoIDCommand<T_Derived>::sID = UndoCommand::InvalidID;

}
