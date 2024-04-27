#include "sketch.h"

#include "controller/undo.h"
#include "model/controlpoint.h"
#include "view/context.h"

#include <wx/rawbmp.h>

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

ID<Model::Path> findPath(Model::Sketch* sketch, double x, double y);

Model::ControlPoint* Handle::controlPoint(const Model::Sketch* sketch) const
{
  return refersTo(ControlPoint) ? sketch->controlPoint(ID<Model::ControlPoint>(mID)) : nullptr;
}

Model::Node* Handle::node(const Model::Sketch* sketch) const
{
  return refersTo(Node) ? sketch->node(ID<Model::Node>(mID)) : nullptr;
}

bool Handle::isIn(const Controller::Selection& selection) const
{
  switch (mType) {
    case Type::Path:
      return selection.contains(id<Model::Path>());
    case Type::Node:
      return selection.contains(id<Model::Node>());
    case Type::ControlPoint:
      return selection.contains(id<Model::ControlPoint>());
    case Type::Null:
    default:
      return false;
  }
}

void Handle::addTo(Controller::Selection* selection, const Model::Sketch* sketch) const
{
  switch (mType) {
    case Type::Path:
      selection->add(id<Model::Path>(), sketch);
      break;
    case Type::Node:
      selection->add(id<Model::Node>(), sketch);
      break;
    case Type::ControlPoint:
      selection->add(id<Model::ControlPoint>());
      break;
    case Type::Null:
      break;
  }
}

void Handle::removeFrom(Controller::Selection* selection, const Model::Sketch* sketch) const
{
  switch (mType) {
    case Type::Path:
      selection->remove(id<Model::Path>(), sketch);
      break;
    case Type::Node:
      selection->remove(id<Model::Node>(), sketch);
      break;
    case Type::ControlPoint:
      selection->remove(id<Model::ControlPoint>());
      break;
    case Type::Null:
      break;
  }
}

