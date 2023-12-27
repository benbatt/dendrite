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
  Add,
  Delete,
};

using NodeType = Model::Node::Type;
using Handle = Sketch::Handle;

Model::ControlPoint* Handle::controlPoint(const Model::Sketch* sketch) const
{
  return refersTo(ControlPoint) ? sketch->controlPoint(ID<Model::ControlPoint>(mID)) : nullptr;
}

Model::Node* Handle::node(const Model::Sketch* sketch) const
{
  return refersTo(Node) ? sketch->node(ID<Model::Node>(mID)) : nullptr;
}

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
    case HandleStyle::Add:
    case HandleStyle::Delete:
      {
        const float Thickness = 2;
        const float ArmLength = HalfSize - (Thickness / 2);

        context->save();

        context->translate(position.x, position.y);

        if (style == HandleStyle::Delete) {
          context->rotate(M_PI / 4);
        }

        context->move_to(-(Thickness / 2), -(Thickness / 2));

        // Up
        context->rel_line_to(0, -ArmLength);
        context->rel_line_to(Thickness, 0);
        context->rel_line_to(0, ArmLength);

        // Right
        context->rel_line_to(ArmLength, 0);
        context->rel_line_to(0, Thickness);
        context->rel_line_to(-ArmLength, 0);

        // Down
        context->rel_line_to(0, ArmLength);
        context->rel_line_to(-Thickness, 0);
        context->rel_line_to(0, -ArmLength);

        // Left
        context->rel_line_to(-ArmLength, 0);
        context->rel_line_to(0, -Thickness);
        context->rel_line_to(ArmLength, 0);

        context->close_path();

        context->restore();
      }
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
    mPreviousCursor = sketch.get_cursor();
    sketch.set_cursor("none");

    setDirectionConstraint(sketch);
  }

  void end(Sketch& sketch) override
  {
    sketch.set_cursor(mPreviousCursor);
  }

  void draw(Sketch& sketch, const Cairo::RefPtr<Cairo::Context>& context, int width, int height) override
  {
    if (mConstrainDirection && mDragHandle.refersTo(Handle::ControlPoint)) {
      const Model::ControlPoint* controlPoint = mDragHandle.controlPoint(sketch.mModel);

      Point start = sketch.mModel->node(controlPoint->node())->position();
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
        ID<Model::Node> nodeID = sketch.nodeIDForHandle(mDragHandle);

        const Model::Node* node = sketch.mModel->node(nodeID);
        Controller::Node controller = sketch.mController->controllerForNode(nodeID);

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
          const Model::ControlPoint* controlPoint = mDragHandle.controlPoint(sketch.mModel);
          sketch.mController->controllerForControlPoint(mDragHandle.id<Model::ControlPoint>())
            .setPosition(controlPoint->position());
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
      const Model::ControlPoint* controlPoint = mDragHandle.controlPoint(sketch.mModel);
      const Model::Node* node = sketch.mModel->node(controlPoint->node());
      mDirectionConstraint = (controlPoint->position() - node->position()).normalised();
    }
  }

  void setHandlePosition(Sketch& sketch, const Handle& handle, double x, double y)
  {
    Point newPosition{x, y};

    if (mConstrainDirection && handle.refersTo(Handle::ControlPoint)) {
      const Model::ControlPoint* controlPoint = handle.controlPoint(sketch.mModel);
      const Model::Node* node = sketch.mModel->node(controlPoint->node());

      Vector offset = newPosition - node->position();

      double length = std::max(0.0, offset.dot(mDirectionConstraint));

      newPosition = node->position() + mDirectionConstraint * length;
    }

    sketch.setHandlePosition(handle, newPosition);
    sketch.queue_draw();
  }

  Glib::RefPtr<Gdk::Cursor> mPreviousCursor;
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
    sketch.drawTangents(context);

    for (auto current : sketch.mModel->nodes()) {
      bool hover = sketch.mHoverHandle == current.first;
      drawHandle(context, handleStyle(current.second->type(), Handle::Node), current.second->position(), hover);
    }

    for (auto current : sketch.mModel->controlPoints()) {
      bool hover = sketch.mHoverHandle == current.first;
      drawHandle(context, handleStyle(NodeType::Sharp, Handle::ControlPoint), current.second->position(), hover);
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

  void begin(Sketch& sketch) override
  {
    mPreviousCursor = sketch.get_cursor();
    sketch.set_cursor("crosshair");
  }

  void end(Sketch& sketch) override
  {
    sketch.set_cursor(mPreviousCursor);
  }

  void draw(Sketch& sketch, const Cairo::RefPtr<Cairo::Context>& context, int width, int height) override
  {
    sketch.drawTangents(context);

    for (auto current : sketch.mModel->paths()) {
      for (const Model::Path::Entry& entry : current.second->entries()) {
        drawHandle(context, HandleStyle::Add, sketch.mModel->controlPoint(entry.mPreControl)->position(),
          sketch.mHoverHandle == entry.mPreControl);
        drawHandle(context, HandleStyle::Add, sketch.mModel->controlPoint(entry.mPostControl)->position(),
          sketch.mHoverHandle == entry.mPostControl);
      }
    }

    if (sketch.activeMode() == &mAdjustHandlesMode) {
      const Sketch::Handle& handle = mAdjustHandlesMode.dragHandle();

      if (handle.refersTo(Handle::ControlPoint)) {
        const Model::Node* node = sketch.mModel->node(handle.controlPoint(sketch.mModel)->node());
        drawHandle(context, handleStyle(node->type(), Handle::Node), node->position(), true);
      }
    }
  }

  void onPointerPressed(Sketch& sketch, int count, double x, double y) override
  {
    if (sketch.mHoverHandle) {
      addNode(sketch, sketch.mHoverHandle);
      sketch.mHoverHandle = Sketch::Handle();
    } else {
      Point position{x, y};

      sketch.mUndoManager->beginGroup();

      mCurrentPath = sketch.mController->addPath();
      sketch.mController->controllerForPath(mCurrentPath).addSymmetricNode(0, position, position);

      mAdjustHandlesMode.setDragHandle(sketch.mModel->path(mCurrentPath)->entries()[0].mPostControl);
      sketch.pushMode(&mAdjustHandlesMode);
    }
  }

  bool onKeyPressed(Sketch& sketch, guint keyval, guint keycode, Gdk::ModifierType state) override
  {
    if ((keyval == GDK_KEY_C || keyval == GDK_KEY_c) && sketch.activeMode() == &mSetPositionMode) {
      const Model::Path* path = sketch.mModel->path(mCurrentPath);

      if (path->entries().size() > 2) {
        sketch.cancelActiveMode();
        sketch.mController->controllerForPath(mCurrentPath).setClosed(true);
        sketch.queue_draw();
      }

      return true;
    }

    return false;
  }

  void onChildPopped(Sketch& sketch, Sketch::Mode* child) override
  {
    if (child == &mSetPositionMode) {
      Model::Node* node = mSetPositionMode.dragHandle().node(sketch.mModel);
      ID<Model::Node> nodeID = mSetPositionMode.dragHandle().id<Model::Node>();
      const Model::Path::EntryList& entries = sketch.mModel->path(mCurrentPath)->entries();

      Handle attachHandle = sketch.findHandle(node->position().x, node->position().y, Handle::ControlPoint,
          node->controlPoints());

      if (attachHandle) {
        ID<Model::ControlPoint> attachID = attachHandle.id<Model::ControlPoint>();

        const Model::Path* attachPath;
        int entryIndex;
        std::tie(attachPath, entryIndex) = findPathEntry(sketch.mModel, attachID);

        if (attachPath) {
          Model::Path::Entry newEntry = attachPath->entries()[entryIndex];
          Controller::Path pathController = sketch.mController->controllerForPath(mCurrentPath);

          if (nodeID == entries.front().mNode) {
            if (newEntry.mPreControl == attachID) {
              std::swap(newEntry.mPreControl, newEntry.mPostControl);
            }

            sketch.mController->removeNode(nodeID);
            pathController.addEntry(0, newEntry);

            sketch.mUndoManager->endGroup();
          } else if (nodeID == entries.back().mNode) {
            if (newEntry.mPostControl == attachID) {
              std::swap(newEntry.mPreControl, newEntry.mPostControl);
            }

            sketch.mController->removeNode(nodeID);
            pathController.addEntry(entries.size(), newEntry);

            sketch.mUndoManager->endGroup();
          }
        }
      } else {
        if (nodeID == entries.front().mNode) {
          mAdjustHandlesMode.setDragHandle(entries.front().mPreControl);
        } else if (nodeID == entries.back().mNode) {
          mAdjustHandlesMode.setDragHandle(entries.back().mPostControl);
        }

        sketch.pushMode(&mAdjustHandlesMode);
      }
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

    for (auto current : sketch.mModel->paths()) {
      const Model::Path* path = current.second;
      const Model::Path::EntryList& entries = path->entries();

      if (entries.empty()) {
        continue;
      }

      Model::Path::EntryList::const_iterator entryIterator = entries.end();
      int addIndex = -1;

      if (!path->isClosed() && handle == entries.front().mPreControl) {
        addIndex = 0;
      } else if (!path->isClosed() && handle == entries.back().mPostControl) {
        addIndex = entries.size();
      } else {
        entryIterator = std::find_if(entries.begin(), entries.end(),
          [&handle](const Model::Path::Entry& entry) -> bool {
            return handle == entry.mPreControl || handle == entry.mPostControl;
          });

        if (entryIterator == entries.end()) {
          continue;
        }

        addIndex = (handle == entryIterator->mPreControl) ? 0 : 1;
      }

      sketch.mUndoManager->beginGroup();

      mCurrentPath = current.first;

      if (entryIterator != entries.end()) {
        mCurrentPath = sketch.mController->addPath();
        sketch.mController->controllerForPath(mCurrentPath).addEntry(0, *entryIterator);
      }

      const Point& position = handle.controlPoint(sketch.mModel)->position();

      sketch.mController->controllerForPath(mCurrentPath).addSymmetricNode(addIndex, position, position);

      mSetPositionMode.setDragHandle(sketch.mModel->path(mCurrentPath)->entries()[addIndex].mNode);
      sketch.pushMode(&mSetPositionMode);

      break;
    }
  }

  std::tuple<const Model::Path*, int> findPathEntry(const Model::Sketch* sketch,
    const ID<Model::ControlPoint>& controlPoint)
  {
    for (auto current : sketch->paths()) {
      const Model::Path* path = current.second;
      const Model::Path::EntryList& entries = path->entries();

      auto it = std::find_if(entries.begin(), entries.end(),
        [&controlPoint](const Model::Path::Entry& entry) {
          return controlPoint == entry.mPreControl || controlPoint == entry.mPostControl;
        });

      if (it != entries.end()) {
        return std::make_tuple(path, std::distance(entries.begin(), it));
      }
    }

    return std::make_tuple(nullptr, -1);
  }

  SketchModePlace mSetPositionMode;
  SketchModePlace mAdjustHandlesMode;
  ID<Model::Path> mCurrentPath;
  Glib::RefPtr<Gdk::Cursor> mPreviousCursor;
};

SketchModeAdd SketchModeAdd::sInstance;

class SketchModeDelete : public Sketch::Mode
{
public:
  static SketchModeDelete sInstance;

  void begin(Sketch& sketch) override
  {
    mPreviousCursor = sketch.get_cursor();
    sketch.set_cursor("crosshair");
  }

  void end(Sketch& sketch) override
  {
    sketch.set_cursor(mPreviousCursor);
  }

  void draw(Sketch& sketch, const Cairo::RefPtr<Cairo::Context>& context, int width, int height) override
  {
    for (auto current : sketch.mModel->nodes()) {
      drawHandle(context, HandleStyle::Delete, current.second->position(), sketch.mHoverHandle == current.first);
    }
  }

  void onPointerPressed(Sketch& sketch, int count, double x, double y) override
  {
    if (sketch.mHoverHandle.refersTo(Sketch::Handle::Node)) {
      sketch.mController->removeNode(sketch.mHoverHandle.id<Model::Node>());
      sketch.mHoverHandle = Sketch::Handle();
      sketch.queue_draw();
    }
  }

private:
  Glib::RefPtr<Gdk::Cursor> mPreviousCursor;
};

SketchModeDelete SketchModeDelete::sInstance;

Sketch::Sketch(Model::Sketch* model, Controller::UndoManager *undoManager, Context& context)
  : mModel(nullptr)
  , mController(nullptr)
  , mUndoManager(undoManager)
{
  set_focusable(true);
  set_hexpand(true);
  set_vexpand(true);

  set_draw_func(sigc::mem_fun(*this, &Sketch::onDraw));

  context.addAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateAddMode));
  context.deleteAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateDeleteMode));
  context.moveAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateMoveMode));
  context.viewAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::activateViewMode));
  context.cancelAction()->signal_activate().connect(sigc::mem_fun(*this, &Sketch::onCancel));
  context.signalModelChanged().connect(sigc::mem_fun(*this, &Sketch::setModel));

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

  undoManager->signalChanged().connect(sigc::mem_fun(*this, &Sketch::refreshHandles));

  setModel(model);
}

