#pragma once

#include "controller/sketch.h"
#include "model/sketch.h"
#include "utilities/geometry.h"

#include <cairo.h>
#include <map>
#include <set>
#include <wx/mousemanager.h>
#include <wx/wx.h>

namespace Controller
{
  class UndoManager;
}

namespace View
{

class Context;

class Sketch : public wxControl
{
public:
  Sketch(wxWindow* parent, Model::Sketch* model, Controller::UndoManager* undoManager, Context& context);

  void setStrokeColour(const wxColour& colour);
  void setFillColour(const wxColour& colour);

  struct Handle
  {
    enum Type
    {
      Path,
      Node,
      ControlPoint,
      Null,
    };

    Handle()
      : mType(Null)
      , mID(0)
    {}

    Handle(const ID<Model::ControlPoint>& id)
      : mType(ControlPoint)
      , mID(id.value())
    {}

    Handle(const ID<Model::Node>& id)
      : mType(Node)
      , mID(id.value())
    {}

    Handle(const ID<Model::Path>& id)
      : mType(Path)
      , mID(id.value())
    {}

    template<class TModel>
    ID<TModel> id() const { return ID<TModel>(mID); }

    bool isValid() const { return mID > 0; }
    bool refersTo(Type type) const { return mID > 0 && mType == type; }
    Model::ControlPoint* controlPoint(const Model::Sketch* sketch) const;
    Model::Node* node(const Model::Sketch* sketch) const;

    bool isIn(const Controller::Selection& selection) const;
    void addTo(Controller::Selection* selection, const Model::Sketch* sketch) const;
    void removeFrom(Controller::Selection* selection, const Model::Sketch* sketch) const;

    bool operator==(const Handle& other) const { return mType == other.mType && mID == other.mID; }
    bool operator!=(const Handle& other) const { return !(*this == other); }
    bool operator<(const Handle& other) const;

    Type mType;
    IDValue mID;
  };

private:
  friend class SketchModeMove;
  friend class SketchModeAdd;
  friend class SketchModeDelete;
  friend class SketchModePlace;
  friend class SketchModePlaceSelection;

  void onPaint(wxPaintEvent& event);
  void drawTangents(cairo_t* context);
  void onPointerPressed(wxMouseEvent& event);
  void onSecondaryPointerPressed(wxMouseEvent& event);
  void onPointerMotion(wxMouseEvent& event);
  void onKeyPressed(wxKeyEvent& event);
  void refreshHandles();
  void activateAddMode();
  void activateDeleteMode();
  void activateMoveMode();
  void activateViewMode();
  void bringForward();
  void sendBackward();
  void onCancel();
  void setModel(Model::Sketch* model);

  Handle findHandle(double x, double y, Handle::Type type, const Model::Node::ControlPointList& ignoreControlPoints);
  Handle findHandle(double x, double y);

  Point handlePosition(const Handle& handle) const;
  void setHandlePosition(const Handle& handle, const Point& position);

  ID<Model::Node> nodeIDForHandle(const Handle& handle);

  class MouseEventsManager : public wxMouseEventsManager
  {
  public:
    MouseEventsManager(Sketch* sketch);

    bool MouseClicked(int item) override;
    bool MouseDragBegin(int item, const wxPoint& position) override;
    void MouseDragCancelled(int item) override;
    void MouseDragEnd(int item, const wxPoint& position) override;
    void MouseDragging(int item, const wxPoint& position) override;
    int MouseHitTest(const wxPoint& position) override;

  private:
    int affirmIndex(const Handle& handle);

    Sketch* mSketch;
    std::vector<Handle> mHandles;
    std::map<Handle, int> mIndices;
  };

  class Mode
  {
  public:
    virtual void begin(Sketch& sketch) { };
    virtual void end(Sketch& sketch) { };
    virtual void draw(Sketch& sketch, cairo_t* context, int width, int height) { }
    virtual void onPointerPressed(Sketch& sketch, double x, double y) { }
    virtual void onPointerReleased(Sketch& sketch) { }
    virtual bool onPointerMotion(Sketch& sketch, double x, double y) { return false; }
    virtual bool onKeyPressed(Sketch& sketch, wxKeyEvent& event) { return false; }
    virtual void onCancel(Sketch& sketch) { }
    virtual void onChildPopped(Sketch& sketch, Mode* child) { }
  };

  Mode* activeMode() const;
  void pushMode(Mode* mode);
  void popMode(Mode* mode);
  void cancelActiveMode();
  void cancelModeStack();

  MouseEventsManager mMouseEventsManager;

  Model::Sketch* mModel;
  Controller::Sketch* mController;
  Controller::UndoManager* mUndoManager;
  std::list<Mode*> mModeStack;

  Handle mHoverHandle;
  Controller::Selection mSelection;

  bool mDragging;
  bool mShowDetails;
  Rectangle mDragArea;
};

}
