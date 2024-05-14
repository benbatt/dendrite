#include "controller/selection.h"

#include "model/node.h"
#include "model/path.h"
#include "model/sketch.h"

namespace Controller
{

using Reference = Model::Reference;

bool Selection::contains(const Reference& reference) const
{
  return mReferences.count(reference) > 0;
}

bool Selection::contains(Model::Type type) const
{
  for (auto& reference : mReferences) {
    if (reference.type() == type) {
      return true;
    }
  }

  return false;
}

int Selection::count(Model::Type type) const
{
  int result = 0;

  for (auto& reference : mReferences) {
    if (reference.type() == type) {
      ++result;
    }
  }

  return result;
}

void Selection::add(const Reference& reference, const Model::Sketch* sketch)
{
  mReferences.insert(reference);

  switch (reference.type()) {
    case Model::Type::Path:
      {
        const Model::Path* path = sketch->path(reference.id<Model::Path>());

        for (auto& entry : path->entries()) {
          add(entry.mNode, sketch);
        }

        break;
      }
    case Model::Type::Node:
      {
        const Model::Node* node = sketch->node(reference.id<Model::Node>());

        for (auto id : node->controlPoints()) {
          add(id, sketch);
        }

        break;
      }
    case Model::Type::ControlPoint:
    case Model::Type::Null:
      break;
  }
}

void Selection::remove(const Reference& reference, const Model::Sketch* sketch)
{
  mReferences.erase(reference);

  switch (reference.type()) {
    case Model::Type::Path:
      {
        const Model::Path* path = sketch->path(reference.id<Model::Path>());

        for (auto& entry : path->entries()) {
          remove(entry.mNode, sketch);
        }

        break;
      }
    case Model::Type::Node:
      {
        const Model::Node* node = sketch->node(reference.id<Model::Node>());

        for (auto id : node->controlPoints()) {
          remove(id, sketch);
        }

        break;
      }
    case Model::Type::ControlPoint:
    case Model::Type::Null:
      break;
  }
}

}
