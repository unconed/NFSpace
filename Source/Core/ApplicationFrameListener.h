/*
 *  ApplicationFrameListener.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 14/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef ApplicationFrameListener_H
#define ApplicationFrameListener_H

#include <Ogre/Ogre.h>
#include <OIS/OIS.h>

using namespace Ogre;

namespace NFSpace {
    
class ApplicationFrameListener : public FrameListener {
public:
    ApplicationFrameListener(RenderWindow* window, SceneManager* sceneManager);
    
	//Adjust mouse clipping area
	virtual void windowResized(RenderWindow* rw);
	virtual void windowClosed(RenderWindow* rw);    
    bool frameStarted(const FrameEvent& evt);
    void setCamera(Camera* camera);

    virtual void updateStats();
    virtual void moveCamera();
    virtual bool processUnbufferedMouseInput(const FrameEvent& evt);
    virtual bool processUnbufferedKeyInput(const FrameEvent& evt);
    virtual void showDebugOverlay(bool show);
    virtual bool frameRenderingQueued(const FrameEvent& evt);
	virtual bool frameEnded(const FrameEvent& evt);
private:
    SceneManager* mSceneManager;
    RenderWindow* mWindow;
    Camera* mCamera;
    Camera* mLODCamera;
    OIS::InputManager* mInputManager;
    OIS::Keyboard* mKeyboard;
    OIS::Mouse* mMouse;

	bool mStatsOn;
	Vector3 mTranslateVector;
	Real mCurrentSpeed;
	Real mMoveSpeed;
	Degree mRotateSpeed;

    Overlay* mDebugOverlay;
	std::string mDebugText;
    unsigned int mNumScreenShots;
    int mSceneDetailIndex;

    bool mRun;
	float mMoveScale;
	float mSpeedLimit;
	Degree mRotScale;
	// just to stop toggles flipping too fast
	Real mTimeUntilNextToggle ;
	Radian mRotX, mRotY;
	TextureFilterOptions mFiltering;
	int mAniso;
};
    
};

#endif