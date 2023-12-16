#pragma once

#include <sigc++/sigc++.h>
#include <stack>
#include <string>

namespace Controller
{

class UndoCommand
{
public:
  virtual ~UndoCommand() { }

  static const int InvalidID = 0;

  virtual void undo() = 0;
  virtual void redo() = 0;
  virtual std::string description() = 0;
  virtual int id() const { return InvalidID; }
  virtual bool mergeWith(UndoCommand* other) { return false; }
};

class AutoID
{
public:
  AutoID();

  int value() const { return mID; }

private:
  int mID;
  static int sNextID;
};

class UndoGroup;

class UndoManager
{
public:
  UndoManager();

  template <class T_Derived>
  class AutoIDCommand : public UndoCommand
  {
  public:
    int id() const override { return sID.value(); }

  private:
    static AutoID sID;
  };

  template <class T_Redo, class T_Undo>
  class LambdaCommand : public UndoCommand
  {
  public:
    LambdaCommand(const T_Redo& redo, const T_Undo& undo, const std::string& description)
      : mRedo(redo)
      , mUndo(undo)
      , mDescription(description)
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

    std::string description() override
    {
      return mDescription;
    }

  private:
    T_Redo mRedo;
    T_Undo mUndo;
    std::string mDescription;
  };

  template <class T_Redo, class T_Undo>
  static UndoCommand* createCommand(const T_Redo& redo, const T_Undo& undo, const std::string& description)
  {
    return new LambdaCommand<T_Redo, T_Undo>(redo, undo, description);
  }

  template <class T_Redo, class T_Undo>
  void pushCommand(const T_Redo& redo, const T_Undo& undo, const std::string& description)
  {
    pushCommand(createCommand(redo, undo, description));
  }

  void pushCommand(UndoCommand* command);
  void setEnableMerge(bool enable) { mEnableMerge = enable; }

  void undo();
  void redo();

  void beginGroup();
  void cancelGroup();
  void endGroup();

  void clear();

  using Signal = sigc::signal<void()>;

  Signal signalChanged() { return mSignalChanged; }

private:
  std::stack<UndoCommand*> mUndoCommands;
  std::stack<UndoCommand*> mRedoCommands;
  Signal mSignalChanged;
  std::stack<UndoGroup*> mGroups;
  bool mEnableMerge;
};

template <class T_Derived>
AutoID UndoManager::AutoIDCommand<T_Derived>::sID;

}
