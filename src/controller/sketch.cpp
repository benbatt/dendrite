#include "controller/sketch.h"

#include "controller/undo.h"

namespace Controller
{

Sketch::Sketch(UndoManager *undoManager, Model::Sketch* model)
  : mUndoManager(undoManager)
  , mModel(model)
{
}

using NodeType = Model::Node::Type;

class AddNodeCommand : public UndoCommand
{
public:
  enum class Position
  {
    Start,
    End,
  };

  AddNodeCommand(Model::Sketch* model, int index, Point position, Vector controlA, Vector controlB,
      NodeType type)
    : mModel(model)
    , mAddPosition()
    , mPosition(position)
    , mControlA(controlA)
    , mControlB(controlB)
    , mType(type)
  {
    if (index == 0) {
      mAddPosition = Position::Start;
    } else if (index == mModel->nodes().size()) {
      mAddPosition = Position::End;
    }
  }

  void redo() override
  { 
    Model::Sketch::NodeList& nodes = Sketch::nodes(mModel);

    if (mAddPosition == Position::Start) {
      nodes.insert(nodes.begin(), Model::Node(mPosition, mControlA, mControlB, mType));
    } else if (mAddPosition == Position::End) {
      nodes.push_back(Model::Node(mPosition, mControlA, mControlB, mType));
    }
  }

  void undo() override
  { 
    Model::Sketch::NodeList& nodes = Sketch::nodes(mModel);

    if (mAddPosition == Position::Start) {
      nodes.erase(nodes.begin());
    } else if (mAddPosition == Position::End) {
      nodes.pop_back();
    }
  }

  std::string description() override
  {
    return "Add node";
  }

private:
  Model::Sketch* mModel;
  Position mAddPosition;
  Point mPosition;
  Vector mControlA;
  Vector mControlB;
  NodeType mType;
};

void Sketch::addSymmetricNode(int index, const Point& position, const Vector& controlA)
{
  mUndoManager->pushCommand(
    new AddNodeCommand(mModel, index, position, controlA, -controlA, NodeType::Symmetric));
}

void Sketch::addSmoothNode(int index, const Point& position, const Vector& controlA, double lengthB)
{
  mUndoManager->pushCommand(
      new AddNodeCommand(mModel, index, position, controlA, -controlA.normalised() * lengthB, NodeType::Smooth));
}

void Sketch::addSharpNode(int index, const Point& position)
{
  mUndoManager->pushCommand(new AddNodeCommand(mModel, index, position, { 0, 0 }, { 0, 0 }, NodeType::Sharp));
}

Node Sketch::controllerForNode(int index)
{
  return Node(mUndoManager, this, index);
}

Model::Node* Sketch::getNode(int index)
{
  return &mModel->mNodes[index];
}

Model::Sketch::NodeList& Sketch::nodes(Model::Sketch* sketch)
{
  return sketch->mNodes;
}

}
