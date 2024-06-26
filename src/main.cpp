#include "mainwindow.h"
#include "controller/sketch.h"
#include "controller/undo.h"
#include "model/document.h"
#include "serialisation/layout.h"
#include "serialisation/reader.h"
#include "serialisation/writer.h"
#include "view/context.h"

#include <fstream>
#include <wx/wx.h>

#include <iostream>

class Application : public wxApp
{
public:
  bool OnInit() override;

private:
  enum ID {
    Open,
    Save,
    Undo,
    Redo,
    Add,
    Delete,
    Group,
    Move,
    Cancel,
    View,
    BringForward,
    SendBackward,
  };

  void onUndo(wxCommandEvent& event);
  void onRedo(wxCommandEvent& event);
  void onOpen(wxCommandEvent& event);
  void onSave(wxCommandEvent& event);

  Controller::UndoManager mUndoManager;
  View::Context mViewContext;
  wxMenuBar* mMenuBar;
  Model::Document* mDocument;
  MainWindow* mMainWindow;
};

bool Application::OnInit()
{
  Bind(wxEVT_MENU, &Application::onUndo, this, ID::Undo);
  Bind(wxEVT_MENU, &Application::onRedo, this, ID::Redo);
  Bind(wxEVT_MENU, &Application::onSave, this, ID::Save);
  Bind(wxEVT_MENU, &Application::onOpen, this, ID::Open);

  Bind(wxEVT_MENU, [this](wxCommandEvent&) { mViewContext.mAddSignal.emit(); }, ID::Add);
  Bind(wxEVT_MENU, [this](wxCommandEvent&) { mViewContext.mDeleteSignal.emit(); }, ID::Delete);
  Bind(wxEVT_MENU, [this](wxCommandEvent&) { mViewContext.mGroupSignal.emit(); }, ID::Group);
  Bind(wxEVT_MENU, [this](wxCommandEvent&) { mViewContext.mMoveSignal.emit(); }, ID::Move);
  Bind(wxEVT_MENU, [this](wxCommandEvent&) { mViewContext.mCancelSignal.emit(); }, ID::Cancel);
  Bind(wxEVT_MENU, [this](wxCommandEvent&) { mViewContext.mViewSignal.emit(); }, ID::View);
  Bind(wxEVT_MENU, [this](wxCommandEvent&) { mViewContext.mBringForwardSignal.emit(); }, ID::BringForward);
  Bind(wxEVT_MENU, [this](wxCommandEvent&) { mViewContext.mSendBackwardSignal.emit(); }, ID::SendBackward);

  mMenuBar = new wxMenuBar;

  wxMenu* fileMenu = new wxMenu;
  mMenuBar->Append(fileMenu, "&File");

  fileMenu->Append(ID::Open, "&Open\tCtrl-O");
  fileMenu->Append(ID::Save, "&Save\tCtrl-S");

  wxMenu* editMenu = new wxMenu;
  mMenuBar->Append(editMenu, "&Edit");

  editMenu->Append(ID::Undo, "&Undo\tCtrl-Z");
  editMenu->Append(ID::Redo, "&Redo\tCtrl-Shift-Z");

  editMenu->AppendSeparator();

  editMenu->Append(ID::Add, "&Add\tA");
  editMenu->Append(ID::Delete, "&Delete\tD");
  editMenu->Append(ID::Group, "&Group\tG");
  editMenu->Append(ID::Move, "&Move\tM");
  editMenu->Append(ID::Cancel, "&Cancel\tEscape");
  editMenu->Append(ID::View, "&View\tSpace");

  editMenu->AppendSeparator();

  editMenu->Append(ID::BringForward, "Bring &Forward\tPageUp");
  editMenu->Append(ID::SendBackward, "Send &Backward\tPageDown");

  mDocument = new Model::Document;

  mMainWindow = new MainWindow(mDocument->sketch(), &mUndoManager, mViewContext);
  mMainWindow->SetMenuBar(mMenuBar);
  mMainWindow->Maximize(true);
  mMainWindow->Show();

  return true;
}

void Application::onUndo(wxCommandEvent& event)
{
  mUndoManager.undo();
}

void Application::onRedo(wxCommandEvent& event)
{
  mUndoManager.redo();
}

void Application::onSave(wxCommandEvent& event)
{
  wxFileDialog dialog(mMainWindow, "Open", "", "", "", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

  if (dialog.ShowModal() == wxID_CANCEL) {
    return;
  }

  std::cout << "Saving to " << dialog.GetPath() << std::endl;
  std::ofstream stream(dialog.GetPath(), std::ios_base::binary);

  Serialisation::Writer writer(stream);
  Serialisation::Layout::process(writer, mDocument);
}

void Application::onOpen(wxCommandEvent& event)
{
  wxFileDialog dialog(mMainWindow, "Open", "", "", "", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

  if (dialog.ShowModal() == wxID_CANCEL) {
    return;
  }

  std::cout << "Opening " << dialog.GetPath() << std::endl;
  std::ifstream stream(dialog.GetPath(), std::ios_base::binary);

  Serialisation::Reader reader(stream);
  Model::Document* newDocument = Serialisation::Layout::process(reader, nullptr);

  mUndoManager.clear();

  delete mDocument;
  mDocument = newDocument;

  mViewContext.mModelChangedSignal.emit(mDocument->sketch());
}

wxIMPLEMENT_APP(Application);
