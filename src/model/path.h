#pragma once

#include "utilities/colour.h"
#include "utilities/id.h"

#include <vector>

namespace Controller
{
  class Path;
}

namespace Serialisation
{
  class Layout;
}

namespace Model
{

class Node;
class ControlPoint;

class Path
{
public:
  Path()
    : mStrokeColour(0, 0, 0, 1)
    , mFillColour(0, 0, 0, 1)
    , mFlags(0)
  {}

  struct Entry
  {
    ID<Node> mNode;
    ID<ControlPoint> mPreControl;
    ID<ControlPoint> mPostControl;
  };

  typedef std::vector<Entry> EntryList;

  const EntryList& entries() const { return mEntries; }
  bool isClosed() const { return (mFlags & Flag_Closed) != 0; }
  bool isFilled() const { return (mFlags & Flag_Filled) != 0; }
  const Colour& strokeColour() const { return mStrokeColour; }
  const Colour& fillColour() const { return mFillColour; }

private:
  friend class Controller::Path;
  friend class Serialisation::Layout;

  enum {
    Flag_Closed = 0x00000001,
    Flag_Filled = 0x00000002,
  };

  EntryList mEntries;
  Colour mStrokeColour;
  Colour mFillColour;
  unsigned int mFlags;
};

}
