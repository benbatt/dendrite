#include "controller/node.h"

namespace Controller
{

Node::Node(Model::Node* model)
  : mModel(model)
{
}

Point Node::handlePosition(HandleType type) const
{
  switch (type) {
    case Position:
      return mModel->position();
      break;
    case ControlA:
      return mModel->position() + mModel->controlA();
      break;
    case ControlB:
      return mModel->position() + mModel->controlB();
      break;
    default:
      return { 0, 0 };
  }
}

void Node::setHandlePosition(HandleType type, const Point& position)
{
  switch (type) {
    case Position:
      mModel->mPosition = position;
      break;
    case ControlA:
      mModel->mControlA = position - mModel->position();
      mModel->mControlB = -mModel->controlA().normalised() * mModel->controlB().length();
      break;
    case ControlB:
      mModel->mControlB = position - mModel->position();
      mModel->mControlA = -mModel->controlB().normalised() * mModel->controlA().length();
      break;
  }
}

}
