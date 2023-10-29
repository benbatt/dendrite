#pragma once

#include "drawingarea.h"

#include <gtkmm/window.h>

class MainWindow : public Gtk::Window
{
public:
  MainWindow();
  ~MainWindow() override;

private:
  DrawingArea mDrawingArea;
};
