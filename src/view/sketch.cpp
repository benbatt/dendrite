#include "sketch.h"

#include "controller/undo.h"
#include "view/context.h"

#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/eventcontrollermotion.h>
#include <gtkmm/gesturedrag.h>

namespace View
{

enum class HandleStyle
{
  Symmetric,
  Smooth,
  Sharp,
  Control,
};

using NodeType = Model::Node::Type;
using HandleType = Controller::Node::HandleType;

HandleStyle handleStyle(NodeType nodeType, HandleType handleType)
{
  if (handleType == HandleType::Position) {
    switch (nodeType) {
      case NodeType::Symmetric:
        return HandleStyle::Symmetric;
      case NodeType::Smooth:
        return HandleStyle::Smooth;
      case NodeType::Sharp:
        return HandleStyle::Sharp;
    }
  }

  return HandleStyle::Control;
}

const float HandleSize = 10;

void drawHandle(const Cairo::RefPtr<Cairo::Context>& context, HandleStyle style, const Point& position, bool hover)
{
  const float HalfSize = HandleSize / 2;

  switch (style) {
    case HandleStyle::Symmetric:
      context->arc(position.x, position.y, HalfSize, 0, 2 * M_PI);
      context->close_path();
      break;
    case HandleStyle::Smooth:
      context->rectangle(position.x - HalfSize, position.y - HalfSize, HandleSize, HandleSize);
      break;
    case HandleStyle::Sharp:
      context->move_to(position.x - HalfSize, position.y);
      context->line_to(position.x, position.y - HalfSize);
      context->line_to(position.x + HalfSize, position.y);
      context->line_to(position.x, position.y + HalfSize);
      context->close_path();
      break;
    case HandleStyle::Control:
      context->arc(position.x, position.y, HalfSize / 2, 0, 2 * M_PI);
      context->close_path();
      break;
  }

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

  SketchModePlace()
    : mConstrainDirection(false)
  { }

  void begin(Sketch& sketch) override
  {
    sketch.set_cursor("none");
    setDirectionConstraint(sketch);
  }

  void end(Sketch& sketch) override
  {
    sketch.set_cursor("");
  }

  void onPointerPressed(Sketch& sketch, int count, double x, double y) override
  {
    sketch.popMode(this);
  }

  bool onPointerMotion(Sketch& sketch, double x, double y) override
  {
    if (sketch.mDragIndex >= 0) {
      setHandlePosition(sketch, sketch.mHandles[sketch.mDragIndex], x, y);
    }

    return true;
  }

  bool onKeyPressed(Sketch& sketch, guint keyval, guint keycode, Gdk::ModifierType state) override
  {
    if (keyval == GDK_KEY_Control_L || keyval == GDK_KEY_Control_R) {
      const Sketch::Handle& handle = sketch.mHandles[sketch.mDragIndex];
      const Model::Node& node = sketch.mModel->nodes()[handle.mNodeIndex];

      Controller::Node controller = sketch.mController->controllerForNode(handle.mNodeIndex);

      switch (node.type()) {
        case NodeType::Symmetric:
          controller.setType(NodeType::Smooth);
          break;
        case NodeType::Smooth:
          controller.setType(NodeType::Sharp);
          break;
        case NodeType::Sharp:
          controller.setType(NodeType::Symmetric);
          break;
      }

      if (handle.mType != HandleType::Position) {
        controller.setHandlePosition(handle.mType, sketch.handlePosition(handle));
      }

      sketch.queue_draw();

      return true;
    } else if (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R) {
      mConstrainDirection = !mConstrainDirection;

      if (mConstrainDirection && sketch.mDragIndex >= 0) {
        const Sketch::Handle& handle = sketch.mHandles[sketch.mDragIndex];
        Point position = sketch.handlePosition(handle);

        setHandlePosition(sketch, handle, position.x, position.y);
      }

      return true;
    }

    return false;
  }

  void onCancel(Sketch& sketch) override
  {
    sketch.mUndoManager->cancelGroup();
    sketch.refreshHandles();
  }

private:
  void setDirectionConstraint(Sketch& sketch)
  {
    if (sketch.mDragIndex >= 0) {
      const Sketch::Handle& handle = sketch.mHandles[sketch.mDragIndex];

      if (handle.mType != HandleType::Position) {
        const Model::Node& node = sketch.mModel->nodes()[handle.mNodeIndex];

        if (handle.mType == HandleType::ControlA) {
          mDirectionConstraint = node.controlA().normalised();
        } else if (handle.mType == HandleType::ControlB) {
          mDirectionConstraint = node.controlB().normalised();
        }
      }
    }
  }

  void setHandlePosition(Sketch& sketch, const Sketch::Handle& handle, double x, double y)
  {
    Point newPosition{x, y};

    if (mConstrainDirection && handle.mType != HandleType::Position) {
      const Model::Node& node = sketch.mModel->nodes()[handle.mNodeIndex];

      Vector offset = newPosition - node.position();

      double length = std::max(0.0, offset.dot(mDirectionConstraint));

      newPosition = node.position() + mDirectionConstraint * length;
    }

    sketch.setHandlePosition(handle, newPosition);
    sketch.queue_draw();
  }

  Vector mDirectionConstraint;
  bool mConstrainDirection;
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
      const Sketch::Handle& handle = handles[i];
      const Model::Node& node = sketch.mModel->nodes()[handle.mNodeIndex];
      bool hover = (i == sketch.mHoverIndex);

      drawHandle(context, handleStyle(node.type(), handle.mType), sketch.handlePosition(handle), hover);
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

  void onChildPopped(Sketch& sketch, Sketch::Mode* child) override
  {
    if (child == &SketchModePlace::sInstance) {
      sketch.mUndoManager->endGroup();
    }
  }
};

SketchModeMove SketchModeMove::sInstance;

class SketchModeAdd : public Sketch::Mode
{
public:
  static SketchModeAdd sInstance;

