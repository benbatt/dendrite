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

class SetNodeTypeCommand : public UndoCommand
{
public:
  SetNodeTypeCommand(Node::SketchAccessor* sketchAccessor, int nodeIndex, Node::Type type)
    : mSketchAccessor(sketchAccessor)
    , mNodeIndex(nodeIndex)
    , mType(type)
    , mOldType(sketchAccessor->getNode(nodeIndex)->type())
  {
  }

  void redo() override
  { 
    Node::type(mSketchAccessor->getNode(mNodeIndex)) = mType;
  }

  void undo() override
  { 
    Node::type(mSketchAccessor->getNode(mNodeIndex)) = mOldType;
  }

  std::string description() override
  {
    return "Set node type";
  }

private:
  Node::SketchAccessor* mSketchAccessor;
  int mNodeIndex;
  Node::Type mType;
  Node::Type mOldType;
};

void Node::setType(Type type)
{
  mUndoManager->pushCommand(new SetNodeTypeCommand(mSketchAccessor, mNodeIndex, type));
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

  void redo() override
  { 
    Node::position(mSketchAccessor->getNode(mNodeIndex)) = mPosition;
  }

  void undo() override
  { 
    Node::position(mSketchAccessor->getNode(mNodeIndex)) = mOldPosition;
  }

  std::string description() override
  {
    return "Move node";
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

    auto opposingControlPoint = [=](const Vector& control, const Vector& currentOpposingControl) {
      switch (model->type()) {
        case Node::Type::Symmetric:
          return -control;
        case Node::Type::Smooth:
          if (control != Vector::zero) {
            return -control.normalised() * currentOpposingControl.length();
          } else {
            return currentOpposingControl;
          }
        case Node::Type::Sharp:
          return currentOpposingControl;
        default:
          assert(false);
      }
    };

    switch (handleType) {
      case Node::ControlA:
        mControlA = position - model->position();
        mControlB = opposingControlPoint(mControlA, model->controlB());
        break;
      case Node::ControlB:
        mControlB = position - model->position();
        mControlA = opposingControlPoint(mControlB, model->controlA());
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

  std::string description() override
  {
    return "Move control point";
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

Node::Type& Node::type(Model::Node* model)
{
  return model->mType;
}

}
