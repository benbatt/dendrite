#pragma once

#include "model/reference.h"
#include "model/sketch.h"

#include <set>

namespace Controller
{

struct Selection
{
  void clear()
  {
    mReferences.clear();
  }

  bool contains(const Model::Reference& reference) const;
  bool contains(Model::Type type) const;
  int count(Model::Type type) const;

  void add(const Model::Reference& reference, const Model::Sketch* sketch);
  void remove(const Model::Reference& reference, const Model::Sketch* sketch);

  template <class T_Callback>
  void forEachPathID(T_Callback callback) const
  {
    for (const Model::Reference& reference : mReferences) {
      if (reference.type() == Model::Type::Path) {
        callback(reference.id<Model::Path>());
      }
    }
  }

  template <class T_Callback>
  void forEachNode(const Model::Sketch* sketch, T_Callback callback) const
  {
    for (const Model::Reference& reference : mReferences) {
      if (reference.type() == Model::Type::Node) {
        callback(sketch->node(reference.id<Model::Node>()));
      }
    }
  }

  template <class T_Callback>
  void forEachControlPoint(const Model::Sketch* sketch, T_Callback callback) const
  {
    for (const Model::Reference& reference : mReferences) {
      if (reference.type() == Model::Type::ControlPoint) {
        callback(sketch->controlPoint(reference.id<Model::ControlPoint>()));
      }
    }
  }

  bool operator==(const Selection& other) const
  {
    return mReferences == other.mReferences;
  }

  bool isEmpty() const
  {
    return mReferences.empty();
  }

  std::set<Model::Reference> mReferences;
};

}
