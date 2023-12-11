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
  Glib::RefPtr<Gio::SimpleAction>& moveAction() { return mMoveAction; }
  Glib::RefPtr<Gio::SimpleAction>& cancelAction() { return mCancelAction; }
  Glib::RefPtr<Gio::SimpleAction>& viewAction() { return mViewAction; }

  sigc::signal<void(Model::Sketch*)> signalModelChanged() { return mModelChangedSignal; }

private:
  friend class ::Application;

  Glib::RefPtr<Gio::SimpleAction> mAddAction;
  Glib::RefPtr<Gio::SimpleAction> mMoveAction;
  Glib::RefPtr<Gio::SimpleAction> mCancelAction;
  Glib::RefPtr<Gio::SimpleAction> mViewAction;
  sigc::signal<void(Model::Sketch*)> mModelChangedSignal;
};

}
