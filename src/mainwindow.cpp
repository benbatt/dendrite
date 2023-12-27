#include "mainwindow.h"

#include "controller/undo.h"

#include <gtkmm/gestureclick.h>

MainWindow::MainWindow(Model::Sketch* model, Controller::UndoManager* undoManager, View::Context& viewContext)
  : mMainBox(Gtk::Orientation::HORIZONTAL)
  , mToolFrame("Fill and Stroke")
  , mSketchView(model, undoManager, viewContext)
  , mUndoManager(undoManager)
{
  set_show_menubar(true);

  set_child(mMainBox);
  mMainBox.append(mSketchView);
  mMainBox.append(mToolFrame);

  mToolFrame.set_child(mToolGrid);

  mToolGrid.set_margin(10);
  mToolGrid.set_column_spacing(5);
  mToolGrid.set_row_spacing(5);

  mStrokeLabel.set_text("Stroke");
  mStrokeLabel.set_halign(Gtk::Align::END);

  mToolGrid.attach(mStrokeLabel, 0, 0);
  mToolGrid.attach(mStrokeColourButton, 1, 0);

  mFillLabel.set_text("Fill");
  mFillLabel.set_halign(Gtk::Align::END);

  mToolGrid.attach(mFillLabel, 0, 1);
  mToolGrid.attach(mFillColourButton, 1, 1);

  mStrokeColourButton.signal_color_set().connect(
      [this]() { mSketchView.setStrokeColour(mStrokeColourButton.get_rgba()); });
  mFillColourButton.signal_color_set().connect(
      [this]() { mSketchView.setFillColour(mFillColourButton.get_rgba()); });

  auto clickController = Gtk::GestureClick::create();
  clickController->signal_released().connect(sigc::mem_fun(*this, &MainWindow::onReleased));
  add_controller(clickController);
}

MainWindow::~MainWindow()
{
}

void MainWindow::onReleased(int, double, double)
{
  mUndoManager->setEnableMerge(false);
}
