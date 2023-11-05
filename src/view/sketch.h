#pragma once

#include <gtkmm/drawingarea.h>

#include "controller/sketch.h"
#include "model/sketch.h"
#include "utilities/geometry.h"

namespace View
{

class Sketch : public Gtk::DrawingArea
{
public:
  Sketch();

private:
  void onDraw(const Cairo::RefPtr<Cairo::Context>& context, int width, int height);
  void onPressed(int count, double x, double y);
  void onPointerMotion(double x, double y);
  void onDragBegin(double x, double y);
  void onDragUpdate(double x, double y);
  void onDragEnd(double x, double y);

  void addNode(const Point& position, const Vector& controlA, const Vector& controlB);
  int findHandle(double x, double y);

  struct Handle
  {
    int mNodeIndex;
    Controller::Node::HandleType mType;
  };

  Point handlePosition(const Handle& handle) const;
  void setHandlePosition(const Handle& handle, const Point& position);

  Model::Sketch* mModel;
  Controller::Sketch* mController;
  std::vector<Handle> mHandles;

  int mHoverIndex;
  int mDragIndex;
  Point mDragStart;
};

}
