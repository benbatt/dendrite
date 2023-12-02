#pragma once

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
    Node* mNode;
    ControlPoint* mPreControl;
    ControlPoint* mPostControl;
  };

  typedef std::vector<Entry> EntryList;

  const EntryList& entries() const { return mEntries; }

private:
  friend class Controller::Path;

  EntryList mEntries;
};

}
