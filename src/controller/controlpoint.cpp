#include "controller/controlpoint.h"

#include "controller/undo.h"
#include "model/controlpoint.h"
#include "model/node.h"

#include <cassert>

namespace Controller
{

ControlPoint::ControlPoint(UndoManager* undoManager, Accessor* accessor, const ID<Model::ControlPoint>& id)
  : mUndoManager(undoManager)
  , mAccessor(accessor)
  , mID(id)
{
}

class SetControlPointPositionCommand : public UndoManager::AutoIDCommand<SetControlPointPositionCommand>
{
public:
  struct Models
  {
    Model::ControlPoint* mPreControl;
    Model::ControlPoint* mPostControl;
    Model::Node* mNode;
  };

  SetControlPointPositionCommand(ControlPoint::Accessor* accessor, const ID<Model::ControlPoint>& id,
    const Point& position)
    : mAccessor(accessor)
    , mID(id)
  {
    Models models = getModels();

    auto opposingControlPoint = [=](const Point& control, const Point& currentOpposingControl) {
      Vector offset = control - models.mNode->position();

      using NodeType = Model::Node::Type;

      switch (models.mNode->type()) {
        case NodeType::Symmetric:
          return models.mNode->position() - offset;
        case NodeType::Smooth:
          if (offset != Vector::zero) {
            Vector opposingOffset = currentOpposingControl - models.mNode->position();
            return models.mNode->position() - offset.normalised() * opposingOffset.length();
          } else {
            return currentOpposingControl;
          }
        case NodeType::Sharp:
          return currentOpposingControl;
        default:
          assert(false);
      }
    };

    mOldPreControl = models.mPreControl->position();
    mOldPostControl = models.mPostControl->position();

    mPreControl = position;
    mPostControl = opposingControlPoint(mPreControl, models.mPostControl->position());
  }

  void redo() override
  { 
    Models models = getModels();

    ControlPoint::position(models.mPreControl) = mPreControl;
    ControlPoint::position(models.mPostControl) = mPostControl;
  }

  void undo() override
  { 
    Models models = getModels();

    ControlPoint::position(models.mPreControl) = mOldPreControl;
    ControlPoint::position(models.mPostControl) = mOldPostControl;
  }

  std::string description() override
  {
    return "Move control point";
  }

  bool mergeWith(UndoCommand* other) override
  {
    SetControlPointPositionCommand* command = static_cast<SetControlPointPositionCommand*>(other);

    if (command->mAccessor == mAccessor && command->mID == mID) {
      mPreControl = command->mPreControl;
      mPostControl = command->mPostControl;
      return true;
    } else {
      return false;
    }
  }

private:
  Models getModels()
  {
    Model::ControlPoint* modelA = mAccessor->getControlPoint(mID);

    Model::Node* node = mAccessor->getNode(modelA->node());

    Model::ControlPoint* modelB = mAccessor->getControlPoint(node->controlPoints().front());

    if (modelB == modelA) {
      modelB = mAccessor->getControlPoint(node->controlPoints().back());
    }

    return { modelA, modelB, node };
  }

  ControlPoint::Accessor* mAccessor;
  ID<Model::ControlPoint> mID;
  Point mPreControl;
  Point mPostControl;
  Point mOldPreControl;
  Point mOldPostControl;
};

void ControlPoint::setPosition(const Point& position)
{
  mUndoManager->pushCommand(new SetControlPointPositionCommand(mAccessor, mID, position));
}

Point& ControlPoint::position(Model::ControlPoint* model)
{
  return model->mPosition;
}

}
