#include "drawingarea.h"

#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gesturedrag.h>
#include <iostream>

DrawingArea::DrawingArea()
  : mHoverIndex(-1)
{
  set_draw_func(sigc::mem_fun(*this, &DrawingArea::onDraw));

  auto motionController = Gtk::EventControllerMotion::create();
  motionController->signal_motion().connect(sigc::mem_fun(*this, &DrawingArea::onPointerMotion));
  add_controller(motionController);

  auto dragController = Gtk::GestureDrag::create();
  dragController->signal_drag_begin().connect(sigc::mem_fun(*this, &DrawingArea::onDragBegin));
  dragController->signal_drag_update().connect(sigc::mem_fun(*this, &DrawingArea::onDragUpdate));
  dragController->signal_drag_end().connect(sigc::mem_fun(*this, &DrawingArea::onDragEnd));
  add_controller(dragController);

  mHandles.push_back({ 30, 20 });
}

const int HandleSize = 10;

void DrawingArea::onDraw(const Cairo::RefPtr<Cairo::Context>& context, int width, int height)
{
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
