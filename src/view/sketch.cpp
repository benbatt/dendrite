#include "sketch.h"

#include "controller/undo.h"
#include "view/context.h"

#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gesturedrag.h>

namespace View
{

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

class SketchModePlace : public Sketch::Mode
{
public:
  static SketchModePlace sInstance;

  SketchModePlace(Controller::Node::SetPositionMode setPositionMode = Controller::Node::SetPositionMode::Smooth)
    : mSetPositionMode(setPositionMode)
  {
  }

  void draw(Sketch& sketch, const Cairo::RefPtr<Cairo::Context>& context, int width, int height) override
  {
  }

  void onPointerPressed(Sketch& sketch, int count, double x, double y) override
  {
    sketch.mUndoManager->endGroup();
    sketch.popMode(this);
  }

  bool onPointerMotion(Sketch& sketch, double x, double y) override
  {
    int dragIndex = sketch.mDragIndex;

    if (dragIndex >= 0) {
      sketch.setHandlePosition(sketch.mHandles[dragIndex], { x, y }, mSetPositionMode);
      sketch.queue_draw();
    }

    return true;
  }

  void onCancel(Sketch& sketch) override
  {
    sketch.mUndoManager->cancelGroup();
    sketch.onUndoChanged();
  }

private:
  Controller::Node::SetPositionMode mSetPositionMode;
};

SketchModePlace SketchModePlace::sInstance;

class SketchModeMove : public Sketch::Mode
{
public:
  static SketchModeMove sInstance;

  void draw(Sketch& sketch, const Cairo::RefPtr<Cairo::Context>& context, int width, int height) override
  {
    const std::vector<Sketch::Handle>& handles = sketch.mHandles;

    for (int i = 0; i < handles.size(); ++i) {
      drawHandle(context, HandleSize, sketch.handlePosition(handles[i]), i == sketch.mHoverIndex);
    }
  }

  void onPointerPressed(Sketch& sketch, int count, double x, double y) override
  {
    if (sketch.mHoverIndex < 0) {
      return;
    }

    sketch.mUndoManager->beginGroup();

    sketch.mDragIndex = sketch.mHoverIndex;
    sketch.pushMode(&SketchModePlace::sInstance);
  }
};

SketchModeMove SketchModeMove::sInstance;

class SketchModeAdd : public Sketch::Mode
{
public:
  static SketchModeAdd sInstance;

  SketchModeAdd()
    : mAdjustHandlesMode(Controller::Node::SetPositionMode::Symmetrical)
  {
  }

  void draw(Sketch& sketch, const Cairo::RefPtr<Cairo::Context>& context, int width, int height) override
  {
    const std::vector<Sketch::Handle>& handles = sketch.mHandles;

    for (int i = 0; i < handles.size(); ++i) {
      if ((handles[i].mNodeIndex == 0 && handles[i].mType == Controller::Node::ControlA)
          || (handles[i].mNodeIndex == sketch.mModel->nodes().size() - 1 && handles[i].mType == Controller::Node::ControlB)) {
        drawAddHandle(context, HandleSize, sketch.handlePosition(handles[i]), i == sketch.mHoverIndex);
      }
    }
  }

  void onPointerPressed(Sketch& sketch, int count, double x, double y) override
  {
    if (sketch.mHoverIndex < 0) {
      return;
    }

    auto addNode = [this, &sketch, x, y](int index) {
      sketch.mUndoManager->beginGroup();

      sketch.addNode(index, { x, y }, { 0, 0 }, { 0, 0 });

      sketch.mDragIndex = sketch.findHandleForNode(index, Controller::Node::Position);
      sketch.pushMode(&mSetPositionMode);
    };

    const Sketch::Handle& handle = sketch.mHandles[sketch.mHoverIndex];

    if (handle.mNodeIndex == 0 && handle.mType == Controller::Node::ControlA) {
      addNode(0);
    } else if (handle.mNodeIndex == sketch.mModel->nodes().size() - 1 && handle.mType == Controller::Node::ControlB) {
      addNode(sketch.mModel->nodes().size());
    }
  }

  void onChildPopped(Sketch& sketch, Sketch::Mode* child)
  {
    if (child == &mSetPositionMode) {
      const Sketch::Handle& handle = sketch.mHandles[sketch.mDragIndex];

      if (handle.mNodeIndex == 0) {
        sketch.mDragIndex = sketch.findHandleForNode(handle.mNodeIndex, Controller::Node::ControlA);
      } else if (handle.mNodeIndex == sketch.mModel->nodes().size() - 1) {
        sketch.mDragIndex = sketch.findHandleForNode(handle.mNodeIndex, Controller::Node::ControlB);
      }

      sketch.pushMode(&mAdjustHandlesMode);
    }
  }

private:
  SketchModePlace mSetPositionMode;
  SketchModePlace mAdjustHandlesMode;
};

SketchModeAdd SketchModeAdd::sInstance;

Sketch::Sketch(Controller::UndoManager *undoManager, Context& context)
  : mUndoManager(undoManager)
  , mHoverIndex(-1)
{
  set_draw_func(sigc::mem_fun(*this, &Sketch::onDraw));

  context.addAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateAddMode));
  context.moveAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateMoveMode));
  context.viewAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateViewMode));
  context.cancelAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::onCancel));

  mClickController = Gtk::GestureClick::create();
  mClickController->signal_pressed().connect(sigc::mem_fun(*this, &Sketch::onPointerPressed));
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

  if (!mModeStack.empty()) {
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

  for (auto it = mModeStack.rbegin(); it != mModeStack.rend(); ++it) {
    (*it)->draw(*this, context, width, height);
  }
}

void Sketch::onPointerPressed(int count, double x, double y)
{
  if (!mModeStack.empty()) {
    mModeStack.front()->onPointerPressed(*this, count, x, y);
  }
}

void Sketch::onPointerMotion(double x, double y)
{
  bool consumed = false;

  if (!mModeStack.empty()) {
    consumed = mModeStack.front()->onPointerMotion(*this, x, y);
  }

  if (!consumed) {
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
  cancelModeStack();
  pushMode(&SketchModeAdd::sInstance);
}

void Sketch::activateMoveMode(const Glib::VariantBase&)
{
  cancelModeStack();
  pushMode(&SketchModeMove::sInstance);
}

void Sketch::activateViewMode(const Glib::VariantBase&)
{
  cancelModeStack();
}

void Sketch::onCancel(const Glib::VariantBase&)
{
  if (!mModeStack.empty()) {
    mModeStack.front()->onCancel(*this);
    popMode(mModeStack.front());
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

void Sketch::pushMode(Mode* mode)
{
  mModeStack.push_front(mode);
  queue_draw();
}

void Sketch::popMode(Mode* mode)
{
  if (std::find(mModeStack.begin(), mModeStack.end(), mode) != mModeStack.end()) {
    while (!mModeStack.empty()) {
      Mode* oldFront = mModeStack.front();
      mModeStack.pop_front();

      if (!mModeStack.empty()) {
        mModeStack.front()->onChildPopped(*this, oldFront);
      }

      if (oldFront == mode) {
        break;
      }
    }

    queue_draw();
  }
}

void Sketch::cancelModeStack()
{
  while (!mModeStack.empty()) {
    mModeStack.front()->onCancel(*this);
    mModeStack.pop_front();
  }

  queue_draw();
}

}
