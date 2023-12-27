#pragma once

namespace Serialisation
{
  class Layout;
}

class Colour
{
public:
  // RGBA8888 representation
  enum { ComponentMask = 0xFF, RedShift = 0, GreenShift = 8, BlueShift = 16, AlphaShift = 24 };

  Colour(float red, float green, float blue, float alpha)
    : mValue((static_cast<uint32_t>(red * ComponentMask) << RedShift)
      | (static_cast<uint32_t>(green * ComponentMask) << GreenShift)
      | (static_cast<uint32_t>(blue * ComponentMask) << BlueShift)
      | (static_cast<uint32_t>(alpha * ComponentMask) << AlphaShift))
  {}

  float red() const { return static_cast<float>((mValue >> RedShift) & ComponentMask) / ComponentMask; }
  float green() const { return static_cast<float>((mValue >> GreenShift) & ComponentMask) / ComponentMask; }
  float blue() const { return static_cast<float>((mValue >> BlueShift) & ComponentMask) / ComponentMask; }
  float alpha() const { return static_cast<float>((mValue >> AlphaShift) & ComponentMask) / ComponentMask; }

private:
  friend class Serialisation::Layout;

  uint32_t mValue;
};
