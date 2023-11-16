#include "sketch.h"

#include "controller/undo.h"
#include "view/context.h"

#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gesturedrag.h>

namespace View
{

Sketch::Sketch(Controller::UndoManager *undoManager, Context& context)
  : mUndoManager(undoManager)
  , mMode(Mode::Move)
  , mHoverIndex(-1)
{
  set_draw_func(sigc::mem_fun(*this, &Sketch::onDraw));

  context.addAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateAddMode));
  context.moveAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateMoveMode));
  context.viewAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateViewMode));
  context.cancelAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::onCancel));

  mClickController = Gtk::GestureClick::create();
  mClickController->signal_pressed().connect(sigc::mem_fun(*this, &Sketch::onPointerPressed));
  mClickController->signal_released().connect(sigc::mem_fun(*this, &Sketch::onPointerReleased));
  add_controller(mClickController);

  auto motionController = Gtk::EventControllerMotion::create();
  motionController->signal_motion().connect(sigc::mem_fun(*this, &Sketch::onPointerMotion));
  add_controller(motionController);

  mModel = new Model::Sketch;
  mController = new Controller::Sketch(undoManager, mModel);

  undoManager->signalChanged().connect(sigc::mem_fun(*this, &Sketch::onUndoChanged));

  addNode(0, { 30, 20 }, { -10, -10 }, { 10, 10 });
  addNode(1, { 60, 30 }, { -10, -10 }, { 10, 10 });
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

void drawAddHandle(const Cairo::RefPtr<Cairo::Context>& context, double size, const Point& position, bool hover)
{
  float halfSize = size / 2;

  context->move_to(position.x - halfSize, position.y);
  context->line_to(position.x + halfSize, position.y);

  context->move_to(position.x, position.y - halfSize);
  context->line_to(position.x, position.y + halfSize);

  if (hover) {
    context->set_source_rgb(1, 1, 1);
  } else {
    context->set_source_rgb(0, 0, 0);
  }

  context->set_line_width(2);
  context->stroke();
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

  if (mMode != Mode::View) {
    for (const Model::Node& n : nodes) {
      context->move_to(n.position().x, n.position().y);
      context->rel_line_to(n.controlA().x, n.controlA().y);
      context->move_to(n.position().x, n.position().y);
      context->rel_line_to(n.controlB().x, n.controlB().y);
    }

    context->set_source_rgb(0, 0, 0);
    context->set_line_width(0.5);
    context->stroke();
  }

  if (mMode == Mode::Move) {
    for (int i = 0; i < mHandles.size(); ++i) {
      drawHandle(context, HandleSize, handlePosition(mHandles[i]), i == mHoverIndex);
    }
  } else if (mMode == Mode::Add) {
    for (int i = 0; i < mHandles.size(); ++i) {
      if ((mHandles[i].mNodeIndex == 0 && mHandles[i].mType == Controller::Node::ControlA)
          || (mHandles[i].mNodeIndex == mModel->nodes().size() - 1 && mHandles[i].mType == Controller::Node::ControlB)) {
        drawAddHandle(context, HandleSize, handlePosition(mHandles[i]), i == mHoverIndex);
      }
    }
  }
}

void Sketch::onPointerPressed(int count, double x, double y)
{
  using Gdk::ModifierType;

  if (mMode == Mode::Move) {
    if (mHoverIndex < 0) {
      return;
    }

    mUndoManager->beginGroup();

    mDragIndex = mHoverIndex;
    mMode = Mode::Move_Place;
  } else if (mMode == Mode::Add) {
    if (mHoverIndex < 0) {
      return;
    }

    auto addNode = [this, x, y](int index) {
      mUndoManager->beginGroup();

      this->addNode(index, { x, y }, { 0, 0 }, { 0, 0 });

      mDragIndex = findHandleForNode(index, Controller::Node::Position);
      mMode = Mode::Add_Place;
      queue_draw();
    };

    const Handle& handle = mHandles[mHoverIndex];

    if (handle.mNodeIndex == 0 && handle.mType == Controller::Node::ControlA) {
      addNode(0);
    } else if (handle.mNodeIndex == mModel->nodes().size() - 1 && handle.mType == Controller::Node::ControlB) {
      addNode(mModel->nodes().size());
    }
  } else if (mMode == Mode::Move_Place) {
    mUndoManager->endGroup();
    mMode = Mode::Move;
    queue_draw();
  } else if (mMode == Mode::Add_Place) {
    const Handle& handle = mHandles[mDragIndex];

    if (handle.mNodeIndex == 0) {
      mDragIndex = findHandleForNode(handle.mNodeIndex, Controller::Node::ControlA);
    } else if (handle.mNodeIndex == mModel->nodes().size() - 1) {
      mDragIndex = findHandleForNode(handle.mNodeIndex, Controller::Node::ControlB);
    }

    mMode = Mode::Add_Adjust;
    queue_draw();
  }
}