bool Handle::operator<(const Handle& other) const
{
  return mType < other.mType || (mType == other.mType && mID < other.mID);
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
const double DashLength = 2;

void drawHandle(cairo_t* context, HandleStyle style, const Point& position, bool hover, bool selected = false)
{
  const float HalfSize = HandleSize / 2;

  switch (style) {
    case HandleStyle::Symmetric:
      cairo_arc(context, position.x, position.y, HalfSize, 0, 2 * M_PI);
      cairo_close_path(context);
      break;
    case HandleStyle::Smooth:
      cairo_rectangle(context, position.x - HalfSize, position.y - HalfSize, HandleSize, HandleSize);
      break;
    case HandleStyle::Sharp:
      cairo_move_to(context, position.x - HalfSize, position.y);
      cairo_line_to(context, position.x, position.y - HalfSize);
      cairo_line_to(context, position.x + HalfSize, position.y);
      cairo_line_to(context, position.x, position.y + HalfSize);
      cairo_close_path(context);
      break;
    case HandleStyle::Control:
      cairo_arc(context, position.x, position.y, HalfSize / 2, 0, 2 * M_PI);
      cairo_close_path(context);
      break;
    case HandleStyle::Add:
    case HandleStyle::Delete:
      {
        const float Thickness = 2;
        const float ArmLength = HalfSize - (Thickness / 2);

        cairo_save(context);

        cairo_translate(context, position.x, position.y);

        if (style == HandleStyle::Delete) {
          cairo_rotate(context, M_PI / 4);
        }

        cairo_move_to(context, -(Thickness / 2), -(Thickness / 2));

        // Up
        cairo_rel_line_to(context, 0, -ArmLength);
        cairo_rel_line_to(context, Thickness, 0);
        cairo_rel_line_to(context, 0, ArmLength);

        // Right
        cairo_rel_line_to(context, ArmLength, 0);
        cairo_rel_line_to(context, 0, Thickness);
        cairo_rel_line_to(context, -ArmLength, 0);

        // Down
        cairo_rel_line_to(context, 0, ArmLength);
        cairo_rel_line_to(context, -Thickness, 0);
        cairo_rel_line_to(context, 0, -ArmLength);

        // Left
        cairo_rel_line_to(context, -ArmLength, 0);
        cairo_rel_line_to(context, 0, -Thickness);
        cairo_rel_line_to(context, ArmLength, 0);

        cairo_close_path(context);

        cairo_restore(context);
      }
      break;
  }

  cairo_set_source_rgb(context, 0, 0, 0);
  cairo_set_line_width(context, 2);
  cairo_stroke_preserve(context);

  if (selected) {
    cairo_set_source_rgb(context, 1, 1, 1);
    cairo_set_dash(context, &DashLength, 1, 0);
    cairo_stroke_preserve(context);
    cairo_set_dash(context, nullptr, 0, 0);
  }

  if (hover) {
    cairo_set_source_rgb(context, 1, 1, 1);
  } else {
    cairo_set_source_rgb(context, 0.5, 0.5, 0.5);
  }

  cairo_fill(context);
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
    mPreviousCursor = sketch.GetCursor();
    sketch.SetCursor(wxCURSOR_BLANK);

    setDirectionConstraint(sketch);
  }

  void end(Sketch& sketch) override
  {
    sketch.SetCursor(mPreviousCursor);
  }

  void draw(Sketch& sketch, cairo_t* context, int width, int height) override
  {
    if (mConstrainDirection && mDragHandle.refersTo(Handle::ControlPoint)) {
      const Model::ControlPoint* controlPoint = mDragHandle.controlPoint(sketch.mModel);

      Point start = sketch.mModel->node(controlPoint->node())->position();
      Point end = start + mDirectionConstraint * std::max(width, height);

      cairo_move_to(context, start.x, start.y);
      cairo_line_to(context, end.x, end.y);

      cairo_set_line_width(context, 1);
      cairo_set_source_rgb(context, 1, 0, 1);

      cairo_stroke(context);
    }
  }

  void onPointerPressed(Sketch& sketch, double x, double y) override
  {
    sketch.popMode(this);
  }

  bool onPointerMotion(Sketch& sketch, double x, double y) override
  {
    if (mDragHandle.isValid()) {
      setHandlePosition(sketch, mDragHandle, x, y);
    }

    return true;
  }

  bool onKeyPressed(Sketch& sketch, wxKeyEvent& event) override
  {
    if (event.GetKeyCode() == WXK_CONTROL) {
      if (mDragHandle.isValid()) {
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

        sketch.Refresh();

        return true;
      }
    } else if (event.GetKeyCode() == WXK_SHIFT) {
      mConstrainDirection = !mConstrainDirection;

      if (mConstrainDirection && mDragHandle.isValid()) {
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
    sketch.Refresh();
  }

  wxCursor mPreviousCursor;
  Vector mDirectionConstraint;
  bool mConstrainDirection;
  Handle mDragHandle;
};

SketchModePlace SketchModePlace::sInstance;

class SketchModePlaceSelection : public Sketch::Mode
{
public:
  SketchModePlaceSelection()
  { }

  void prepare(double mouseX, double mouseY)
  {
    mPreviousPosition = Point{mouseX, mouseY};
  }

  void begin(Sketch& sketch) override
  {
    mPreviousCursor = sketch.GetCursor();
    sketch.SetCursor(wxCURSOR_BLANK);
  }

  void end(Sketch& sketch) override
  {
    sketch.SetCursor(mPreviousCursor);
  }

  void onPointerPressed(Sketch& sketch, double x, double y) override
  {
    sketch.popMode(this);
  }

  bool onPointerMotion(Sketch& sketch, double x, double y) override
  {
    const Point position{x, y};

    sketch.mController->moveSelection(sketch.mSelection, position - mPreviousPosition);
    mPreviousPosition = position;

    sketch.Refresh();
    return true;
  }

  void onCancel(Sketch& sketch) override
  {
    sketch.mUndoManager->cancelGroup();
  }

private:
  wxCursor mPreviousCursor;
  Point mPreviousPosition;
};

class SketchModeMove : public Sketch::Mode
{
public:
  static SketchModeMove sInstance;

  enum UpdateMode
  {
    Add,
    Remove,
  };

  void draw(Sketch& sketch, cairo_t* context, int width, int height) override
  {
    sketch.drawTangents(context);

    for (auto [id, node] : sketch.mModel->nodes()) {
      drawHandle(context, handleStyle(node->type(), Handle::Node), node->position(),
        sketch.mHoverHandle == id, sketch.mSelection.contains(id));
    }

    for (auto [id, point] : sketch.mModel->controlPoints()) {
      drawHandle(context, handleStyle(NodeType::Sharp, Handle::ControlPoint), point->position(),
        sketch.mHoverHandle == id, sketch.mSelection.contains(id));
    }
  }

  void onPointerPressed(Sketch& sketch, double x, double y) override
  {
    if (sketch.mHoverHandle.isValid()) {
      sketch.mUndoManager->beginGroup();

      if (sketch.mSelection.isEmpty()) {
        mPlaceMode.setDragHandle(sketch.mHoverHandle);
        sketch.pushMode(&mPlaceMode);
      } else {
        mPlaceSelectionMode.prepare(x, y);
        sketch.pushMode(&mPlaceSelectionMode);
      }
    }
  }

  void onChildPopped(Sketch& sketch, Sketch::Mode* child) override
  {
    if (child == &mPlaceMode || child == &mPlaceSelectionMode) {
      sketch.mUndoManager->endGroup();
    }
  }

private:
  SketchModePlace mPlaceMode;
  SketchModePlaceSelection mPlaceSelectionMode;
};

SketchModeMove SketchModeMove::sInstance;

class SketchModeAdd : public Sketch::Mode
{
public:
  static SketchModeAdd sInstance;

  void begin(Sketch& sketch) override
  {
    mPreviousCursor = sketch.GetCursor();
    sketch.SetCursor(wxCURSOR_CROSS);
  }

  void end(Sketch& sketch) override
  {
    sketch.SetCursor(mPreviousCursor);
  }

  void draw(Sketch& sketch, cairo_t* context, int width, int height) override
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

  void onPointerPressed(Sketch& sketch, double x, double y) override
  {
    if (sketch.mHoverHandle.isValid()) {
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

  bool onKeyPressed(Sketch& sketch, wxKeyEvent& event) override
  {
    if (event.GetKeyCode() == 'C' && sketch.activeMode() == &mSetPositionMode) {
      const Model::Path* path = sketch.mModel->path(mCurrentPath);

      if (path->entries().size() > 2) {
        sketch.cancelActiveMode();
        sketch.mController->controllerForPath(mCurrentPath).setClosed(true);
        sketch.Refresh();
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

      if (attachHandle.isValid()) {
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
  wxCursor mPreviousCursor;
};

SketchModeAdd SketchModeAdd::sInstance;

class SketchModeDelete : public Sketch::Mode
{
public:
  static SketchModeDelete sInstance;

  void begin(Sketch& sketch) override
  {
    mPreviousCursor = sketch.GetCursor();
    sketch.SetCursor(wxCURSOR_CROSS);
  }

  void end(Sketch& sketch) override
  {
    sketch.SetCursor(mPreviousCursor);
  }

  void draw(Sketch& sketch, cairo_t* context, int width, int height) override
  {
    for (auto current : sketch.mModel->nodes()) {
      drawHandle(context, HandleStyle::Delete, current.second->position(), sketch.mHoverHandle == current.first);
    }
  }

  void onPointerPressed(Sketch& sketch, double x, double y) override
  {
    if (sketch.mHoverHandle.refersTo(Sketch::Handle::Node)) {
      sketch.mController->removeNode(sketch.mHoverHandle.id<Model::Node>());
      sketch.mHoverHandle = Sketch::Handle();
      sketch.Refresh();
    }
  }

private:
  wxCursor mPreviousCursor;
};

SketchModeDelete SketchModeDelete::sInstance;

Sketch::Sketch(wxWindow* parent, Model::Sketch* model, Controller::UndoManager *undoManager, Context& context)
  : wxControl(parent, wxID_ANY)
  , mMouseEventsManager(this)
  , mModel(nullptr)
  , mController(nullptr)
  , mUndoManager(undoManager)
  , mDragging(false)
  , mShowDetails(false)
{
  SetBackgroundStyle(wxBG_STYLE_PAINT);

  Bind(wxEVT_PAINT, &Sketch::onPaint, this);

  context.addSignal().connect(sigc::mem_fun(*this, &Sketch::activateAddMode));
  context.deleteSignal().connect(sigc::mem_fun(*this, &Sketch::activateDeleteMode));
  context.moveSignal().connect(sigc::mem_fun(*this, &Sketch::activateMoveMode));
  context.viewSignal().connect(sigc::mem_fun(*this, &Sketch::activateViewMode));
  context.cancelSignal().connect(sigc::mem_fun(*this, &Sketch::onCancel));
  context.bringForwardSignal().connect(sigc::mem_fun(*this, &Sketch::bringForward));
  context.sendBackwardSignal().connect(sigc::mem_fun(*this, &Sketch::sendBackward));
  context.signalModelChanged().connect(sigc::mem_fun(*this, &Sketch::setModel));

  Bind(wxEVT_LEFT_DOWN, &Sketch::onPointerPressed, this);
  Bind(wxEVT_RIGHT_DOWN, &Sketch::onSecondaryPointerPressed, this);
  Bind(wxEVT_MOTION, &Sketch::onPointerMotion, this);
  Bind(wxEVT_KEY_DOWN, &Sketch::onKeyPressed, this);

  undoManager->signalChanged().connect(sigc::mem_fun(*this, &Sketch::refreshHandles));

  setModel(model);
}

bool pathToCairo(cairo_t* context, const Model::Path* path, const Model::Sketch* sketch)
{
  const Model::Path::EntryList& entries = path->entries();

  if (entries.size() > 1) {
    const Point& position = sketch->node(entries[0].mNode)->position();

    cairo_move_to(context, position.x, position.y);

    for (int i = 1; i < entries.size(); ++i) {
      const Point& control1 = sketch->controlPoint(entries[i - 1].mPostControl)->position();
      const Point& control2 = sketch->controlPoint(entries[i].mPreControl)->position();
      const Point& position = sketch->node(entries[i].mNode)->position();

      cairo_curve_to(context, control1.x, control1.y, control2.x, control2.y, position.x, position.y);
    }

    if (path->isClosed()) {
      const Point& control1 = sketch->controlPoint(entries.back().mPostControl)->position();
      const Point& control2 = sketch->controlPoint(entries.front().mPreControl)->position();
      const Point& position = sketch->node(entries.front().mNode)->position();

      cairo_curve_to(context, control1.x, control1.y, control2.x, control2.y, position.x, position.y);
      cairo_close_path(context);
    }

    return true;
  } else {
    return false;
  }
}

void Sketch::onPaint(wxPaintEvent& event)
{
  wxSize size = GetSize();

  cairo_format_t format = CAIRO_FORMAT_RGB24;
  cairo_surface_t* surface = cairo_image_surface_create(format, size.GetWidth(), size.GetHeight());
  cairo_t* context = cairo_create(surface);

  cairo_set_source_rgb(context, 0.7, 0.7, 0.7);
  cairo_paint(context);

  std::vector<Rectangle> extents;

  for (auto pathID : mModel->drawOrder()) {
    const Model::Path* path = mModel->path(pathID);

    if (pathToCairo(context, path, mModel)) {
      if (mSelection.contains(pathID)) {
        Rectangle r;
        cairo_path_extents(context, &r.left, &r.top, &r.right, &r.bottom);
        extents.push_back(r);
      }

      {
        const Colour& colour = path->strokeColour();
        cairo_set_source_rgb(context, colour.red(), colour.green(), colour.blue());
        cairo_set_line_width(context, 2);
        cairo_stroke_preserve(context);
      }

      if (path->isFilled()) {
        const Colour& colour = path->fillColour();
        cairo_set_source_rgb(context, colour.red(), colour.green(), colour.blue());
        cairo_fill(context);
      }

      cairo_new_path(context);
    }
  }

  if (mShowDetails && mModeStack.empty()) {
    drawTangents(context);

    for (auto [id, node] : mModel->nodes()) {
      drawHandle(context, handleStyle(node->type(), Handle::Node), node->position(), false, mSelection.contains(id));
    }

    for (auto [id, point] : mModel->controlPoints()) {
      drawHandle(context, handleStyle(NodeType::Sharp, Handle::ControlPoint), point->position(), false,
        mSelection.contains(id));
    }
  }

  if (!extents.empty()) {
    cairo_save(context);

    for (auto rectangle : extents) {
      cairo_rectangle(context, rectangle.left, rectangle.top, rectangle.width(), rectangle.height());
    }

    cairo_set_source_rgb(context, 0, 0, 0);
    cairo_set_line_width(context, 1);
    cairo_set_dash(context, &DashLength, 1, 0);
    cairo_stroke(context);

    cairo_restore(context);
  }

  for (auto it = mModeStack.rbegin(); it != mModeStack.rend(); ++it) {
    (*it)->draw(*this, context, size.GetWidth(), size.GetHeight());
  }

  if (mDragging) {
    Rectangle rectangle = mDragArea.normalised();

    cairo_save(context);

    cairo_rectangle(context, rectangle.left, rectangle.top, rectangle.width(), rectangle.height());
    cairo_set_source_rgb(context, 1, 1, 1);
    cairo_set_line_width(context, 1);
    cairo_set_dash(context, &DashLength, 1, 0);
    cairo_set_operator(context, CAIRO_OPERATOR_DIFFERENCE);
    cairo_stroke(context);

    cairo_restore(context);
  }

  cairo_destroy(context);

  wxImage image(size);

  wxImagePixelData pixelData(image);
  wxImagePixelData::Iterator it(pixelData);

  unsigned char* cairoData = cairo_image_surface_get_data(surface);
  int cairoStride = cairo_format_stride_for_width(format, size.GetWidth());

  for (int y = 0; y < size.GetHeight(); ++y) {
    auto pixel = it;
    auto cairoPixel = cairoData;

    for (int x = 0; x < size.GetWidth(); ++x, ++pixel, cairoPixel += 4) {
      pixel.Red() = cairoPixel[2];
      pixel.Green() = cairoPixel[1];
      pixel.Blue() = cairoPixel[0];
    }

    it.OffsetY(pixelData, 1);
    cairoData += cairoStride;
  }

  cairo_surface_destroy(surface);

  wxPaintDC dc(this);
  dc.DrawBitmap(wxBitmap(image), 0, 0);
}

void Sketch::drawTangents(cairo_t* context)
{
  for (auto current : mModel->paths()) {
    for (const Model::Path::Entry& entry : current.second->entries()) {
      const Point& nodePosition = mModel->node(entry.mNode)->position();
      const Point& preControl = mModel->controlPoint(entry.mPreControl)->position();
      const Point& postControl = mModel->controlPoint(entry.mPostControl)->position();

      cairo_move_to(context, preControl.x, preControl.y);
      cairo_line_to(context, nodePosition.x, nodePosition.y);
      cairo_line_to(context, postControl.x, postControl.y);
    }
  }

  cairo_set_source_rgb(context, 0, 0, 0);
  cairo_set_line_width(context, 0.5);
  cairo_stroke(context);
}

cairo_t* createCollisionDetectionContext()
{
  const cairo_format_t format = CAIRO_FORMAT_A8;
  const int SurfaceSize = 11;
  cairo_surface_t* surface = cairo_image_surface_create(format, SurfaceSize, SurfaceSize);

  cairo_t* context = cairo_create(surface);

  cairo_set_antialias(context, CAIRO_ANTIALIAS_NONE);

  cairo_surface_destroy(surface);

  return context;
}

bool pointInPath(cairo_t* context, Model::Sketch* sketch, Model::Path* path, double x, double y)
{
  cairo_save(context);

  cairo_surface_t* surface = cairo_get_target(context);

  assert(cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_IMAGE);

  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);

  cairo_set_source_rgba(context, 0, 0, 0, 0);
  cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);
  cairo_paint(context);

  cairo_new_path(context);

  const int OffsetX = width / 2;
  const int OffsetY = height / 2;

  cairo_translate(context, -x + OffsetX, -y + OffsetY);

  bool result = false;

  if (pathToCairo(context, path, sketch)) {
    cairo_set_source_rgba(context, 1, 1, 1, 1);
    cairo_set_operator(context, CAIRO_OPERATOR_OVER);
    cairo_stroke_preserve(context);

    if (path->isFilled()) {
      cairo_fill(context);
    }

    cairo_surface_flush(surface);

    int stride = cairo_image_surface_get_stride(surface);
    unsigned char* imageData = cairo_image_surface_get_data(surface);
    unsigned char* pixel = imageData + (OffsetY * stride) + OffsetX;

    result = *pixel != 0;
  }

  cairo_restore(context);

  return result;
}

ID<Model::Path> findPath(Model::Sketch* sketch, double x, double y)
{
  cairo_t* context = createCollisionDetectionContext();

  cairo_set_line_width(context, 4);

  ID<Model::Path> id;

  for (auto it = sketch->drawOrder().rbegin(); it != sketch->drawOrder().rend(); ++it) {
    Model::Path* path = sketch->path(*it);

    if (pointInPath(context, sketch, path, x, y)) {
      id = *it;
      break;
    }
  }

  cairo_destroy(context);

  return id;
}

bool rectangleIntersectsCairoPath(const Rectangle& rectangle, cairo_path_t* path, bool implicitlyClosed)
{
  cairo_path_data_t* firstPoint = nullptr;
  Point previousPoint = { 0, 0 };

  for (int i = 0; i < path->num_data; i += path->data[i].header.length) {
    cairo_path_data_t* data = &path->data[i];

    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
      previousPoint = Point{ data[1].point.x, data[1].point.y };

      if (!firstPoint) {
        firstPoint = &data[1];
      }

      break;
    case CAIRO_PATH_LINE_TO:
      {
        Point currentPoint = { data[1].point.x, data[1].point.y };

        if (rectangle.intersectsLine(previousPoint, currentPoint)) {
          return true;
        }

        previousPoint = currentPoint;

        break;
      }
    }
  }

  if (implicitlyClosed && firstPoint) {
    Point currentPoint = { firstPoint->point.x, firstPoint->point.y };

    if (rectangle.intersectsLine(previousPoint, currentPoint)) {
      return true;
    }
  }

  return false;
}

template<class T_Process>
void forEachPathInDragArea(Model::Sketch* sketch, const Rectangle& area, T_Process process)
{
  cairo_t* context = createCollisionDetectionContext();

  cairo_set_line_width(context, 2);

  bool crossing = area.right < area.left;

  const Rectangle rectangle = area.normalised();

  for (auto [id, path] : sketch->paths()) {
    cairo_new_path(context);

    bool inDragArea = false;

    if (crossing && path->isFilled()) {
      inDragArea = pointInPath(context, sketch, path, rectangle.left, rectangle.top);
    }

    if (!inDragArea && pathToCairo(context, path, sketch)) {
      if (crossing) {
        cairo_path_t* cairoPath = cairo_copy_path_flat(context);

        inDragArea = rectangleIntersectsCairoPath(rectangle, cairoPath, path->isFilled() && !path->isClosed());

        cairo_path_destroy(cairoPath);
      } else {
        Rectangle extents;
        cairo_stroke_extents(context, &extents.left, &extents.top, &extents.right, &extents.bottom);

        inDragArea = rectangle.contains(extents);
      }
    }

    if (inDragArea) {
      process(id);
    }
  }

  cairo_destroy(context);
}

void Sketch::onPointerPressed(wxMouseEvent& event)
{
  double x = event.GetX();
  double y = event.GetY();

  if (!mModeStack.empty()) {
    mModeStack.front()->onPointerPressed(*this, x, y);
  }

  event.Skip();
}

void Sketch::onSecondaryPointerPressed(wxMouseEvent& event)
{
  onCancel();
}

void Sketch::onPointerMotion(wxMouseEvent& event)
{
  double x = event.GetX();
  double y = event.GetY();

  bool consumed = false;

  if (!mModeStack.empty()) {
    consumed = mModeStack.front()->onPointerMotion(*this, x, y);
  }

  if (!consumed) {
    Handle newHoverHandle = findHandle(x, y);

    if (newHoverHandle != mHoverHandle) {
      mHoverHandle = newHoverHandle;
      Refresh();
    }
  }
}

void Sketch::onKeyPressed(wxKeyEvent& event)
{
  if (!mModeStack.empty()) {
    for (auto it = mModeStack.begin(); it != mModeStack.end(); ++it) {
      if ((*it)->onKeyPressed(*this, event)) {
        return;
      }
    }
  } else if (event.GetKeyCode() == WXK_TAB) {
    mShowDetails = !mShowDetails;
    Refresh();
  }

  event.Skip();
}

void Sketch::refreshHandles()
{
  mHoverHandle.mID = 0;

  Refresh();
}

void Sketch::activateAddMode()
{
  cancelModeStack();
  pushMode(&SketchModeAdd::sInstance);
}

void Sketch::activateDeleteMode()
{
  cancelModeStack();
  pushMode(&SketchModeDelete::sInstance);
}

void Sketch::activateMoveMode()
{
  cancelModeStack();
  pushMode(&SketchModeMove::sInstance);
}

void Sketch::activateViewMode()
{
  cancelModeStack();
}

void Sketch::bringForward()
{
  if (!mSelection.mPaths.empty()) {
    mUndoManager->beginGroup();

    for (auto id : mSelection.mPaths) {
      mController->bringPathForward(id);
    }

    mUndoManager->endGroup();

    Refresh();
  }
}

void Sketch::sendBackward()
{
  if (!mSelection.mPaths.empty()) {
    mUndoManager->beginGroup();

    for (auto id : mSelection.mPaths) {
      mController->sendPathBackward(id);
    }

    mUndoManager->endGroup();

    Refresh();
  }
}

void Sketch::onCancel()
{
  if (!mModeStack.empty()) {
    mModeStack.front()->onCancel(*this);
    mModeStack.front()->end(*this);
    mModeStack.pop_front();

    Refresh();
  }
}

void Sketch::setStrokeColour(const wxColour& colour)
{
  if (!mSelection.mPaths.empty()) {
    mUndoManager->beginGroup();

    for (auto id : mSelection.mPaths) {
      mController->controllerForPath(id).setStrokeColour(Colour(colour.GetRGBA()));
    }

    mUndoManager->endGroup();

    Refresh();
  }
}

void Sketch::setFillColour(const wxColour& colour)
{
  if (!mSelection.mPaths.empty()) {
    mUndoManager->beginGroup();

    for (auto id : mSelection.mPaths) {
      mController->controllerForPath(id).setFillColour(Colour(colour.GetRGBA()));
    }

    mUndoManager->endGroup();

    Refresh();
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
  const float Radius = View::HandleSize / 2;

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
    case Handle::Type::Path:
    case Handle::Type::Null:
      break;
  }
}

Sketch::MouseEventsManager::MouseEventsManager(Sketch* sketch)
  : wxMouseEventsManager(sketch)
  , mSketch(sketch)
  , mHandles{ Handle() }
{
}

bool Sketch::MouseEventsManager::MouseClicked(int item)
{
  if (mSketch->mModeStack.empty()) {
    Controller::Selection& selection = mSketch->mSelection;

    bool replaceSelection = !wxGetKeyState(WXK_SHIFT);

    if (replaceSelection) {
      selection.clear();
    }

    const Handle &handle = mHandles.at(item);

    if (handle.isValid()) {
      if (replaceSelection || !handle.isIn(selection)) {
        handle.addTo(&selection, mSketch->mModel);
      } else {
        handle.removeFrom(&selection, mSketch->mModel);
      }
    }

    mSketch->Refresh();

    return true;
  } else {
    return false;
  }
}

bool Sketch::MouseEventsManager::MouseDragBegin(int item, const wxPoint& position)
{
  mSketch->mDragging = true;
  mSketch->mDragArea.right = position.x;
  mSketch->mDragArea.bottom = position.y;

  mSketch->Refresh();

  return true;
}

void Sketch::MouseEventsManager::MouseDragCancelled(int item)
{
  mSketch->mDragging = false;
  mSketch->Refresh();
}

void Sketch::MouseEventsManager::MouseDragEnd(int item, const wxPoint& position)
{
  mSketch->mDragging = false;

  Controller::Selection& selection = mSketch->mSelection;

  bool add = wxGetKeyState(WXK_SHIFT);
  bool remove = wxGetKeyState(WXK_CONTROL);

  if (!add && !remove) {
    // Replace selection
    selection.clear();
    add = true;
  }

  if (mSketch->mShowDetails) {
    const Rectangle dragArea = mSketch->mDragArea.normalised();

    for (auto [id, node] : mSketch->mModel->nodes()) {
      if (dragArea.contains(node->position())) {
        if (add) {
          selection.add(id, mSketch->mModel);
        } else {
          selection.remove(id, mSketch->mModel);
        }
      }
    }

    for (auto [id, point] : mSketch->mModel->controlPoints()) {
      if (dragArea.contains(point->position())) {
        if (add) {
          selection.add(id);
        } else {
          selection.remove(id);
        }
      }
    }
  } else {
    forEachPathInDragArea(mSketch->mModel, mSketch->mDragArea,
      [this, add, &selection](const ID<Model::Path>& id)
      {
        if (add) {
          selection.add(id, mSketch->mModel);
        } else {
          selection.remove(id, mSketch->mModel);
        }
      });
  }

  mSketch->Refresh();
}

void Sketch::MouseEventsManager::MouseDragging(int item, const wxPoint& position)
{
  mSketch->mDragArea.right = position.x;
  mSketch->mDragArea.bottom = position.y;
  mSketch->Refresh();
}

int Sketch::MouseEventsManager::MouseHitTest(const wxPoint& position)
{
  if (mSketch->mModeStack.empty()) {
    if (mSketch->mShowDetails) {
      Handle handle = mSketch->findHandle(position.x, position.y);

      if (handle.isValid()) {
        return affirmIndex(handle);
      }
    }

    Handle handle = findPath(mSketch->mModel, position.x, position.y);

    mSketch->mDragArea.left = position.x;
    mSketch->mDragArea.top = position.y;

    if (handle.isValid()) {
      return affirmIndex(handle);
    } else {
      return 0;
    }
  } else {
    return wxNOT_FOUND;
  }
}

int Sketch::MouseEventsManager::affirmIndex(const Handle& handle)
{
  auto it = mIndices.find(handle);

  if (it != mIndices.end()) {
    return it->second;
  } else {
    size_t index = mHandles.size();

    mIndices[handle] = index;
    mHandles.push_back(handle);

    return index;
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
  Refresh();
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

    Refresh();
  }
}

void Sketch::cancelActiveMode()
{
  if (!mModeStack.empty()) {
    mModeStack.front()->onCancel(*this);
    mModeStack.front()->end(*this);
    mModeStack.pop_front();
  }

  Refresh();
}

void Sketch::cancelModeStack()
{
  while (!mModeStack.empty()) {
    cancelActiveMode();
  }
}

}
