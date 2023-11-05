#include "utilities/geometry.h"

#include <cmath>

double Vector::length() const
{
  return std::sqrt(x * x + y * y);
}

Vector Vector::normalised() const
{
  double length = this->length();

  if (length > 0) {
    return { x / length, y / length };
  } else {
    return { 0, 0 };
  }
}

Vector Vector::operator-() const
{
  return { -x, -y };
}

Vector Vector::operator*(double scale) const
{
  return { x * scale, y * scale };
}

Point Point::operator+(const Vector& v) const
{
  return { x + v.x, y + v.y };
}

Vector Point::operator-(const Point& p) const
{
  return { x - p.x, y - p.y };
}
