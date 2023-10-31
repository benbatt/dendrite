#include "drawingarea.h"

#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/gesturedrag.h>
#include <iostream>

Point Point::operator+(const Vector& v) const
{
  return { x + v.x, y + v.y };
}

Vector Point::operator-(const Point& p) const
{
  return { x - p.x, y - p.y };
}

DrawingArea::DrawingArea()
  : mHoverIndex(-1)
{
  set_draw_func(sigc::mem_fun(*this, &DrawingArea::onDraw));

  auto clickController = Gtk::GestureClick::create();
  clickController->signal_pressed().connect(sigc::mem_fun(*this, &DrawingArea::onPressed));
  add_controller(clickController);

  auto motionController = Gtk::EventControllerMotion::create();
  motionController->signal_motion().connect(sigc::mem_fun(*this, &DrawingArea::onPointerMotion));
  add_controller(motionController);

  auto dragController = Gtk::GestureDrag::create();
  dragController->signal_drag_begin().connect(sigc::mem_fun(*this, &DrawingArea::onDragBegin));
  dragController->signal_drag_update().connect(sigc::mem_fun(*this, &DrawingArea::onDragUpdate));
  dragController->signal_drag_end().connect(sigc::mem_fun(*this, &DrawingArea::onDragEnd));
  add_controller(dragController);

  addNode({ 30, 20 }, { -10, -10 }, { 10, 10 });
  addNode({ 60, 30 }, { -10, -10 }, { 10, 10 });
}

const int HandleSize = 10;

void drawHandle(const Cairo::RefPtr<Cairo::Context>& context, double size, const Point& position, bool hover)
{
  float halfSize = size / 2;
  context->rectangle(position.x - halfSize, position.y - halfSize, size, size);

  context->set_source_rgb(0, 0, 0);
  context->stroke_preserve();

  if (hover)
  {
    context->set_source_rgb(1, 1, 1);
  }
  else
  {
    context->set_source_rgb(0.5, 0.5, 0.5);
  }

  context->fill();
}

void DrawingArea::onDraw(const Cairo::RefPtr<Cairo::Context>& context, int width, int height)
{
  if (mNodes.size() > 1)
  {
    context->move_to(mNodes[0].position.x, mNodes[0].position.y);

    for (int i = 1; i < mNodes.size(); ++i)
    {
      const Point& position = mNodes[i].position;
      const Point control1 = mNodes[i - 1].position + mNodes[i - 1].controlB;
      const Point control2 = position + mNodes[i].controlA;

      context->curve_to(control1.x, control1.y, control2.x, control2.y, position.x, position.y);
    }

    context->set_source_rgb(0, 0, 0);
    context->stroke();
  }

  for (int i = 0; i < mHandles.size(); ++i)
  {
    drawHandle(context, HandleSize, handlePosition(mHandles[i]), i == mHoverIndex);
  }
}

void DrawingArea::onPressed(int count, double x, double y)
{
  if (count == 2)
  {
    addNode({ x, y }, { -5, 0 }, { 5, 0 });
    queue_draw();
  }
}

void DrawingArea::onPointerMotion(double x, double y)
{
  int newHoverIndex = findHandle(x, y);

  if (newHoverIndex != mHoverIndex)
  {
    mHoverIndex = newHoverIndex;
    queue_draw();
  }
}

void DrawingArea::onDragBegin(double x, double y)
{
  mDragIndex = findHandle(x, y);

  if (mDragIndex >= 0)
  {
    mDragStart = handlePosition(mHandles[mDragIndex]);
  }
}

void DrawingArea::onDragUpdate(double xOffset, double yOffset)
{
  if (mDragIndex >= 0)
  {
    setHandlePosition(mHandles[mDragIndex], mDragStart + Vector{ xOffset, yOffset });
    queue_draw();
  }
}

void DrawingArea::onDragEnd(double xOffset, double yOffset)
{
  if (mDragIndex >= 0)
  {
    setHandlePosition(mHandles[mDragIndex], mDragStart + Vector{ xOffset, yOffset });
    queue_draw();
  }
}

void DrawingArea::addNode(const Point& position, const Vector& controlA, const Vector& controlB)
{
  int index = mNodes.size();

  mNodes.push_back({ .position = position, .controlA = controlA, .controlB = controlB });

  mHandles.push_back({ .mNodeIndex = index, .mType = Handle::Position });
  mHandles.push_back({ .mNodeIndex = index, .mType = Handle::ControlA });
  mHandles.push_back({ .mNodeIndex = index, .mType = Handle::ControlB });
}

int DrawingArea::findHandle(double x, double y)
{
  float halfSize = HandleSize / 2;

  for (int i = 0; i < mHandles.size(); ++i)
  {
    const Point position = handlePosition(mHandles[i]);

    if (position.x - halfSize <= x && x < position.x + halfSize
      && position.y - halfSize <= y && y < position.y + halfSize)
    {
      return i;
    }
  }

  return -1;
}

Point DrawingArea::handlePosition(const Handle& handle) const
{
  const Node& node = mNodes[handle.mNodeIndex];

  switch (handle.mType)
  {
    case Handle::Position:
      return node.position;
      break;
    case Handle::ControlA:
      return node.position + node.controlA;
      break;
    case Handle::ControlB:
      return node.position + node.controlB;
      break;
    default:
      return { 0, 0 };
  }
}

void DrawingArea::setHandlePosition(const Handle& handle, const Point& position)
{
  Node& node = mNodes[handle.mNodeIndex];

  switch (handle.mType)
  {
    case Handle::Position:
      node.position = position;
      break;
    case Handle::ControlA:
      node.controlA = position - node.position;
      break;
    case Handle::ControlB:
      node.controlB = position - node.position;
      break;
  }
}
