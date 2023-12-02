#include "controller/controlpoint.h"

#include "controller/undo.h"
#include "model/controlpoint.h"
#include "model/node.h"

#include <cassert>

namespace Controller
{

ControlPoint::ControlPoint(UndoManager* undoManager, Accessor* accessor, int index)
  : mUndoManager(undoManager)
  , mAccessor(accessor)
  , mIndex(index)
{
}

class SetControlPointPositionCommand : public UndoManager::AutoIDCommand<SetControlPointPositionCommand>
{
public:
  SetControlPointPositionCommand(ControlPoint::Accessor* accessor, int index, const Point& position)
    : mAccessor(accessor)
    , mIndex(index)
  {
    Model::ControlPoint* modelA = mAccessor->getControlPoint(mIndex);
    Model::Node* node = modelA->node();

    Model::ControlPoint* modelB = node->controlPoints().front();

    if (modelB == modelA) {
      modelB = node->controlPoints().back();
    }

    auto opposingControlPoint = [=](const Point& control, const Point& currentOpposingControl) {
      Vector offset = control - node->position();

      using NodeType = Model::Node::Type;

      switch (node->type()) {
        case NodeType::Symmetric:
          return node->position() - offset;
        case NodeType::Smooth:
          if (offset != Vector::zero) {
            Vector opposingOffset = currentOpposingControl - node->position();
            return node->position() - offset.normalised() * opposingOffset.length();
          } else {
            return currentOpposingControl;
          }
        case NodeType::Sharp:
          return currentOpposingControl;
        default:
          assert(false);
      }
    };

    mOldPreControl = modelA->position();
    mOldPostControl = modelB->position();

    mPreControl = position;
    mPostControl = opposingControlPoint(mPreControl, modelB->position());
  }

  void redo() override
  { 
    Model::ControlPoint* modelA = mAccessor->getControlPoint(mIndex);

    Model::Node* node = modelA->node();

    Model::ControlPoint* modelB = node->controlPoints().front();

    if (modelB == modelA) {
      modelB = node->controlPoints().back();
    }

    ControlPoint::position(modelA) = mPreControl;
    ControlPoint::position(modelB) = mPostControl;
  }

  void undo() override
  { 
    Model::ControlPoint* modelA = mAccessor->getControlPoint(mIndex);
    Model::Node* node = modelA->node();

    Model::ControlPoint* modelB = node->controlPoints().front();

    if (modelB == modelA) {
      modelB = node->controlPoints().back();
    }

    ControlPoint::position(modelA) = mOldPreControl;
    ControlPoint::position(modelB) = mOldPostControl;
  }

  std::string description() override
  {
    return "Move control point";
  }

  bool mergeWith(UndoCommand* other) override
  {
    SetControlPointPositionCommand* command = static_cast<SetControlPointPositionCommand*>(other);

    if (command->mAccessor == mAccessor && command->mIndex == mIndex) {
      mPreControl = command->mPreControl;
      mPostControl = command->mPostControl;
      return true;
    } else {
      return false;
    }
  }

private:
  ControlPoint::Accessor* mAccessor;
  int mIndex;
  Point mPreControl;
  Point mPostControl;
  Point mOldPreControl;
  Point mOldPostControl;
};

void ControlPoint::setPosition(const Point& position)
{
  mUndoManager->pushCommand(new SetControlPointPositionCommand(mAccessor, mIndex, position));
}

Point& ControlPoint::position(Model::ControlPoint* model)
{
  return model->mPosition;
}

}
