#include "sketch.h"

#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gestureclick.h>
#include <gtkmm/gesturedrag.h>

namespace View
{

Sketch::Sketch()
  : mHoverIndex(-1)
{
  set_draw_func(sigc::mem_fun(*this, &Sketch::onDraw));

  auto clickController = Gtk::GestureClick::create();
  clickController->signal_pressed().connect(sigc::mem_fun(*this, &Sketch::onPressed));
  add_controller(clickController);

  auto motionController = Gtk::EventControllerMotion::create();
  motionController->signal_motion().connect(sigc::mem_fun(*this, &Sketch::onPointerMotion));
  add_controller(motionController);

  auto dragController = Gtk::GestureDrag::create();
  dragController->signal_drag_begin().connect(sigc::mem_fun(*this, &Sketch::onDragBegin));
  dragController->signal_drag_update().connect(sigc::mem_fun(*this, &Sketch::onDragUpdate));
  dragController->signal_drag_end().connect(sigc::mem_fun(*this, &Sketch::onDragEnd));
  add_controller(dragController);

  mModel = new Model::Sketch;
  mController = new Controller::Sketch(mModel);

  addNode({ 30, 20 }, { -10, -10 }, { 10, 10 });
  addNode({ 60, 30 }, { -10, -10 }, { 10, 10 });
}

const int HandleSize = 10;

void drawHandle(const Cairo::RefPtr<Cairo::Context>& context, double size, const Point& position, bool hover)
{
  float halfSize = size / 2;
  context->rectangle(position.x - halfSize, position.y - halfSize, size, size);

  context->set_source_rgb(0, 0, 0);
  context->set_line_width(2);
  context->stroke_preserve();

  if (hover) {
    context->set_source_rgb(1, 1, 1);
  } else {
    context->set_source_rgb(0.5, 0.5, 0.5);
  }

  context->fill();
}

void Sketch::onDraw(const Cairo::RefPtr<Cairo::Context>& context, int width, int height)
{
  const Model::Sketch::NodeList& nodes = mModel->nodes();

  if (nodes.size() > 1) {
    context->move_to(nodes[0].position().x, nodes[0].position().y);

    for (int i = 1; i < nodes.size(); ++i) {
      const Point& position = nodes[i].position();
      const Point control1 = nodes[i - 1].position() + nodes[i - 1].controlB();
      const Point control2 = position + nodes[i].controlA();

      context->curve_to(control1.x, control1.y, control2.x, control2.y, position.x, position.y);
    }

    context->set_source_rgb(0, 0, 0);
    context->set_line_width(2);
    context->stroke();
  }

  for (const Model::Node& n : nodes) {
    context->move_to(n.position().x, n.position().y);
    context->rel_line_to(n.controlA().x, n.controlA().y);
    context->move_to(n.position().x, n.position().y);
    context->rel_line_to(n.controlB().x, n.controlB().y);
  }

  context->set_source_rgb(0, 0, 0);
  context->set_line_width(0.5);
  context->stroke();

  for (int i = 0; i < mHandles.size(); ++i) {
    drawHandle(context, HandleSize, handlePosition(mHandles[i]), i == mHoverIndex);
  }
}

void Sketch::onPressed(int count, double x, double y)
{
  if (count == 2) {
    addNode({ x, y }, { -10, 0 }, { 10, 0 });
    queue_draw();
  }
}

void Sketch::onPointerMotion(double x, double y)
{
  int newHoverIndex = findHandle(x, y);

  if (newHoverIndex != mHoverIndex) {
    mHoverIndex = newHoverIndex;
    queue_draw();
  }
}

void Sketch::onDragBegin(double x, double y)
{
  mDragIndex = findHandle(x, y);

  if (mDragIndex >= 0) {
    mDragStart = handlePosition(mHandles[mDragIndex]);
  }
}

void Sketch::onDragUpdate(double xOffset, double yOffset)
{
  if (mDragIndex >= 0) {
    setHandlePosition(mHandles[mDragIndex], mDragStart + Vector{ xOffset, yOffset });
    queue_draw();
  }
}

void Sketch::onDragEnd(double xOffset, double yOffset)
{
  if (mDragIndex >= 0) {
    setHandlePosition(mHandles[mDragIndex], mDragStart + Vector{ xOffset, yOffset });
    queue_draw();
  }
}

void Sketch::addNode(const Point& position, const Vector& controlA, const Vector& controlB)
{
  int index = mModel->nodes().size();

  mController->addNode(position, controlA, controlB);

  mHandles.push_back({ .mNodeIndex = index, .mType = Controller::Node::Position });
  mHandles.push_back({ .mNodeIndex = index, .mType = Controller::Node::ControlA });
  mHandles.push_back({ .mNodeIndex = index, .mType = Controller::Node::ControlB });
}

int Sketch::findHandle(double x, double y)
{
  float halfSize = HandleSize / 2;

  for (int i = 0; i < mHandles.size(); ++i) {
    const Point position = handlePosition(mHandles[i]);

    if (position.x - halfSize <= x && x < position.x + halfSize
      && position.y - halfSize <= y && y < position.y + halfSize) {
      return i;
    }
  }

  return -1;
}

Point Sketch::handlePosition(const Handle& handle) const
{
  return mController->controllerForNode(handle.mNodeIndex).handlePosition(handle.mType);
}

void Sketch::setHandlePosition(const Handle& handle, const Point& position)
{
  mController->controllerForNode(handle.mNodeIndex).setHandlePosition(handle.mType, position);
}

}