bool pathToCairo(const Cairo::RefPtr<Cairo::Context>& context, const Model::Path* path, const Model::Sketch* sketch)
{
  const Model::Path::EntryList& entries = path->entries();

  if (entries.size() > 1) {
    const Point& position = sketch->node(entries[0].mNode)->position();

    context->move_to(position.x, position.y);

    for (int i = 1; i < entries.size(); ++i) {
      const Point& control1 = sketch->controlPoint(entries[i - 1].mPostControl)->position();
      const Point& control2 = sketch->controlPoint(entries[i].mPreControl)->position();
      const Point& position = sketch->node(entries[i].mNode)->position();

      context->curve_to(control1.x, control1.y, control2.x, control2.y, position.x, position.y);
    }

    if (path->isClosed()) {
      const Point& control1 = sketch->controlPoint(entries.back().mPostControl)->position();
      const Point& control2 = sketch->controlPoint(entries.front().mPreControl)->position();
      const Point& position = sketch->node(entries.front().mNode)->position();

      context->curve_to(control1.x, control1.y, control2.x, control2.y, position.x, position.y);
      context->close_path();
    }

    return true;
  } else {
    return false;
  }
}

void Sketch::onDraw(const Cairo::RefPtr<Cairo::Context>& context, int width, int height)
{
  for (auto current : mModel->paths()) {
    const Model::Path* path = current.second;

    bool drawExtents = (current.first == mSelectedPath);
    double xMin, xMax, yMin, yMax;

    if (pathToCairo(context, path, mModel)) {
      if (drawExtents) {
        context->get_path_extents(xMin, yMin, xMax, yMax);
      }

      {
        const Colour& colour = path->strokeColour();
        context->set_source_rgb(colour.red(), colour.green(), colour.blue());
        context->set_line_width(2);
        context->stroke_preserve();
      }

      if (path->isFilled()) {
        const Colour& colour = path->fillColour();
        context->set_source_rgb(colour.red(), colour.green(), colour.blue());
        context->fill();
      }

      context->begin_new_path();
    }

    if (drawExtents) {
      static const std::vector<double> sDash{2};

      context->save();

      context->rectangle(xMin, yMin, xMax - xMin, yMax - yMin);
      context->set_source_rgb(0, 0, 0);
      context->set_line_width(1);
      context->set_dash(sDash, 0);
      context->stroke();

      context->restore();
    }
  }

  for (auto it = mModeStack.rbegin(); it != mModeStack.rend(); ++it) {
    (*it)->draw(*this, context, width, height);
  }
}

