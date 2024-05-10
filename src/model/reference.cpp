#include "reference.h"

#include "model/sketch.h"

namespace Model
{

ControlPoint* Reference::controlPoint(const Sketch* sketch) const
{
  return refersTo(Type::ControlPoint) ? sketch->controlPoint(ID<ControlPoint>(mID)) : nullptr;
}

Node* Reference::node(const Sketch* sketch) const
{
  return refersTo(Type::Node) ? sketch->node(ID<Node>(mID)) : nullptr;
}

bool Reference::operator<(const Reference& other) const
{
  return mType < other.mType || (mType == other.mType && mID < other.mID);
}

}
