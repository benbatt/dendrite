#pragma once

#include "view/sketch.h"

#include <gtkmm/applicationwindow.h>

namespace Controller
{
  class UndoManager;
}

class MainWindow : public Gtk::ApplicationWindow
{
public:
  MainWindow(Controller::UndoManager* undoManager);
  ~MainWindow() override;

private:
  void onReleased(int count, double x, double y);

  View::Sketch mSketchView;
  Controller::UndoManager* mUndoManager;
};
