#pragma once

#include "view/sketch.h"

#include <gtkmm/applicationwindow.h>

namespace Model
{
  class Sketch;
}

namespace Controller
{
  class UndoManager;
}

class MainWindow : public Gtk::ApplicationWindow
{
public:
  MainWindow(Model::Sketch* model, Controller::UndoManager* undoManager, View::Context& viewContext);
  ~MainWindow() override;

private:
  void onReleased(int count, double x, double y);

  View::Sketch mSketchView;
  Controller::UndoManager* mUndoManager;
};