void Sketch::onPointerReleased(int count, double x, double y)
{
  if (mMode == Mode::Add_Adjust) {
    mUndoManager->endGroup();
    mMode = Mode::Add;
    queue_draw();
  }
}

void Sketch::onPointerMotion(double x, double y)
{
  if (mMode == Mode::Move_Place || mMode == Mode::Add_Place || mMode == Mode::Add_Adjust) {
    if (mDragIndex >= 0) {
      if (mMode == Mode::Add_Adjust) {
        setHandlePosition(mHandles[mDragIndex], { x, y }, Controller::Node::SetPositionMode::Symmetrical);
      } else {
        setHandlePosition(mHandles[mDragIndex], { x, y }, Controller::Node::SetPositionMode::Smooth);
      }

      queue_draw();
    }
  } else {
    int newHoverIndex = findHandle(x, y);

    if (newHoverIndex != mHoverIndex) {
      mHoverIndex = newHoverIndex;
      queue_draw();
    }
  }
}

void Sketch::onUndoChanged()
{
  mHandles.clear();

  for (int i = 0; i < mModel->nodes().size(); ++i) {
    addHandles(i);
  }

  queue_draw();
}

void Sketch::activateAddMode(const Glib::VariantBase&)
{
  setMode(Mode::Add);
}

void Sketch::activateMoveMode(const Glib::VariantBase&)
{
  setMode(Mode::Move);
}

void Sketch::activateViewMode(const Glib::VariantBase&)
{
  setMode(Mode::View);
}

void Sketch::onCancel(const Glib::VariantBase&)
{
  if (mMode == Mode::Add_Place || mMode == Mode::Add_Adjust) {
    setMode(Mode::Add);
  } else if (mMode == Mode::Move_Place) {
    setMode(Mode::Move);
  }
}

void Sketch::addNode(int index, const Point& position, const Vector& controlA, const Vector& controlB)
{
  mController->addNode(index, position, controlA, controlB);
  addHandles(index);
}

void Sketch::addHandles(int nodeIndex)
{
  mHandles.push_back({ .mNodeIndex = nodeIndex, .mType = Controller::Node::Position });
  mHandles.push_back({ .mNodeIndex = nodeIndex, .mType = Controller::Node::ControlA });
  mHandles.push_back({ .mNodeIndex = nodeIndex, .mType = Controller::Node::ControlB });
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

int Sketch::findHandleForNode(int nodeIndex, Controller::Node::HandleType type)
{
  for (int i = 0; i < mHandles.size(); ++i) {
    if (mHandles[i].mNodeIndex == nodeIndex && mHandles[i].mType == type) {
      return i;
    }
  }

  return -1;
}

Point Sketch::handlePosition(const Handle& handle) const
{
  return mController->controllerForNode(handle.mNodeIndex).handlePosition(handle.mType);
}

void Sketch::setHandlePosition(const Handle& handle, const Point& position, Controller::Node::SetPositionMode mode)
{
  mController->controllerForNode(handle.mNodeIndex).setHandlePosition(handle.mType, position, mode);
}

void Sketch::setMode(Mode mode)
{
  if (mMode != mode) {
    if (mMode == Mode::Add_Place || mMode == Mode::Add_Adjust || mMode == Mode::Move_Place) {
      mUndoManager->cancelGroup();
      onUndoChanged();
    }

    mMode = mode;
    queue_draw();
  }
}

}
