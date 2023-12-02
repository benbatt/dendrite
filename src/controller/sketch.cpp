#include "controller/sketch.h"

#include "controller/undo.h"
#include "model/controlpoint.h"

namespace Controller
{

Sketch::Sketch(UndoManager *undoManager, Model::Sketch* model)
  : mUndoManager(undoManager)
  , mModel(model)
{
}

void Sketch::addPath()
{
  mUndoManager->pushCommand(
      [this]() { mModel->mPaths.push_back(new Model::Path); },
      [this]() {
        delete mModel->mPaths.back();
        mModel->mPaths.pop_back();
      },
      "Add path");
}

Path Sketch::controllerForPath(int index)
{
  return Path(mUndoManager, this, index);
}

Node Sketch::controllerForNode(int index)
{
  return Node(mUndoManager, this, index);
}

Node Sketch::controllerForNode(const Model::Node* node)
{
  const Model::Sketch::NodeList& nodes = mModel->nodes();
  int index = std::distance(nodes.begin(), std::find(nodes.begin(), nodes.end(), node));
  return controllerForNode(index);
}

ControlPoint Sketch::controllerForControlPoint(int index)
{
  return ControlPoint(mUndoManager, this, index);
}

Model::Node* Sketch::getNode(int index)
{
  return mModel->mNodes[index];
}

Model::ControlPoint* Sketch::getControlPoint(int index)
{
  return mModel->mControlPoints[index];
}

Model::Node* Sketch::createNode(const Point& position, Model::Node::Type type)
{
  Model::Node* node = new Model::Node(position, type);
  mModel->mNodes.push_back(node);

  return node;
}

void Sketch::destroyNode(Model::Node* node)
{
  mModel->mNodes.erase(std::remove(mModel->mNodes.begin(), mModel->mNodes.end(), node));
  delete node;
}

Model::ControlPoint* Sketch::createControlPoint(Model::Node* node, const Point& position)
{
  Model::ControlPoint* controlPoint = new Model::ControlPoint(node, position);
  Node::controlPoints(node).push_back(controlPoint);
  mModel->mControlPoints.push_back(controlPoint);

  return controlPoint;
}

void Sketch::destroyControlPoint(Model::ControlPoint* controlPoint)
{
  mModel->mControlPoints.erase(std::remove(mModel->mControlPoints.begin(), mModel->mControlPoints.end(), controlPoint));
  delete controlPoint;
}

Model::Path* Sketch::getPath(int index)
{
  return mModel->mPaths[index];
}

Model::Sketch::NodeList& Sketch::nodes(Model::Sketch* sketch)
{
  return sketch->mNodes;
}

}
