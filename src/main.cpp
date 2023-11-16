#include "mainwindow.h"
#include "controller/undo.h"
#include "view/context.h"

#include <gtkmm/application.h>

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

  Controller::UndoManager mUndoManager;
  View::Context mViewContext;
};

Glib::RefPtr<Application> Application::create()
{
  return Glib::make_refptr_for_instance<Application>(new Application());
}

void Application::on_activate()
{
  MainWindow* mainWindow = new MainWindow(&mUndoManager, mViewContext);
  add_window(*mainWindow);

  mainWindow->signal_hide().connect([mainWindow]() { delete mainWindow; });

  mainWindow->present();
}

void Application::on_startup()
{
  Gtk::Application::on_startup();

  add_action("undo", sigc::mem_fun(*this, &Application::onUndo));
  add_action("redo", sigc::mem_fun(*this, &Application::onRedo));
  mViewContext.mAddAction = add_action("add");
  mViewContext.mMoveAction = add_action("move");
  mViewContext.mCancelAction = add_action("cancel");
  mViewContext.mViewAction = add_action("view");

  set_accel_for_action("app.undo", "<Control>Z");
  set_accel_for_action("app.redo", "<Control><Shift>Z");
  set_accel_for_action("app.add", "A");
  set_accel_for_action("app.move", "M");
  set_accel_for_action("app.cancel", "Escape");
  set_accel_for_action("app.view", "space");

  auto menuBar = Gio::Menu::create();

  auto editMenu = Gio::Menu::create();
  menuBar->append_submenu("_Edit", editMenu);

  editMenu->append("_Undo", "app.undo");
  editMenu->append("_Redo", "app.redo");

  auto actionSection = Gio::Menu::create();
  editMenu->append_section(actionSection);

  actionSection->append("_Add", "app.add");

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

int main(int argc, char* argv[])
{
  auto application = Application::create();
  return application->run(argc, argv);
}
