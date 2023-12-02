#include "sketch.h"

#include "controller/undo.h"
#include "model/controlpoint.h"
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
using Handle = Sketch::Handle;

HandleStyle handleStyle(NodeType nodeType, Handle::Type handleType)
{
  if (handleType == Handle::Node) {
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

  const Handle& dragHandle() const { return mDragHandle; }
  void setDragHandle(const Handle& handle)
  {
    mDragHandle = handle;
  }

  void begin(Sketch& sketch) override
  {
    sketch.set_cursor("none");
    setDirectionConstraint(sketch);
  }

  void end(Sketch& sketch) override
  {
    sketch.set_cursor("");
  }

  void draw(Sketch& sketch, const Cairo::RefPtr<Cairo::Context>& context, int width, int height) override
  {
    if (mConstrainDirection && mDragHandle.refersTo(Handle::ControlPoint)) {
      const Model::ControlPoint* controlPoint = sketch.mModel->controlPoints()[mDragHandle.mIndex];

      Point start = controlPoint->node()->position();
      Point end = start + mDirectionConstraint * std::max(width, height);

      context->move_to(start.x, start.y);
      context->line_to(end.x, end.y);

      context->set_line_width(1);
      context->set_source_rgb(1, 0, 1);

      context->stroke();
    }
  }

  void onPointerPressed(Sketch& sketch, int count, double x, double y) override
  {
    sketch.popMode(this);
  }

  bool onPointerMotion(Sketch& sketch, double x, double y) override
  {
    if (mDragHandle) {
      setHandlePosition(sketch, mDragHandle, x, y);
    }

    return true;
  }

  bool onKeyPressed(Sketch& sketch, guint keyval, guint keycode, Gdk::ModifierType state) override
  {
    if (keyval == GDK_KEY_Control_L || keyval == GDK_KEY_Control_R) {
      if (mDragHandle) {
        const Model::Node* node = sketch.nodeForHandle(mDragHandle);

        Controller::Node controller = sketch.mController->controllerForNode(node);

        switch (node->type()) {
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

        if (mDragHandle.refersTo(Handle::ControlPoint)) {
          const Model::ControlPoint* controlPoint = sketch.mModel->controlPoints()[mDragHandle.mIndex];
          sketch.mController->controllerForControlPoint(mDragHandle.mIndex).setPosition(controlPoint->position());
        }

        sketch.queue_draw();

        return true;
      }
    } else if (keyval == GDK_KEY_Shift_L || keyval == GDK_KEY_Shift_R) {
      mConstrainDirection = !mConstrainDirection;

      if (mConstrainDirection && mDragHandle) {
        Point position = sketch.handlePosition(mDragHandle);
        setHandlePosition(sketch, mDragHandle, position.x, position.y);
      }

      return true;
    }

    return false;
  }

  void onCancel(Sketch& sketch) override
  {
    sketch.mUndoManager->cancelGroup();
  }

private:
  void setDirectionConstraint(Sketch& sketch)
  {
    if (mDragHandle.refersTo(Handle::ControlPoint)) {
      const Model::ControlPoint* controlPoint = sketch.mModel->controlPoints()[mDragHandle.mIndex];
      mDirectionConstraint = (controlPoint->position() - controlPoint->node()->position()).normalised();
    }
  }

  void setHandlePosition(Sketch& sketch, const Handle& handle, double x, double y)
  {
    Point newPosition{x, y};

    if (mConstrainDirection && handle.refersTo(Handle::ControlPoint)) {
      const Model::ControlPoint* controlPoint = sketch.mModel->controlPoints()[handle.mIndex];
      const Model::Node* node = controlPoint->node();

      Vector offset = newPosition - node->position();

      double length = std::max(0.0, offset.dot(mDirectionConstraint));

      newPosition = node->position() + mDirectionConstraint * length;
    }

    sketch.setHandlePosition(handle, newPosition);
    sketch.queue_draw();
  }

  Vector mDirectionConstraint;
  bool mConstrainDirection;
  Handle mDragHandle;
};

SketchModePlace SketchModePlace::sInstance;

class SketchModeMove : public Sketch::Mode
{
public:
  static SketchModeMove sInstance;

  void draw(Sketch& sketch, const Cairo::RefPtr<Cairo::Context>& context, int width, int height) override
  {
    const Model::Sketch::NodeList& nodes = sketch.mModel->nodes();

    for (int i = 0; i < nodes.size(); ++i) {
      const Model::Node* node = nodes[i];
      bool hover = sketch.mHoverHandle == Handle{Handle::Node, i};

      drawHandle(context, handleStyle(node->type(), Handle::Node), node->position(), hover);
    }

    const Model::Sketch::ControlPointList& controlPoints = sketch.mModel->controlPoints();

    for (int i = 0; i < controlPoints.size(); ++i) {
      const Model::ControlPoint* controlPoint = controlPoints[i];
      bool hover = sketch.mHoverHandle == Handle{Handle::ControlPoint, i};

      drawHandle(context, handleStyle(NodeType::Sharp, Handle::ControlPoint), controlPoint->position(), hover);
    }
  }

  void onPointerPressed(Sketch& sketch, int count, double x, double y) override
  {
    if (sketch.mHoverHandle) {
      sketch.mUndoManager->beginGroup();

      mPlaceMode.setDragHandle(sketch.mHoverHandle);
      sketch.pushMode(&mPlaceMode);
    }
  }

  void onChildPopped(Sketch& sketch, Sketch::Mode* child) override
  {
    if (child == &mPlaceMode) {
      sketch.mUndoManager->endGroup();
    }
  }

  SketchModePlace mPlaceMode;
};

SketchModeMove SketchModeMove::sInstance;

class SketchModeAdd : public Sketch::Mode
{
public:
  static SketchModeAdd sInstance;

  void draw(Sketch& sketch, const Cairo::RefPtr<Cairo::Context>& context, int width, int height) override
  {
    const Model::Path* path = sketch.mModel->paths().front();
    const Model::Path::EntryList& entries = path->entries();

    if (!entries.empty()) {
      drawAddHandle(context, 10, entries.front().mPreControl->position(), false);
      drawAddHandle(context, 10, entries.back().mPostControl->position(), false);
    }

    if (sketch.activeMode() == &mAdjustHandlesMode) {
      const Sketch::Handle& handle = mAdjustHandlesMode.dragHandle();

      if (handle.refersTo(Handle::ControlPoint)) {
        const Model::Node* node = sketch.mModel->controlPoints()[handle.mIndex]->node();
        drawHandle(context, handleStyle(node->type(), Handle::Node), node->position(), true);
      }
    }
  }

  void onPointerPressed(Sketch& sketch, int count, double x, double y) override
  {
    if (sketch.mHoverHandle) {
      addNode(sketch, sketch.mHoverHandle);
    }
  }

  void onChildPopped(Sketch& sketch, Sketch::Mode* child) override
  {
    if (child == &mSetPositionMode) {
      const Model::Node* node = sketch.mModel->nodes()[mSetPositionMode.dragHandle().mIndex];

      const Model::Path::EntryList& entries = sketch.mModel->paths().front()->entries();

      if (node == entries.front().mNode) {
        mAdjustHandlesMode.setDragHandle(sketch.findHandle(entries.front().mPreControl));
      } else if (node == entries.back().mNode) {
        mAdjustHandlesMode.setDragHandle(sketch.findHandle(entries.back().mPostControl));
      }

      sketch.pushMode(&mAdjustHandlesMode);
    } else if (child == &mAdjustHandlesMode) {
      sketch.mUndoManager->endGroup();

      addNode(sketch, mAdjustHandlesMode.dragHandle());
    }
  }

private:
  void addNode(Sketch& sketch, const Handle& handle)
  {
    if (!handle.refersTo(Handle::ControlPoint)) {
      return;
    }

    const Model::Path::EntryList& entries = sketch.mModel->paths().front()->entries();

    if (entries.empty()) {
      return;
    }

    const Model::ControlPoint* controlPoint = sketch.mModel->controlPoints()[handle.mIndex];
    int addIndex = -1;

    if (controlPoint == entries.front().mPreControl) {
      addIndex = 0;
    } else if (controlPoint == entries.back().mPostControl) {
      addIndex = entries.size();
    } else {
      return;
    }

    sketch.mUndoManager->beginGroup();

    sketch.mController->controllerForPath(0)
      .addSymmetricNode(addIndex, controlPoint->position(), controlPoint->position());

    mSetPositionMode.setDragHandle(sketch.findHandle(entries[addIndex].mNode));
    sketch.pushMode(&mSetPositionMode);
  };

  SketchModePlace mSetPositionMode;
  SketchModePlace mAdjustHandlesMode;
};

SketchModeAdd SketchModeAdd::sInstance;

Sketch::Sketch(Controller::UndoManager *undoManager, Context& context)
  : mUndoManager(undoManager)
  , mHoverHandle{Handle::Node, -1}
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

  mController->addPath();

  Controller::Path pathController = mController->controllerForPath(0);

  pathController.addSymmetricNode(0, { 30, 20 }, { 20, 10 });
  pathController.addSymmetricNode(1, { 60, 30 }, { 50, 20 });

  refreshHandles();
}

void Sketch::onDraw(const Cairo::RefPtr<Cairo::Context>& context, int width, int height)
{
  const Model::Sketch::PathList& paths = mModel->paths();

  for (Model::Path* path : paths) {
    const Model::Path::EntryList& entries = path->entries();

    if (entries.size() > 1) {
      context->move_to(entries[0].mNode->position().x, entries[0].mNode->position().y);

      for (int i = 1; i < entries.size(); ++i) {
        const Point& control1 = entries[i - 1].mPostControl->position();
        const Point& control2 = entries[i].mPreControl->position();
        const Point& position = entries[i].mNode->position();

        context->curve_to(control1.x, control1.y, control2.x, control2.y, position.x, position.y);
      }

      context->set_source_rgb(0, 0, 0);
      context->set_line_width(2);
      context->stroke();
    }

    if (!mModeStack.empty()) {
      for (const Model::Path::Entry& entry : entries) {
        const Point& nodePosition = entry.mNode->position();
        context->move_to(entry.mPreControl->position().x, entry.mPreControl->position().y);
        context->line_to(nodePosition.x, nodePosition.y);
        context->line_to(entry.mPostControl->position().x, entry.mPostControl->position().y);
      }

      context->set_source_rgb(0, 0, 0);
      context->set_line_width(0.5);
      context->stroke();
    }
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
    Handle newHoverHandle = findHandle(x, y);

    if (newHoverHandle != mHoverHandle) {
      mHoverHandle = newHoverHandle;
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
  mHoverHandle.mIndex = -1;

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

Handle Sketch::findHandle(double x, double y)
{
  const float Radius = HandleSize / 2;

  auto withinRadius = [x, y, Radius](const Point& position) -> bool {
    return position.x - Radius <= x && x < position.x + Radius
      && position.y - Radius <= y && y < position.y + Radius;
  };

  const Model::Sketch::ControlPointList& controlPoints = mModel->controlPoints();

  for (int i = controlPoints.size() - 1; i >= 0; --i) {
    if (withinRadius(controlPoints[i]->position())) {
      return {Handle::ControlPoint, i};
    }
  }

  const Model::Sketch::NodeList& nodes = mModel->nodes();

  for (int i = nodes.size() - 1; i >= 0; --i) {
    if (withinRadius(nodes[i]->position())) {
      return {Handle::Node, i};
    }
  }

  return {Handle::Node, -1};
}

Handle Sketch::findHandle(const Model::Node* node)
{
  const Model::Sketch::NodeList& nodes = mModel->nodes();

  for (int i = 0; i < nodes.size(); ++i) {
    if (nodes[i] == node) {
      return {Handle::Node, i};
    }
  }

  return {Handle::Node, -1};
}

Handle Sketch::findHandle(const Model::ControlPoint* controlPoint)
{
  const Model::Sketch::ControlPointList& controlPoints = mModel->controlPoints();

  for (int i = 0; i < controlPoints.size(); ++i) {
    if (controlPoints[i] == controlPoint) {
      return {Handle::ControlPoint, i};
    }
  }

  return {Handle::ControlPoint, -1};
}

Point Sketch::handlePosition(const Handle& handle) const
{
  switch (handle.mType) {
    case Handle::Type::Node:
      return mModel->nodes()[handle.mIndex]->position();
    case Handle::Type::ControlPoint:
      return mModel->controlPoints()[handle.mIndex]->position();
    default:
      return Point();
  }
}

void Sketch::setHandlePosition(const Handle& handle, const Point& position)
{
  switch (handle.mType) {
    case Handle::Type::Node:
      mController->controllerForNode(handle.mIndex).setPosition(position);
      break;
    case Handle::Type::ControlPoint:
      mController->controllerForControlPoint(handle.mIndex).setPosition(position);
      break;
  }
}

const Model::Node* Sketch::nodeForHandle(const Handle& handle)
{
  switch (handle.mType) {
    case Handle::Node:
      return mModel->nodes()[handle.mIndex];
    case Handle::ControlPoint:
      return mModel->controlPoints()[handle.mIndex]->node();
    default:
      return nullptr;
  }
}

Sketch::Mode* Sketch::activeMode() const
{
  if (!mModeStack.empty()) {
    return mModeStack.front();
  } else {
    return nullptr;
  }
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
