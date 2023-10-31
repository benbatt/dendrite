#pragma once

#include <gtkmm/drawingarea.h>

struct Vector
{
  double x;
  double y;
};

struct Point
{
  Point operator+(const Vector& v) const;
  Vector operator-(const Point& p) const;
  double x;
  double y;
};

class DrawingArea : public Gtk::DrawingArea
{
public:
  DrawingArea();

private:
  void onDraw(const Cairo::RefPtr<Cairo::Context>& context, int width, int height);
  void onPressed(int count, double x, double y);
  void onPointerMotion(double x, double y);
  void onDragBegin(double x, double y);
  void onDragUpdate(double x, double y);
  void onDragEnd(double x, double y);

  void addNode(const Point& position, const Vector& controlA, const Vector& controlB);
  int findHandle(double x, double y);

  struct Node
  {
    Point position;
    Vector controlA;
    Vector controlB;
  };

  struct Handle
  {
    int mNodeIndex;

    enum
    {
      Position,
      ControlA,
      ControlB,
    } mType;
  };

  Point handlePosition(const Handle& handle) const;
  void setHandlePosition(const Handle& handle, const Point& position);

  std::vector<Node> mNodes;
  std::vector<Handle> mHandles;

  int mHoverIndex;
  int mDragIndex;
  Point mDragStart;
};
