#pragma once

#include "view/sketch.h"

#include <gtkmm/window.h>

class MainWindow : public Gtk::Window
{
public:
  MainWindow();
  ~MainWindow() override;

private:
  View::Sketch mSketchView;
};
