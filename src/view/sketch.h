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
  void onDraw(const Cairo::RefPtr<Cairo::Context>& context, int width, int height);
  void onPointerPressed(int count, double x, double y);
  void onPointerReleased(int count, double x, double y);
  void onPointerMotion(double x, double y);
  void onDragBegin(double x, double y);
  void onDragUpdate(double x, double y);
  void onDragEnd(double x, double y);
  void onUndoChanged();
  void activateAddMode(const Glib::VariantBase&);
  void activateMoveMode(const Glib::VariantBase&);
  void activateViewMode(const Glib::VariantBase&);
  void onCancel(const Glib::VariantBase&);

  void addNode(int index, const Point& position, const Vector& controlA, const Vector& controlB);
  void addHandles(int nodeIndex);
  int findHandle(double x, double y);
  int findHandleForNode(int nodeIndex, Controller::Node::HandleType type);

  struct Handle
  {
    int mNodeIndex;
    Controller::Node::HandleType mType;
  };

  Point handlePosition(const Handle& handle) const;
  void setHandlePosition(const Handle& handle, const Point& position, Controller::Node::SetPositionMode mode);

  enum class Mode
  {
    View,
    Move,
    Add,
    Move_Place,
    Add_Place,
    Add_Adjust,
  };

  void setMode(Mode mode);

  Model::Sketch* mModel;
  Controller::Sketch* mController;
  Controller::UndoManager* mUndoManager;
  std::vector<Handle> mHandles;
  Glib::RefPtr<Gtk::GestureClick> mClickController;

  Mode mMode;
  int mHoverIndex;
  int mDragIndex;
  Point mDragStart;
};

}
