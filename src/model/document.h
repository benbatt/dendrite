#pragma once

#include "utilities/id.h"

namespace Controller
{
  class Sketch;
}

namespace Serialisation
{
  class Reader;
}

namespace Model
{

class Sketch;

class Document
{
public:
  Document();
  ~Document();

  Sketch* sketch() const { return mSketch; }

private:
  friend class Controller::Sketch;
  friend class Serialisation::Reader;

  Sketch* mSketch;
  IDValue mNextID;
};

}
