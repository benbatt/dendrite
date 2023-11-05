#pragma once

#include <vector>

#include "model/node.h"

namespace Controller
{
  class Sketch;
}

namespace Model
{

class Sketch
{
public:
  typedef std::vector<Node> NodeList;

  const NodeList& nodes() const { return mNodes; }

private:
  friend class Controller::Sketch;

  NodeList mNodes;
};

}
