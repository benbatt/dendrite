#include "controller/node.h"

#include "controller/undo.h"
#include "model/node.h"

#include <cassert>

namespace Controller
{

Node::Node(UndoManager* undoManager, SketchAccessor* sketchAccessor, int nodeIndex)
  : mUndoManager(undoManager)
  , mSketchAccessor(sketchAccessor)
  , mNodeIndex(nodeIndex)
{
}

Point Node::handlePosition(HandleType type) const
{
  const Model::Node* model = mSketchAccessor->getNode(mNodeIndex);

  switch (type) {
    case Position:
      return model->position();
      break;
    case ControlA:
      return model->position() + model->controlA();
      break;
    case ControlB:
      return model->position() + model->controlB();
      break;
    default:
      return { 0, 0 };
  }
}

class SetNodePositionCommand : public UndoManager::AutoIDCommand<SetNodePositionCommand>
{
public:
  SetNodePositionCommand(Node::SketchAccessor* sketchAccessor, int nodeIndex, const Point& position)
    : mSketchAccessor(sketchAccessor)
    , mNodeIndex(nodeIndex)
    , mPosition(position)
    , mOldPosition(sketchAccessor->getNode(nodeIndex)->position())
  {
  }

  void redo()
  { 
    Node::position(mSketchAccessor->getNode(mNodeIndex)) = mPosition;
  }

  void undo()
  { 
    Node::position(mSketchAccessor->getNode(mNodeIndex)) = mOldPosition;
  }

  bool mergeWith(UndoCommand* other) override
  {
    SetNodePositionCommand* command = static_cast<SetNodePositionCommand*>(other);

    if (command->mSketchAccessor == mSketchAccessor && command->mNodeIndex == mNodeIndex) {
      mPosition = command->mPosition;
      return true;
    } else {
      return false;
    }
  }

private:
  Node::SketchAccessor* mSketchAccessor;
  int mNodeIndex;
  Point mPosition;
  Point mOldPosition;
};

class SetNodeControlPointCommand : public UndoManager::AutoIDCommand<SetNodeControlPointCommand>
{
public:
  SetNodeControlPointCommand(Node::SketchAccessor* sketchAccessor, int nodeIndex, const Point& position,
      Node::HandleType handleType)
    : mSketchAccessor(sketchAccessor)
    , mNodeIndex(nodeIndex)
    , mHandleType(handleType)
    , mOldControlA(sketchAccessor->getNode(nodeIndex)->controlA())
    , mOldControlB(sketchAccessor->getNode(nodeIndex)->controlB())
  {
    Model::Node* model = mSketchAccessor->getNode(mNodeIndex);

    switch (handleType) {
      case Node::ControlA:
        mControlA = position - model->position();
        mControlB = -model->controlA().normalised() * model->controlB().length();
        break;
      case Node::ControlB:
        mControlB = position - model->position();
        mControlA = -model->controlB().normalised() * model->controlA().length();
        break;
      default:
        assert(false);
    }
  }

  void redo() override
  { 
    Model::Node* model = mSketchAccessor->getNode(mNodeIndex);

    Node::controlA(model) = mControlA;
    Node::controlB(model) = mControlB;
  }

  void undo() override
  { 
    Model::Node* model = mSketchAccessor->getNode(mNodeIndex);

    Node::controlA(model) = mOldControlA;
    Node::controlB(model) = mOldControlB;
  }

  bool mergeWith(UndoCommand* other) override
  {
    SetNodeControlPointCommand* command = static_cast<SetNodeControlPointCommand*>(other);

    if (command->mSketchAccessor == mSketchAccessor && command->mNodeIndex == mNodeIndex
      && command->mHandleType == mHandleType) {
      mControlA = command->mControlA;
      mControlB = command->mControlB;
      return true;
    } else {
      return false;
    }
  }

private:
  Node::SketchAccessor* mSketchAccessor;
  int mNodeIndex;
  Node::HandleType mHandleType;
  Vector mControlA;
  Vector mControlB;
  Vector mOldControlA;
  Vector mOldControlB;
};

void Node::setHandlePosition(HandleType type, const Point& position)
{
  switch (type) {
    case Position:
      mUndoManager->pushCommand(new SetNodePositionCommand(mSketchAccessor, mNodeIndex, position));
      break;
    case ControlA:
    case ControlB:
      mUndoManager->pushCommand(new SetNodeControlPointCommand(mSketchAccessor, mNodeIndex, position, type));
      break;
    default:
      assert(false);
  }
}

Point& Node::position(Model::Node* model)
{
  return model->mPosition;
}

Vector& Node::controlA(Model::Node* model)
{
  return model->mControlA;
}

Vector& Node::controlB(Model::Node* model)
{
  return model->mControlB;
}

}
