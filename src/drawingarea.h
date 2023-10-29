#pragma once

#include <gtkmm/drawingarea.h>

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

  int findHandle(double x, double y);

  struct Handle
  {
    double x;
    double y;
  };

  std::vector<Handle> mHandles;
  int mHoverIndex;
  int mDragIndex;
  double mDragStartX;
  double mDragStartY;
};
