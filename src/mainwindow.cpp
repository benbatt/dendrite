#include "mainwindow.h"

#include "controller/undo.h"

#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/gestureclick.h>

MainWindow::MainWindow(Controller::UndoManager* undoManager)
  : mSketchView(undoManager)
  , mUndoManager(undoManager)
{
  set_child(mSketchView);

  auto keyController = Gtk::EventControllerKey::create();
  keyController->signal_key_pressed().connect(sigc::mem_fun(*this, &MainWindow::onKeyPressed), true);
  add_controller(keyController);

  auto clickController = Gtk::GestureClick::create();
  clickController->signal_released().connect(sigc::mem_fun(*this, &MainWindow::onReleased));
  add_controller(clickController);
}

MainWindow::~MainWindow()
{
}

bool MainWindow::onKeyPressed(guint keyval, guint keycode, Gdk::ModifierType state)
{
  using Gdk::ModifierType;

  switch (keyval) {
    case GDK_KEY_Z:
    case GDK_KEY_z:
      if ((state & ModifierType::CONTROL_MASK) == ModifierType::CONTROL_MASK) {
        if ((state & ModifierType::SHIFT_MASK) == ModifierType::SHIFT_MASK) {
          mUndoManager->redo();
        } else {
          mUndoManager->undo();
        }

        return true;
      }
  }

  return false;
}

void MainWindow::onReleased(int, double, double)
{
  mUndoManager->setEnableMerge(false);
}
