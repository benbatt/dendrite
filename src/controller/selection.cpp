#include "controller/selection.h"

#include "model/node.h"
#include "model/path.h"
#include "model/reference.h"
#include "model/sketch.h"

namespace Controller
{

using Reference = Model::Reference;

bool Selection::contains(const Reference& reference) const
{
  switch (reference.type()) {
    case Reference::Type::Path:
      return contains(reference.id<Model::Path>());
    case Reference::Type::Node:
      return contains(reference.id<Model::Node>());
    case Reference::Type::ControlPoint:
      return contains(reference.id<Model::ControlPoint>());
    case Reference::Type::Null:
    default:
      return false;
  }
}

void Selection::add(const Reference& reference, const Model::Sketch* sketch)
{
  switch (reference.type()) {
    case Reference::Type::Path:
      add(reference.id<Model::Path>(), sketch);
      break;
    case Reference::Type::Node:
      add(reference.id<Model::Node>(), sketch);
      break;
    case Reference::Type::ControlPoint:
      add(reference.id<Model::ControlPoint>());
      break;
    case Reference::Type::Null:
      break;
  }
}

void Selection::remove(const Reference& reference, const Model::Sketch* sketch)
{
  switch (reference.type()) {
    case Reference::Type::Path:
      remove(reference.id<Model::Path>(), sketch);
      break;
    case Reference::Type::Node:
      remove(reference.id<Model::Node>(), sketch);
      break;
    case Reference::Type::ControlPoint:
      remove(reference.id<Model::ControlPoint>());
      break;
    case Reference::Type::Null:
      break;
  }
}

void Selection::add(const ID<Model::Path>& id, const Model::Sketch* sketch)
{
  const Model::Path* path = sketch->path(id);

  mPaths.insert(id);

  for (auto& entry : path->entries()) {
    add(entry.mNode, sketch);
  }
}

void Selection::add(const ID<Model::Node>& id, const Model::Sketch* sketch)
{
  const Model::Node* node = sketch->node(id);

  mNodes.insert(id);

  for (auto controlPointID : node->controlPoints()) {
    add(controlPointID);
  }
}

void Selection::add(const ID<Model::ControlPoint>& id)
{
  mControlPoints.insert(id);
}

void Selection::remove(const ID<Model::Node>& id, const Model::Sketch* sketch)
{
  const Model::Node* node = sketch->node(id);

  mNodes.erase(id);

  for (auto controlPointID : node->controlPoints()) {
    remove(controlPointID);
  }
}

void Selection::remove(const ID<Model::ControlPoint>& id)
{
  mControlPoints.erase(id);
}

void Selection::remove(const ID<Model::Path>& id, const Model::Sketch* sketch)
{
  const Model::Path* path = sketch->path(id);

  mPaths.erase(id);

  for (auto& entry : path->entries()) {
    mNodes.erase(entry.mNode);
    mControlPoints.erase(entry.mPreControl);
    mControlPoints.erase(entry.mPostControl);
  }
}

}
