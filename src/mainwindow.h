#pragma once

#include <wx/wx.h>

namespace Model
{
  class Sketch;
}

namespace Controller
{
  class UndoManager;
}

namespace View
{
  class Context;
}

class MainWindow : public wxFrame
{
public:
  MainWindow(Model::Sketch* model, Controller::UndoManager* undoManager, View::Context& viewContext);
  ~MainWindow() override;

private:
  Controller::UndoManager* mUndoManager;
};
