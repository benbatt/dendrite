#include "utilities/geometry.h"

#include <cmath>

double Vector::length() const
{
  return std::sqrt(x * x + y * y);
}

double Vector::dot(const Vector& other) const
{
  return x * other.x + y * other.y;
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

const Vector Vector::zero{0, 0};

bool operator==(const Vector& a, const Vector& b)
{
  return a.x == b.x && a.y == b.y;
}

bool operator!=(const Vector& a, const Vector& b)
{
  return !(a == b);
}

Point Point::operator+(const Vector& v) const
{
  return { x + v.x, y + v.y };
}

Point Point::operator-(const Vector& v) const
{
  return { x - v.x, y - v.y };
}

Vector Point::operator-(const Point& p) const
{
  return { x - p.x, y - p.y };
}

Rectangle Rectangle::normalised() const
{
  return Rectangle {
    std::min(left, right),
    std::min(top, bottom),
    std::max(left, right),
    std::max(top, bottom),
  };
}
