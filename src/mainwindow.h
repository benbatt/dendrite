#pragma once

#include "view/sketch.h"

#include <gtkmm/applicationwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/colorbutton.h>
#include <gtkmm/frame.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>

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

  Gtk::Box mMainBox;
  View::Sketch mSketchView;
  Gtk::Frame mToolFrame;
  Gtk::Grid mToolGrid;
  Gtk::Label mStrokeLabel;
  Gtk::ColorButton mStrokeColourButton;
  Gtk::Label mFillLabel;
  Gtk::ColorButton mFillColourButton;
  Controller::UndoManager* mUndoManager;
};
