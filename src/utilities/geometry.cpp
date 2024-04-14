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

double Vector::cross(const Vector& other) const
{
  return x * other.y - y * other.x;
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

Vector Vector::operator+(const Vector& other) const
{
  return { x + other.x, y + other.y };
}

Vector Vector::operator-(const Vector& other) const
{
  return { x - other.x, y - other.y };
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

bool Rectangle::contains(const Rectangle& other) const
{
  return left <= other.left && top <= other.top && other.right <= right && other.bottom <= bottom;
}

bool Rectangle::contains(const Point& point) const
{
  return left <= point.x && point.x < right && top <= point.y && point.y < bottom;
}

bool Rectangle::intersectsLine(const Point& p1, const Point& p2) const
{
  if (contains(p1) || contains(p2)) {
    return true;
  }

  const Vector delta = p2 - p1;

  auto crossing = [](double a, double b, double threshold) { return (a < threshold) != (b < threshold); };
  auto direction = [p1, delta](double x, double y) { return std::signbit(delta.cross(Point{x, y} - p1)); };

  if (crossing(p1.x, p2.x, left) && direction(left, top) != direction(left, bottom)) {
    return true;
  }

  if (crossing(p1.x, p2.x, right) && direction(right, top) != direction(right, bottom)) {
    return true;
  }

  if (crossing(p1.y, p2.y, top) && direction(left, top) != direction(right, top)) {
    return true;
  }

  if (crossing(p1.y, p2.y, bottom) && direction(left, bottom) != direction(right, bottom)) {
    return true;
  }

  return false;
}
