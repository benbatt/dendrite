#include "controller/node.h"

#include "controller/controlpoint.h"
#include "controller/undo.h"
#include "model/controlpoint.h"
#include "model/node.h"

#include <cassert>

namespace Controller
{

Node::Node(UndoManager* undoManager, Accessor* sketchAccessor, int nodeIndex)
  : mUndoManager(undoManager)
  , mAccessor(sketchAccessor)
  , mNodeIndex(nodeIndex)
{
}

class SetNodeTypeCommand : public UndoCommand
{
public:
  SetNodeTypeCommand(Node::Accessor* sketchAccessor, int nodeIndex, Node::Type type)
    : mAccessor(sketchAccessor)
    , mNodeIndex(nodeIndex)
    , mType(type)
    , mOldType(sketchAccessor->getNode(nodeIndex)->type())
  {
  }

  void redo() override
  { 
    Node::type(mAccessor->getNode(mNodeIndex)) = mType;
  }

  void undo() override
  { 
    Node::type(mAccessor->getNode(mNodeIndex)) = mOldType;
  }

  std::string description() override
  {
    return "Set node type";
  }

private:
  Node::Accessor* mAccessor;
  int mNodeIndex;
  Node::Type mType;
  Node::Type mOldType;
};

void Node::setType(Type type)
{
  mUndoManager->pushCommand(new SetNodeTypeCommand(mAccessor, mNodeIndex, type));
}

class SetNodePositionCommand : public UndoManager::AutoIDCommand<SetNodePositionCommand>
{
public:
  SetNodePositionCommand(Node::Accessor* sketchAccessor, int nodeIndex, const Point& position)
    : mAccessor(sketchAccessor)
    , mNodeIndex(nodeIndex)
    , mPosition(position)
    , mOldPosition(sketchAccessor->getNode(nodeIndex)->position())
  {
    const Model::Node* node = sketchAccessor->getNode(nodeIndex);

    mControlPoints.reserve(node->controlPoints().size());
    mOldControlPoints.reserve(node->controlPoints().size());

    const Vector offset = mPosition - mOldPosition;

    for (int i = 0; i < node->controlPoints().size(); ++i) {
      const Model::ControlPoint* controlPoint = node->controlPoints()[i];

      mControlPoints.push_back(controlPoint->position() + offset);
      mOldControlPoints.push_back(controlPoint->position());
    }
  }

  void redo() override
  { 
    Model::Node* node = mAccessor->getNode(mNodeIndex);

    Node::position(node) = mPosition;

    const Model::Node::ControlPointList& controlPoints = node->controlPoints();

    for (int i = 0; i < controlPoints.size(); ++i) {
      ControlPoint::position(controlPoints[i]) = mControlPoints[i];
    }
  }

  void undo() override
  { 
    Model::Node* node = mAccessor->getNode(mNodeIndex);

    Node::position(node) = mOldPosition;

    const Model::Node::ControlPointList& controlPoints = node->controlPoints();

    for (int i = 0; i < controlPoints.size(); ++i) {
      ControlPoint::position(controlPoints[i]) = mOldControlPoints[i];
    }
  }

  std::string description() override
  {
    return "Move node";
  }

  bool mergeWith(UndoCommand* other) override
  {
    SetNodePositionCommand* command = static_cast<SetNodePositionCommand*>(other);

    if (command->mAccessor == mAccessor && command->mNodeIndex == mNodeIndex) {
      mPosition = command->mPosition;
      mControlPoints = std::move(command->mControlPoints);
      return true;
    } else {
      return false;
    }
  }

private:
  Node::Accessor* mAccessor;
  int mNodeIndex;
  Point mPosition;
  Point mOldPosition;
  std::vector<Point> mControlPoints;
  std::vector<Point> mOldControlPoints;
};

void Node::setPosition(const Point& position)
{
  mUndoManager->pushCommand(new SetNodePositionCommand(mAccessor, mNodeIndex, position));
}

Point& Node::position(Model::Node* model)
{
  return model->mPosition;
}

Node::Type& Node::type(Model::Node* model)
{
  return model->mType;
}

Model::Node::ControlPointList& Node::controlPoints(Model::Node* model)
{
  return model->mControlPoints;
}

}
