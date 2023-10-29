#include "drawingarea.h"

#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/gesturedrag.h>
#include <iostream>

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

  mHandles.push_back({ 30, 20 });
  mHandles.push_back({ 60, 30 });
}

const int HandleSize = 10;

void DrawingArea::onDraw(const Cairo::RefPtr<Cairo::Context>& context, int width, int height)
{
  if (mHandles.size() > 1)
  {
    context->move_to(mHandles[0].x, mHandles[0].y);

    for (int i = 1; i < mHandles.size(); ++i)
    {
      context->line_to(mHandles[i].x, mHandles[i].y);
    }

    context->set_source_rgb(0, 0, 0);
    context->stroke();
  }

  float halfSize = HandleSize / 2;

  for (int i = 0; i < mHandles.size(); ++i)
  {
    const Handle& h = mHandles[i];

    context->rectangle(h.x - halfSize, h.y - halfSize, HandleSize, HandleSize);

    context->set_source_rgb(0, 0, 0);
    context->stroke_preserve();

    if (i == mHoverIndex)
    {
      context->set_source_rgb(1, 1, 1);
    }
    else
    {
      context->set_source_rgb(0.5, 0.5, 0.5);
    }

    context->fill();
  }
}

void DrawingArea::onPressed(int count, double x, double y)
{
  if (count == 2)
  {
    mHandles.push_back({ x, y });
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
    Handle& h = mHandles[mDragIndex];
    mDragStartX = h.x;
    mDragStartY = h.y;
  }
}

void DrawingArea::onDragUpdate(double xOffset, double yOffset)
{
  if (mDragIndex >= 0)
  {
    Handle& h = mHandles[mDragIndex];
    h.x = mDragStartX + xOffset;
    h.y = mDragStartY + yOffset;

    queue_draw();
  }
}

void DrawingArea::onDragEnd(double xOffset, double yOffset)
{
  if (mDragIndex >= 0)
  {
    Handle& h = mHandles[mDragIndex];
    h.x = mDragStartX + xOffset;
    h.y = mDragStartY + yOffset;

    queue_draw();
  }
}

int DrawingArea::findHandle(double x, double y)
{
  float halfSize = HandleSize / 2;

  for (int i = 0; i < mHandles.size(); ++i)
  {
    const Handle& h = mHandles[i];

    if (h.x - halfSize <= x && x < h.x + halfSize
      && h.y - halfSize <= y && y < h.y + halfSize)
    {
      return i;
    }
  }

  return -1;
}
