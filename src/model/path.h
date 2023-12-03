#pragma once

#include "utilities/id.h"

#include <vector>

namespace Controller
{
  class Path;
}

namespace Model
{

class Node;
class ControlPoint;

class Path
{
public:
  struct Entry
  {
    ID<Node> mNode;
    ID<ControlPoint> mPreControl;
    ID<ControlPoint> mPostControl;
  };

  typedef std::vector<Entry> EntryList;

  const EntryList& entries() const { return mEntries; }

private:
  friend class Controller::Path;

  EntryList mEntries;
};

}
