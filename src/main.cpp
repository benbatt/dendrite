#include "mainwindow.h"
#include "controller/undo.h"

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
};

Glib::RefPtr<Application> Application::create()
{
  return Glib::make_refptr_for_instance<Application>(new Application());
}

void Application::on_activate()
{
  MainWindow* mainWindow = new MainWindow(&mUndoManager);
  add_window(*mainWindow);

  mainWindow->signal_hide().connect([mainWindow]() { delete mainWindow; });

  mainWindow->present();
}

void Application::on_startup()
{
  Gtk::Application::on_startup();

  add_action("undo", sigc::mem_fun(*this, &Application::onUndo));
  add_action("redo", sigc::mem_fun(*this, &Application::onRedo));

  set_accel_for_action("app.undo", "<Control>Z");
  set_accel_for_action("app.redo", "<Control><Shift>Z");

  auto menuBar = Gio::Menu::create();

  auto editMenu = Gio::Menu::create();
  menuBar->append_submenu("_Edit", editMenu);

  editMenu->append("_Undo", "app.undo");
  editMenu->append("_Redo", "app.redo");

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
