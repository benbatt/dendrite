#pragma once

struct Vector
{
  double length() const;
  double dot(const Vector& other) const;
  Vector normalised() const;
  Vector operator-() const;
  Vector operator*(double scale) const;

  static const Vector zero;

  double x;
  double y;
};

bool operator==(const Vector& a, const Vector& b);
bool operator!=(const Vector& a, const Vector& b);

struct Point
{
  Point operator+(const Vector& v) const;
  Point operator-(const Vector& v) const;
  Vector operator-(const Point& p) const;

  double x;
  double y;
};

struct Rectangle
{
  Rectangle normalised() const;
  double width() const { return right - left; }
  double height() const { return bottom - top; }

  double left;
  double top;
  double right;
  double bottom;
};
