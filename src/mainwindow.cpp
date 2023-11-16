#include "mainwindow.h"

#include "controller/undo.h"

#include <gtkmm/gestureclick.h>

MainWindow::MainWindow(Controller::UndoManager* undoManager, View::Context& viewContext)
  : mSketchView(undoManager, viewContext)
  , mUndoManager(undoManager)
{
  set_show_menubar(true);

  set_child(mSketchView);

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
