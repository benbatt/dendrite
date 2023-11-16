#pragma once

#include <giomm/simpleaction.h>

class Application;

namespace View
{

class Context
{
public:
  Glib::RefPtr<Gio::SimpleAction>& addAction() { return mAddAction; }
  Glib::RefPtr<Gio::SimpleAction>& moveAction() { return mMoveAction; }
  Glib::RefPtr<Gio::SimpleAction>& cancelAction() { return mCancelAction; }
  Glib::RefPtr<Gio::SimpleAction>& viewAction() { return mViewAction; }

private:
  friend class ::Application;

  Glib::RefPtr<Gio::SimpleAction> mAddAction;
  Glib::RefPtr<Gio::SimpleAction> mMoveAction;
  Glib::RefPtr<Gio::SimpleAction> mCancelAction;
  Glib::RefPtr<Gio::SimpleAction> mViewAction;
};

}
