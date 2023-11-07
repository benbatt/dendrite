#pragma once

#include "view/sketch.h"

#include <gtkmm/window.h>

namespace Controller
{
  class UndoManager;
}

class MainWindow : public Gtk::Window
{
public:
  MainWindow(Controller::UndoManager* undoManager);
  ~MainWindow() override;

private:
  bool onKeyPressed(guint keyval, guint keycode, Gdk::ModifierType state);
  void onReleased(int count, double x, double y);

  View::Sketch mSketchView;
  Controller::UndoManager* mUndoManager;
};
