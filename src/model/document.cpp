#include "model/document.h"

#include "model/sketch.h"

namespace Model
{

Document::Document()
  : mNextID(1)
{
  mSketch = new Sketch(this);
}

Document::~Document()
{
}

}
