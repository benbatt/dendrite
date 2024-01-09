#pragma once

#include <giomm/simpleaction.h>

class Application;

namespace Model
{
  class Sketch;
}

namespace View
{

class Context
{
public:
  Glib::RefPtr<Gio::SimpleAction>& addAction() { return mAddAction; }
  Glib::RefPtr<Gio::SimpleAction>& deleteAction() { return mDeleteAction; }
  Glib::RefPtr<Gio::SimpleAction>& moveAction() { return mMoveAction; }
  Glib::RefPtr<Gio::SimpleAction>& cancelAction() { return mCancelAction; }
  Glib::RefPtr<Gio::SimpleAction>& viewAction() { return mViewAction; }
  Glib::RefPtr<Gio::SimpleAction>& bringForwardAction() { return mBringForwardAction; }

  sigc::signal<void(Model::Sketch*)> signalModelChanged() { return mModelChangedSignal; }

private:
  friend class ::Application;

  Glib::RefPtr<Gio::SimpleAction> mAddAction;
  Glib::RefPtr<Gio::SimpleAction> mDeleteAction;
  Glib::RefPtr<Gio::SimpleAction> mMoveAction;
  Glib::RefPtr<Gio::SimpleAction> mCancelAction;
  Glib::RefPtr<Gio::SimpleAction> mViewAction;
  Glib::RefPtr<Gio::SimpleAction> mBringForwardAction;
  sigc::signal<void(Model::Sketch*)> mModelChangedSignal;
};

}