  void draw(Sketch& sketch, const Cairo::RefPtr<Cairo::Context>& context, int width, int height) override
  {
    const std::vector<Sketch::Handle>& handles = sketch.mHandles;

    for (int i = 0; i < handles.size(); ++i) {
      if ((handles[i].mNodeIndex == 0 && handles[i].mType == Controller::Node::ControlA)
          || (handles[i].mNodeIndex == sketch.mModel->nodes().size() - 1 && handles[i].mType == Controller::Node::ControlB)) {
        drawAddHandle(context, 10, sketch.handlePosition(handles[i]), i == sketch.mHoverIndex);
      }
    }
  }

  void onPointerPressed(Sketch& sketch, int count, double x, double y) override
  {
    if (sketch.mHoverIndex >= 0) {
      addNode(sketch, sketch.mHoverIndex);
    }
  }

  void onChildPopped(Sketch& sketch, Sketch::Mode* child) override
  {
    if (child == &mSetPositionMode) {
      const Sketch::Handle& handle = sketch.mHandles[sketch.mDragIndex];

      if (handle.mNodeIndex == 0) {
        sketch.mDragIndex = sketch.findHandleForNode(handle.mNodeIndex, Controller::Node::ControlA);
      } else if (handle.mNodeIndex == sketch.mModel->nodes().size() - 1) {
        sketch.mDragIndex = sketch.findHandleForNode(handle.mNodeIndex, Controller::Node::ControlB);
      }

      sketch.pushMode(&mAdjustHandlesMode);
    } else if (child == &mAdjustHandlesMode) {
      sketch.mUndoManager->endGroup();

      addNode(sketch, sketch.mDragIndex);
    }
  }

private:
  void addNode(Sketch& sketch, int handleIndex)
  {
    int addIndex = -1;

    const Sketch::Handle& handle = sketch.mHandles[handleIndex];

    if (handle.mNodeIndex == 0 && handle.mType == Controller::Node::ControlA) {
      addIndex = 0;
    } else if (handle.mNodeIndex == sketch.mModel->nodes().size() - 1 && handle.mType == Controller::Node::ControlB) {
      addIndex = sketch.mModel->nodes().size();
    } else {
      return;
    }

    sketch.mUndoManager->beginGroup();

    sketch.mController->addSymmetricNode(addIndex, sketch.handlePosition(handle), { 0, 0 });
    sketch.addHandles(addIndex);

    sketch.mDragIndex = sketch.findHandleForNode(addIndex, Controller::Node::Position);
    sketch.pushMode(&mSetPositionMode);
  };

