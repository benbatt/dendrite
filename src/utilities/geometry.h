#pragma once

struct Vector
{
  double length() const;
  Vector normalised() const;
  Vector operator-() const;
  Vector operator*(double scale) const;

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
