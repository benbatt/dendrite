#include "controller/selection.h"

#include "model/node.h"
#include "model/path.h"
#include "model/sketch.h"

namespace Controller
{

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
