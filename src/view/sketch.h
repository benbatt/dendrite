#pragma once

#include <gtkmm/drawingarea.h>
#include <gtkmm/gestureclick.h>

#include "controller/sketch.h"
#include "model/sketch.h"
#include "utilities/geometry.h"

namespace Controller
{
  class UndoManager;
}

namespace View
{

class Context;

class Sketch : public Gtk::DrawingArea
{
public:
  Sketch(Controller::UndoManager* undoManager, Context& context);

private:
  friend class SketchModeMove;
  friend class SketchModeAdd;
  friend class SketchModePlace;

  void onDraw(const Cairo::RefPtr<Cairo::Context>& context, int width, int height);
  void onPointerPressed(int count, double x, double y);
  void onSecondaryPointerPressed(int count, double x, double y);
  void onPointerMotion(double x, double y);
  bool onKeyPressed(guint keyval, guint keycode, Gdk::ModifierType state);
  void refreshHandles();
  void activateAddMode(const Glib::VariantBase&);
  void activateMoveMode(const Glib::VariantBase&);
  void activateViewMode(const Glib::VariantBase&);
  void onCancel(const Glib::VariantBase&);

  void addHandles(int nodeIndex);
  int findHandle(double x, double y);
  int findHandleForNode(int nodeIndex, Controller::Node::HandleType type);

  struct Handle
  {
    int mNodeIndex;
    Controller::Node::HandleType mType;
  };

  Point handlePosition(const Handle& handle) const;
  void setHandlePosition(const Handle& handle, const Point& position);

  class Mode
  {
  public:
    virtual void draw(Sketch& sketch, const Cairo::RefPtr<Cairo::Context>& context, int width, int height) { }
    virtual void onPointerPressed(Sketch& sketch, int count, double x, double y) { }
    virtual void onPointerReleased(Sketch& sketch, int count, double x, double y) { }
    virtual bool onPointerMotion(Sketch& sketch, double x, double y) { return false; }
    virtual bool onKeyPressed(Sketch& sketch, guint keyval, guint keycode, Gdk::ModifierType state) { return false; }
    virtual void onCancel(Sketch& sketch) { }
    virtual void onChildPopped(Sketch& sketch, Mode* child) { }
  };

  void pushMode(Mode* mode);
  void popMode(Mode* mode);
  void cancelModeStack();

  Model::Sketch* mModel;
  Controller::Sketch* mController;
  Controller::UndoManager* mUndoManager;
  std::list<Mode*> mModeStack;
  std::vector<Handle> mHandles;

  int mHoverIndex;
  int mDragIndex;
};

}