void Sketch::drawTangents(const Cairo::RefPtr<Cairo::Context>& context)
{
  for (auto current : mModel->paths()) {
    for (const Model::Path::Entry& entry : current.second->entries()) {
      const Point& nodePosition = mModel->node(entry.mNode)->position();
      const Point& preControl = mModel->controlPoint(entry.mPreControl)->position();
      const Point& postControl = mModel->controlPoint(entry.mPostControl)->position();

      context->move_to(preControl.x, preControl.y);
      context->line_to(nodePosition.x, nodePosition.y);
      context->line_to(postControl.x, postControl.y);
    }
  }

  context->set_source_rgb(0, 0, 0);
  context->set_line_width(0.5);
  context->stroke();
}

ID<Model::Path> findPath(Model::Sketch* sketch, double x, double y)
{
  auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::A8, 1, 1);
  auto context = Cairo::Context::create(surface);
  context->set_line_width(4);

  for (auto current : sketch->paths()) {
    context->begin_new_path();

    if (pathToCairo(context, current.second, sketch)) {
      if (context->in_stroke(x, y)) {
        return current.first;
      }
    }
  }

  return ID<Model::Path>();
}

void Sketch::onPointerPressed(int count, double x, double y)
{
  if (!mModeStack.empty()) {
    mModeStack.front()->onPointerPressed(*this, count, x, y);
  } else {
    ID<Model::Path> newSelectedPath = findPath(mModel, x, y);

    if (newSelectedPath != mSelectedPath) {
      mSelectedPath = newSelectedPath;
      queue_draw();
    }
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
  for (auto it = mModeStack.begin(); it != mModeStack.end(); ++it) {
    if ((*it)->onKeyPressed(*this, keyval, keycode, state)) {
      return true;
    }
  }

  return false;
}

void Sketch::refreshHandles()
{
  mHoverHandle.mID = 0;

  queue_draw();
}

void Sketch::activateAddMode(const Glib::VariantBase&)
{
  cancelModeStack();
  pushMode(&SketchModeAdd::sInstance);
}

void Sketch::activateDeleteMode(const Glib::VariantBase&)
{
  cancelModeStack();
  pushMode(&SketchModeDelete::sInstance);
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

void Sketch::setStrokeColour(const Gdk::RGBA& rgba)
{
  if (mSelectedPath) {
    Colour colour(rgba.get_red(), rgba.get_green(), rgba.get_blue(), rgba.get_alpha());

    mController->controllerForPath(mSelectedPath).setStrokeColour(colour);

    queue_draw();
  }
}

void Sketch::setFillColour(const Gdk::RGBA& rgba)
{
  if (mSelectedPath) {
    Colour colour(rgba.get_red(), rgba.get_green(), rgba.get_blue(), rgba.get_alpha());

    mController->controllerForPath(mSelectedPath).setFillColour(colour);

    queue_draw();
  }
}

void Sketch::setModel(Model::Sketch* model)
{
  mModel = model;

  delete mController;
  mController = new Controller::Sketch(mUndoManager, mModel);

  cancelModeStack();
  refreshHandles();
}

Handle Sketch::findHandle(double x, double y, Handle::Type type,
  const Model::Node::ControlPointList& ignorePoints)
{
  const float Radius = HandleSize / 2;

  auto withinRadius = [x, y, Radius](const Point& position) -> bool {
    return position.x - Radius <= x && x < position.x + Radius
      && position.y - Radius <= y && y < position.y + Radius;
  };

  if (type == Handle::ControlPoint || type == Handle::Null) {
    for (auto current : mModel->controlPoints()) {
      bool ignore = std::find(ignorePoints.begin(), ignorePoints.end(), current.first) != ignorePoints.end();

      if (!ignore && withinRadius(current.second->position())) {
        return current.first;
      }
    }
  }

  if (type == Handle::Node || type == Handle::Null) {
    for (auto current : mModel->nodes()) {
      if (withinRadius(current.second->position())) {
        return current.first;
      }
    }
  }

  return Handle();
}

Handle Sketch::findHandle(double x, double y)
{
  return findHandle(x, y, Handle::Null, {});
}

Point Sketch::handlePosition(const Handle& handle) const
{
  switch (handle.mType) {
    case Handle::Type::Node:
      return handle.node(mModel)->position();
    case Handle::Type::ControlPoint:
      return handle.controlPoint(mModel)->position();
    default:
      return Point();
  }
}

void Sketch::setHandlePosition(const Handle& handle, const Point& position)
{
  switch (handle.mType) {
    case Handle::Type::Node:
      mController->controllerForNode(handle.id<Model::Node>()).setPosition(position);
      break;
    case Handle::Type::ControlPoint:
      mController->controllerForControlPoint(handle.id<Model::ControlPoint>()).setPosition(position);
      break;
  }
}

ID<Model::Node> Sketch::nodeIDForHandle(const Handle& handle)
{
  switch (handle.mType) {
    case Handle::Node:
      return handle.id<Model::Node>();
    case Handle::ControlPoint:
      return handle.controlPoint(mModel)->node();
    default:
      return ID<Model::Node>();
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

void Sketch::cancelActiveMode()
{
  if (!mModeStack.empty()) {
    mModeStack.front()->onCancel(*this);
    mModeStack.front()->end(*this);
    mModeStack.pop_front();
  }

  queue_draw();
}

void Sketch::cancelModeStack()
{
  while (!mModeStack.empty()) {
    cancelActiveMode();
  }
}

}