  SketchModePlace mSetPositionMode;
  SketchModePlace mAdjustHandlesMode;
};

SketchModeAdd SketchModeAdd::sInstance;

Sketch::Sketch(Controller::UndoManager *undoManager, Context& context)
  : mUndoManager(undoManager)
  , mHoverIndex(-1)
  , mDragIndex(-1)
{
  set_focusable(true);

  set_draw_func(sigc::mem_fun(*this, &Sketch::onDraw));

  context.addAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateAddMode));
  context.moveAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateMoveMode));
  context.viewAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateViewMode));
  context.cancelAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::onCancel));

  auto clickController = Gtk::GestureClick::create();
  clickController->signal_pressed().connect(sigc::mem_fun(*this, &Sketch::onPointerPressed));
  add_controller(clickController);

  auto secondaryClickController = Gtk::GestureClick::create();
  secondaryClickController->set_button(GDK_BUTTON_SECONDARY);
  secondaryClickController->signal_pressed().connect(sigc::mem_fun(*this, &Sketch::onSecondaryPointerPressed));
  add_controller(secondaryClickController);

  auto motionController = Gtk::EventControllerMotion::create();
  motionController->signal_motion().connect(sigc::mem_fun(*this, &Sketch::onPointerMotion));
  add_controller(motionController);

  auto keyController = Gtk::EventControllerKey::create();
  keyController->signal_key_pressed().connect(sigc::mem_fun(*this, &Sketch::onKeyPressed), false);
  add_controller(keyController);

  mModel = new Model::Sketch;
  mController = new Controller::Sketch(undoManager, mModel);

  undoManager->signalChanged().connect(sigc::mem_fun(*this, &Sketch::refreshHandles));

  mController->addSymmetricNode(0, { 30, 20 }, { -10, -10 });
  mController->addSymmetricNode(1, { 60, 30 }, { -10, -10 });

  refreshHandles();
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

void Sketch::onSecondaryPointerPressed(int count, double x, double y)
{
  onCancel(Glib::VariantBase());
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

bool Sketch::onKeyPressed(guint keyval, guint keycode, Gdk::ModifierType state)
{
  if (!mModeStack.empty()) {
    return mModeStack.front()->onKeyPressed(*this, keyval, keycode, state);
  }

  return false;
}

void Sketch::refreshHandles()
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
    mModeStack.front()->end(*this);
    mModeStack.pop_front();

    queue_draw();
  }
}

void Sketch::addHandles(int nodeIndex)
{
  mHandles.push_back({ .mNodeIndex = nodeIndex, .mType = Controller::Node::Position });
  mHandles.push_back({ .mNodeIndex = nodeIndex, .mType = Controller::Node::ControlA });
  mHandles.push_back({ .mNodeIndex = nodeIndex, .mType = Controller::Node::ControlB });
}

int Sketch::findHandle(double x, double y)
{
  const float Radius = HandleSize / 2;

  for (int i = mHandles.size() - 1; i >= 0; --i) {
    const Point position = handlePosition(mHandles[i]);

    if (position.x - Radius <= x && x < position.x + Radius
      && position.y - Radius <= y && y < position.y + Radius) {
      return i;
    }
  }

  return -1;
}

int Sketch::findHandleForNode(int nodeIndex, HandleType type)
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

void Sketch::setHandlePosition(const Handle& handle, const Point& position)
{
  mController->controllerForNode(handle.mNodeIndex).setHandlePosition(handle.mType, position);
}

void Sketch::pushMode(Mode* mode)
{
  mModeStack.push_front(mode);
  mode->begin(*this);
  queue_draw();
}

void Sketch::popMode(Mode* mode)
{
  if (std::find(mModeStack.begin(), mModeStack.end(), mode) != mModeStack.end()) {
    while (!mModeStack.empty()) {
      mModeStack.front()->end(*this);

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
    mModeStack.front()->end(*this);
    mModeStack.pop_front();
  }

  queue_draw();
}

}
