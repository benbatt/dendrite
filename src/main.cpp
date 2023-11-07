#include "mainwindow.h"
#include "controller/undo.h"

#include <gtkmm/application.h>

int main(int argc, char* argv[])
{
  Controller::UndoManager undoManager;

  auto app = Gtk::Application::create("com.benbatt.spline_draw");
  return app->make_window_and_run<MainWindow>(argc, argv, &undoManager);
}
