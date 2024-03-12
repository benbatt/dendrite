#include "mainwindow.h"

#include "controller/undo.h"

#include "view/sketch.h"

#include <wx/clrpicker.h>

MainWindow::MainWindow(Model::Sketch* model, Controller::UndoManager* undoManager, View::Context& viewContext)
  : wxFrame(nullptr, wxID_ANY, "SplineDraw")
  , mUndoManager(undoManager)
{
  wxBoxSizer* mainBox = new wxBoxSizer(wxHORIZONTAL);

  View::Sketch* sketchView = new View::Sketch(this, model, undoManager, viewContext);
  mainBox->Add(sketchView, wxSizerFlags(1).Expand());

  wxFlexGridSizer* toolSizer = new wxFlexGridSizer(2, 5, 5);
  mainBox->Add(toolSizer, wxSizerFlags().Border());

  wxColourPickerCtrl* strokeColourPicker = new wxColourPickerCtrl(this, wxID_ANY);
  toolSizer->Add(new wxStaticText(this, wxID_ANY, "Stroke"), wxSizerFlags().CentreVertical().Right());
  toolSizer->Add(strokeColourPicker);

  wxColourPickerCtrl* fillColourPicker = new wxColourPickerCtrl(this, wxID_ANY);
  toolSizer->Add(new wxStaticText(this, wxID_ANY, "Fill"), wxSizerFlags().CentreVertical().Right());
  toolSizer->Add(fillColourPicker);

  strokeColourPicker->Bind(wxEVT_COLOURPICKER_CHANGED,
    [sketchView](wxColourPickerEvent& event) { sketchView->setStrokeColour(event.GetColour()); });
  fillColourPicker->Bind(wxEVT_COLOURPICKER_CHANGED,
    [sketchView](wxColourPickerEvent& event) { sketchView->setFillColour(event.GetColour()); });

  SetSizerAndFit(mainBox);

  CreateStatusBar();
}

MainWindow::~MainWindow()
{
}
