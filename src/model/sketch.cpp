#include "model/sketch.h"

namespace Model
{

Sketch::Sketch(Document* parent)
  : mParent(parent)
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

Sketch* Sketch::sketch(const ID<Sketch>& id) const
{
  return mSketches.at(id);
}

}
