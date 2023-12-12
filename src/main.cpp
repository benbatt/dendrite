#include "mainwindow.h"
#include "controller/sketch.h"
#include "controller/undo.h"
#include "model/sketch.h"
#include "serialisation/layout.h"
#include "serialisation/reader.h"
#include "serialisation/writer.h"
#include "view/context.h"

#include <fstream>
#include <gtkmm/application.h>
#include <gtkmm/filedialog.h>

#include <iostream>

class Application : public Gtk::Application
{
public:
  static Glib::RefPtr<Application> create();

protected:
  void on_activate() override;
  void on_startup() override;

private:
  void onUndo();
  void onRedo();
  void onOpen();
  void onSave();

  void addWindow(Model::Sketch* model);

  Controller::UndoManager mUndoManager;
  View::Context mViewContext;
  Model::Sketch* mModel;
};

Glib::RefPtr<Application> Application::create()
{
  return Glib::make_refptr_for_instance<Application>(new Application());
}

void Application::on_activate()
{
  mModel = new Model::Sketch;

  {
    auto controller = Controller::Sketch(&mUndoManager, mModel);
    auto pathController = controller.controllerForPath(controller.addPath());

    pathController.addSymmetricNode(0, { 30, 20 }, { 20, 10 });
    pathController.addSymmetricNode(1, { 60, 30 }, { 50, 20 });
  }

  addWindow(mModel);
}

void Application::addWindow(Model::Sketch* model)
{
  MainWindow* mainWindow = new MainWindow(model, &mUndoManager, mViewContext);
  add_window(*mainWindow);

  mainWindow->signal_hide().connect([mainWindow]() { delete mainWindow; });

  mainWindow->present();
}

void Application::on_startup()
{
  Gtk::Application::on_startup();

  add_action("undo", sigc::mem_fun(*this, &Application::onUndo));
  add_action("redo", sigc::mem_fun(*this, &Application::onRedo));
  add_action("save", sigc::mem_fun(*this, &Application::onSave));
  add_action("open", sigc::mem_fun(*this, &Application::onOpen));
  mViewContext.mAddAction = add_action("add");
  mViewContext.mMoveAction = add_action("move");
  mViewContext.mCancelAction = add_action("cancel");
  mViewContext.mViewAction = add_action("view");

  set_accel_for_action("app.undo", "<Control>Z");
  set_accel_for_action("app.redo", "<Control><Shift>Z");
  set_accel_for_action("app.save", "<Control>S");
  set_accel_for_action("app.open", "<Control>O");
  set_accel_for_action("app.add", "A");
  set_accel_for_action("app.move", "M");
  set_accel_for_action("app.cancel", "Escape");
  set_accel_for_action("app.view", "space");

  auto menuBar = Gio::Menu::create();

  auto fileMenu = Gio::Menu::create();
  menuBar->append_submenu("_File", fileMenu);

  fileMenu->append("_Open", "app.open");
  fileMenu->append("_Save", "app.save");

  auto editMenu = Gio::Menu::create();
  menuBar->append_submenu("_Edit", editMenu);

  editMenu->append("_Undo", "app.undo");
  editMenu->append("_Redo", "app.redo");

  auto actionSection = Gio::Menu::create();
  editMenu->append_section(actionSection);

  actionSection->append("_Add", "app.add");
  actionSection->append("_Move", "app.move");
  actionSection->append("_Cancel", "app.cancel");
  actionSection->append("_View", "app.view");

  set_menubar(menuBar);
}

void Application::onUndo()
{
  mUndoManager.undo();
}

void Application::onRedo()
{
  mUndoManager.redo();
}

void Application::onSave()
{
  auto fileDialog = Gtk::FileDialog::create();

  fileDialog->save([this, fileDialog](Glib::RefPtr<Gio::AsyncResult>& result) {
      auto file = fileDialog->save_finish(result);
      std::cout << "Saving to " << file->get_path() << std::endl;
      std::ofstream stream(file->get_path(), std::ios_base::binary);

      Serialisation::Writer writer(stream);
      Serialisation::Layout::process(writer, mModel);
    });
}

void Application::onOpen()
{
  auto fileDialog = Gtk::FileDialog::create();

  fileDialog->open([this, fileDialog](Glib::RefPtr<Gio::AsyncResult>& result) {
      auto file = fileDialog->open_finish(result);
      std::cout << "Opening " << file->get_path() << std::endl;
      std::ifstream stream(file->get_path(), std::ios_base::binary);

      Serialisation::Reader reader(stream);
      Model::Sketch* newModel = Serialisation::Layout::process(reader, nullptr);

      mUndoManager.clear();

      delete mModel;
      mModel = newModel;

      mViewContext.mModelChangedSignal.emit(mModel);
    });
}

int main(int argc, char* argv[])
{
  auto application = Application::create();
  return application->run(argc, argv);
}
