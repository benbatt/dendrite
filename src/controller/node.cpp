#include "controller/node.h"

#include "controller/controlpoint.h"
#include "controller/undo.h"
#include "model/controlpoint.h"
#include "model/node.h"

#include <cassert>

namespace Controller
{

Node::Node(UndoManager* undoManager, Accessor* sketchAccessor, const ID<Model::Node>& id)
  : mUndoManager(undoManager)
  , mAccessor(sketchAccessor)
  , mID(id)
{
}

class SetNodeTypeCommand : public UndoCommand
{
public:
  SetNodeTypeCommand(Node::Accessor* sketchAccessor, const ID<Model::Node>& id, Node::Type type)
    : mAccessor(sketchAccessor)
    , mID(id)
    , mType(type)
    , mOldType(sketchAccessor->getNode(id)->type())
  {
  }

  void redo() override
  { 
    Node::type(mAccessor->getNode(mID)) = mType;
  }

  void undo() override
  { 
    Node::type(mAccessor->getNode(mID)) = mOldType;
  }

  std::string description() override
  {
    return "Set node type";
  }

private:
  Node::Accessor* mAccessor;
  ID<Model::Node> mID;
  Node::Type mType;
  Node::Type mOldType;
};

void Node::setType(Type type)
{
  mUndoManager->pushCommand(new SetNodeTypeCommand(mAccessor, mID, type));
}

class SetNodePositionCommand : public UndoManager::AutoIDCommand<SetNodePositionCommand>
{
public:
  SetNodePositionCommand(Node::Accessor* sketchAccessor, const ID<Model::Node>& id, const Point& position)
    : mAccessor(sketchAccessor)
    , mID(id)
    , mPosition(position)
    , mOldPosition(sketchAccessor->getNode(id)->position())
  {
    const Model::Node* node = sketchAccessor->getNode(id);

    mControlPoints.reserve(node->controlPoints().size());
    mOldControlPoints.reserve(node->controlPoints().size());

    const Vector offset = mPosition - mOldPosition;

    for (int i = 0; i < node->controlPoints().size(); ++i) {
      const Model::ControlPoint* controlPoint = mAccessor->getControlPoint(node->controlPoints()[i]);

      mControlPoints.push_back(controlPoint->position() + offset);
      mOldControlPoints.push_back(controlPoint->position());
    }
  }

  void redo() override
  { 
    Model::Node* node = mAccessor->getNode(mID);

    Node::position(node) = mPosition;

    const Model::Node::ControlPointList& controlPoints = node->controlPoints();

    for (int i = 0; i < controlPoints.size(); ++i) {
      ControlPoint::position(mAccessor->getControlPoint(controlPoints[i])) = mControlPoints[i];
    }
  }

  void undo() override
  { 
    Model::Node* node = mAccessor->getNode(mID);

    Node::position(node) = mOldPosition;

    const Model::Node::ControlPointList& controlPoints = node->controlPoints();

    for (int i = 0; i < controlPoints.size(); ++i) {
      ControlPoint::position(mAccessor->getControlPoint(controlPoints[i])) = mOldControlPoints[i];
    }
  }

  std::string description() override
  {
    return "Move node";
  }

  bool mergeWith(UndoCommand* other) override
  {
    SetNodePositionCommand* command = static_cast<SetNodePositionCommand*>(other);

    if (command->mAccessor == mAccessor && command->mID == mID) {
      mPosition = command->mPosition;
      mControlPoints = std::move(command->mControlPoints);
      return true;
    } else {
      return false;
    }
  }

private:
  Node::Accessor* mAccessor;
  ID<Model::Node> mID;
  Point mPosition;
  Point mOldPosition;
  std::vector<Point> mControlPoints;
  std::vector<Point> mOldControlPoints;
};

void Node::setPosition(const Point& position)
{
  mUndoManager->pushCommand(new SetNodePositionCommand(mAccessor, mID, position));
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
