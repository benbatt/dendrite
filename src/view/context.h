#pragma once

#include <sigc++/sigc++.h>

class Application;

namespace Model
{
  class Sketch;
}

namespace View
{

class Context
{
public:
  sigc::signal<void()> addSignal() { return mAddSignal; }
  sigc::signal<void()> deleteSignal() { return mDeleteSignal; }
  sigc::signal<void()> moveSignal() { return mMoveSignal; }
  sigc::signal<void()> cancelSignal() { return mCancelSignal; }
  sigc::signal<void()> viewSignal() { return mViewSignal; }
  sigc::signal<void()> bringForwardSignal() { return mBringForwardSignal; }
  sigc::signal<void()> sendBackwardSignal() { return mSendBackwardSignal; }

  sigc::signal<void(Model::Sketch*)> signalModelChanged() { return mModelChangedSignal; }

private:
  friend class ::Application;

  sigc::signal<void()> mAddSignal;
  sigc::signal<void()> mDeleteSignal;
  sigc::signal<void()> mMoveSignal;
  sigc::signal<void()> mCancelSignal;
  sigc::signal<void()> mViewSignal;
  sigc::signal<void()> mBringForwardSignal;
  sigc::signal<void()> mSendBackwardSignal;
  sigc::signal<void(Model::Sketch*)> mModelChangedSignal;
};

}
