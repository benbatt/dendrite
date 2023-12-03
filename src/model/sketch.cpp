#include "model/sketch.h"

namespace Model
{

Sketch::Sketch()
  : mNextID(1)
{}

ControlPoint* Sketch::controlPoint(const ID<ControlPoint>& id) const
{
  return mControlPoints.at(id);
}

Node* Sketch::node(const ID<Node>& id) const
{
  return mNodes.at(id);
}

Path* Sketch::path(const ID<Path>& id) const
{
  return mPaths.at(id);
}

}
