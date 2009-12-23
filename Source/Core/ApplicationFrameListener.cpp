/*
 *  ApplicationFrameListener.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 14/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "ApplicationFrameListener.h"
#include "EngineState.h"
#include "Planet.h"
#include "PlanetCube.h"

using namespace Ogre;

namespace NFSpace {

ApplicationFrameListener::ApplicationFrameListener(RenderWindow* window, SceneManager* sceneManager)
: mWindow(window), mSceneManager(sceneManager), mCamera(0), mTranslateVector(Vector3::ZERO), mCurrentSpeed(0), mNumScreenShots(0),
mMoveScale(0.0f), mRotScale(0.0f), mTimeUntilNextToggle(0), mFiltering(TFO_BILINEAR), mAniso(1),
mSceneDetailIndex(0), mMoveSpeed(40), mRotateSpeed(18), mDebugOverlay(0), mRun(false),
mInputManager(0), mMouse(0), mKeyboard(0)
{
    mLODCamera = mSceneManager->createCamera("LOD Camera");
    SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode();
    node->attachObject(mLODCamera);
    
    mDebugOverlay = OverlayManager::getSingleton().getByName("Core/DebugOverlay");

    size_t windowHnd = 0;
    std::ostringstream windowHndStr;
    OIS::ParamList pl;
    
    mWindow->getCustomAttribute("WINDOW", &windowHnd);
    windowHndStr << windowHnd;
    pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));
    mInputManager = OIS::InputManager::createInputSystem(pl);
    
    try {
        mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject(OIS::OISKeyboard, false));
        mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject(OIS::OISMouse, false));
        //mJoy = static_cast<OIS::JoyStick*>(mInputManager->createInputObject(OIS::OISJoyStick, false));
    }
    catch (const OIS::Exception &e) {
        throw Exception(42, e.eText, "ApplicationFrameListener::ApplicationFrameListener");
    }
}

//Adjust mouse clipping area
void ApplicationFrameListener::windowResized(RenderWindow* window) {
    unsigned int width, height, depth;
    int left, top;
    window->getMetrics(width, height, depth, left, top);
    
    const OIS::MouseState &ms = mMouse->getMouseState();
    ms.width = width;
    ms.height = height;
}

//Unattach OIS before window shutdown (very important under Linux)
void ApplicationFrameListener::windowClosed(RenderWindow* rw) {
    //Only close for window that created OIS (the main window in these demos)
    if (rw == mWindow) {
        if (mInputManager) {
            mInputManager->destroyInputObject(mMouse);
            mInputManager->destroyInputObject(mKeyboard);
            
            OIS::InputManager::destroyInputSystem(mInputManager);
            mInputManager = 0;
        }
    }
}

bool ApplicationFrameListener::frameStarted(const FrameEvent& evt) {
    mKeyboard->capture();
    return !mKeyboard->isKeyDown(OIS::KC_ESCAPE);
}

void ApplicationFrameListener::setCamera(Camera* camera) {
    mCamera = camera;
}

bool ApplicationFrameListener::processUnbufferedKeyInput(const FrameEvent& evt)
{

    mRun = mKeyboard->isKeyDown(OIS::KC_LSHIFT);
    float boost = mRun ? 2.0 : 1.0;

    if(mKeyboard->isKeyDown(OIS::KC_A))
        mTranslateVector.x = -mMoveScale * boost;	// Move camera left
    
    if(mKeyboard->isKeyDown(OIS::KC_D))
        mTranslateVector.x = mMoveScale * boost;	// Move camera RIGHT
    
    if(mKeyboard->isKeyDown(OIS::KC_UP) || mKeyboard->isKeyDown(OIS::KC_W) )
        mTranslateVector.z = -mMoveScale * boost;	// Move camera forward
    
    if(mKeyboard->isKeyDown(OIS::KC_DOWN) || mKeyboard->isKeyDown(OIS::KC_S) )
        mTranslateVector.z = mMoveScale * boost;	// Move camera backward
    
    if(mKeyboard->isKeyDown(OIS::KC_Q))
        mTranslateVector.y = mMoveScale * boost;	// Move camera up
    
    if(mKeyboard->isKeyDown(OIS::KC_E))
        mTranslateVector.y = -mMoveScale * boost;	// Move camera down
    
    if(mKeyboard->isKeyDown(OIS::KC_RIGHT))
        mCamera->yaw(-mRotScale);
    
    if(mKeyboard->isKeyDown(OIS::KC_LEFT))
        mCamera->yaw(mRotScale);
    
    if( mKeyboard->isKeyDown(OIS::KC_ESCAPE) )
        return false;
    
    if( mKeyboard->isKeyDown(OIS::KC_F) && mTimeUntilNextToggle <= 0 )
    {
        mStatsOn = !mStatsOn;
        showDebugOverlay(mStatsOn);
        mTimeUntilNextToggle = 1;
    }
    
    if( mKeyboard->isKeyDown(OIS::KC_T) && mTimeUntilNextToggle <= 0 )
    {
        switch(mFiltering)
        {
			case TFO_BILINEAR:
				mFiltering = TFO_TRILINEAR;
				mAniso = 1;
				break;
			case TFO_TRILINEAR:
				mFiltering = TFO_ANISOTROPIC;
				mAniso = 8;
				break;
			case TFO_ANISOTROPIC:
				mFiltering = TFO_BILINEAR;
				mAniso = 1;
				break;
			default: break;
        }
        MaterialManager::getSingleton().setDefaultTextureFiltering(mFiltering);
        MaterialManager::getSingleton().setDefaultAnisotropy(mAniso);
        
        showDebugOverlay(mStatsOn);
        mTimeUntilNextToggle = 1;
    }
    
    if(mKeyboard->isKeyDown(OIS::KC_SYSRQ) && mTimeUntilNextToggle <= 0)
    {
        std::ostringstream ss;
        ss << "screenshot_" << ++mNumScreenShots << ".png";
        mWindow->writeContentsToFile(ss.str());
        mTimeUntilNextToggle = 0.5;
        mDebugText = "Saved: " + ss.str();
    }
    
    if(mKeyboard->isKeyDown(OIS::KC_R) && mTimeUntilNextToggle <=0)
    {
        mSceneDetailIndex = (mSceneDetailIndex+1)%3 ;
        switch(mSceneDetailIndex) {
            case 0 : mCamera->setPolygonMode(PM_SOLID); break;
            case 1 : mCamera->setPolygonMode(PM_WIREFRAME); break;
            case 2 : mCamera->setPolygonMode(PM_POINTS); break;
        }
        mTimeUntilNextToggle = 0.5;
    }

    if(mKeyboard->isKeyDown(OIS::KC_TAB) && mTimeUntilNextToggle <=0)
    {
        EngineState::getSingleton().setValue("planet.seed", getInt("planet.seed") + 1);

        srand(time(0) % 65536);
        Real radius = randf();
        Real height = randf() * .25;
        Real norm = 100.f / (radius + height);
        EngineState::getSingleton().setValue("planet.radius", radius * norm);
        EngineState::getSingleton().setValue("planet.height", height * norm);

        PlanetMovable* planetMovable;
        SceneNode *planetNode = mSceneManager->getSceneNode("PlanetNode");
        planetMovable = (PlanetMovable*)planetNode->getAttachedObject(0);
        planetMovable->refresh(PlanetMovableFactory::getDefaultDescriptor());

        /*
        mSceneManager->destroyMovableObject((PlanetMovable*)planetNode->getAttachedObject(0));
        
        planetMovable = static_cast<PlanetMovable*>(mSceneManager->createMovableObject("PlanetEntity", PlanetMovableFactory::FACTORY_TYPE_NAME));
        planetNode->attachObject(planetMovable);
         */
        
        mTimeUntilNextToggle = 0.5;
    }
    
    if(mKeyboard->isKeyDown(OIS::KC_L) && mTimeUntilNextToggle <=0)
    {
        EngineState::getSingleton().setValue("planet.lodFreeze", !EngineState::getSingleton().getBoolValue("planet.lodFreeze"));
        if (mCamera && EngineState::getSingleton().getBoolValue("planet.lodFreeze")) {
            mLODCamera->setPosition(mCamera->getPosition());
            mLODCamera->setOrientation(mCamera->getOrientation());
            mLODCamera->setNearClipDistance(mCamera->getNearClipDistance());
            mLODCamera->setFarClipDistance(mCamera->getFarClipDistance());
            mLODCamera->setFOVy(mCamera->getFOVy());
            mLODCamera->setVisible(true);
        }
        else {
            mLODCamera->setVisible(false);
        }
        mTimeUntilNextToggle = 0.5;
    }

    if(mKeyboard->isKeyDown(OIS::KC_K) && mTimeUntilNextToggle <=0)
    {
        EngineState::getSingleton().setValue("planet.pageFreeze", !EngineState::getSingleton().getBoolValue("planet.pageFreeze"));
        mTimeUntilNextToggle = 0.5;
    }
    
    static bool displayCameraDetails = false;
    if(mKeyboard->isKeyDown(OIS::KC_P) && mTimeUntilNextToggle <= 0)
    {
        displayCameraDetails = !displayCameraDetails;
        mTimeUntilNextToggle = 0.5;
        if (!displayCameraDetails)
            mDebugText = "";
    }
    
    // Print camera details
    if(displayCameraDetails)
        mDebugText = "P: " + StringConverter::toString(mCamera->getDerivedPosition()) +
        " " + "O: " + StringConverter::toString(mCamera->getDerivedOrientation());
    
    // Return true to continue rendering
    return true;
}

bool ApplicationFrameListener::processUnbufferedMouseInput(const FrameEvent& evt)
{
    
    // Rotation factors, may not be used if the second mouse button is pressed
    // 2nd mouse button - slide, otherwise rotate
    const OIS::MouseState &ms = mMouse->getMouseState();
    if( ms.buttonDown( OIS::MB_Right ) )
    {
        mTranslateVector.x += ms.X.rel * 0.13;
        mTranslateVector.y -= ms.Y.rel * 0.13;
    }
    else
    {
        mRotX = Degree(-ms.X.rel * 0.13);
        mRotY = Degree(-ms.Y.rel * 0.13);
    }
    
    return true;
}

void ApplicationFrameListener::moveCamera()
{
    // Make all the changes to the camera
    // Note that YAW direction is around a fixed axis (freelook style) rather than a natural YAW
    //(e.g. airplane)
    mCamera->yaw(mRotX);
    mCamera->pitch(mRotY);
    mCamera->moveRelative(mTranslateVector);
}

void ApplicationFrameListener::showDebugOverlay(bool show) {
    if (mDebugOverlay) {
        if (show)
            mDebugOverlay->show();
        else
            mDebugOverlay->hide();
    }
}

// Override frameRenderingQueued event to process that (don't care about frameEnded)
bool ApplicationFrameListener::frameRenderingQueued(const FrameEvent& evt)
{
    
    if(mWindow->isClosed())	return false;
    
    mSpeedLimit = mMoveScale * evt.timeSinceLastFrame;
    
    //Need to capture/update each device
    mKeyboard->capture();
    mMouse->capture();
    
    Ogre::Vector3 lastMotion = mTranslateVector;
    
    //Check if one of the devices is not buffered
    if( !mMouse->buffered() || !mKeyboard->buffered())
    {
        // one of the input modes is immediate, so setup what is needed for immediate movement
        if (mTimeUntilNextToggle >= 0)
            mTimeUntilNextToggle -= evt.timeSinceLastFrame;
        
        // Move about 100 units per second
        mMoveScale = mMoveSpeed * evt.timeSinceLastFrame;
        // Take about 10 seconds for full rotation
        mRotScale = mRotateSpeed * evt.timeSinceLastFrame;
        
        mRotX = 0;
        mRotY = 0;
        mTranslateVector = Ogre::Vector3::ZERO;
        
    }
    
    //Check to see which device is not buffered, and handle it
    if( !mKeyboard->buffered() )
        if( processUnbufferedKeyInput(evt) == false )
            return false;
    if( !mMouse->buffered() )
        if( processUnbufferedMouseInput(evt) == false )
            return false;
    
    // ramp up / ramp down speed
    if (mTranslateVector == Ogre::Vector3::ZERO)
    {
        // decay (one third speed)
        mCurrentSpeed -= evt.timeSinceLastFrame * 0.3;
        mTranslateVector = lastMotion;
    }
    else
    {
        // ramp up
        mCurrentSpeed += evt.timeSinceLastFrame;
        
    }
    // Limit motion speed
    if (mCurrentSpeed > 1.0)
        mCurrentSpeed = 1.0;
    if (mCurrentSpeed < 0.0)
        mCurrentSpeed = 0.0;
    
    mTranslateVector *= mCurrentSpeed;
    
    
    if( !mMouse->buffered() || !mKeyboard->buffered() )
        moveCamera();
    
    // Do cube maintenance.
    Planet::doMaintenance();
    return true;
}

bool ApplicationFrameListener::frameEnded(const FrameEvent& evt) {
    updateStats();
    return true;
}

void ApplicationFrameListener::updateStats() {
    static String currFps = "Current FPS: ";
    static String avgFps = "Average FPS: ";
    static String bestFps = "Best FPS: ";
    static String worstFps = "Worst FPS: ";
    static String tris = "Triangle Count: ";
    static String batches = "Batch Count: ";
    
    static String planetNodes = "Nodes: ";
    static String planetLeafs = "Open Nodes: ";
    static String planetTiles = "Tiles: ";
    static String planetPagedOut = "Nodes/tiles paged out: ";
    static String planetRenderables = "Renderables: ";
    static String planetActiveRenderables = "Drawn: ";
    static String planetHotTiles = "Hot tiles: ";
    static String planetQueue = "Queue size: ";
    static String planetMemory = "GPU tile cache: ";
    
    // update stats when necessary
    try {
        OverlayElement* guiText = OverlayManager::getSingleton().getOverlayElement("NF/TreeStats");
        guiText->setCaption(
                            planetNodes + StringConverter::toString(PlanetStats::totalNodes) + "\n" +
                            planetLeafs + StringConverter::toString(PlanetStats::totalOpenNodes) + "\n" +
                            planetTiles + StringConverter::toString(PlanetStats::totalTiles) + "\n" +
                            planetPagedOut + StringConverter::toString(PlanetStats::totalPagedOut) + "\n" +
                            planetRenderables + StringConverter::toString(PlanetStats::totalRenderables) + "\n" +
                            planetActiveRenderables + StringConverter::toString(PlanetStats::renderedRenderables) + "\n" +
                            planetHotTiles + StringConverter::toString(PlanetStats::hotTiles) + "\n" +
                            planetQueue + StringConverter::toString(PlanetStats::requestQueue) + "\n" +
                            planetMemory + StringConverter::toString(PlanetStats::gpuMemoryUsage >> 20) + " MB" +
                            "");
        
        OverlayElement* guiAvg = OverlayManager::getSingleton().getOverlayElement("Core/AverageFps");
        OverlayElement* guiCurr = OverlayManager::getSingleton().getOverlayElement("Core/CurrFps");
        OverlayElement* guiBest = OverlayManager::getSingleton().getOverlayElement("Core/BestFps");
        OverlayElement* guiWorst = OverlayManager::getSingleton().getOverlayElement("Core/WorstFps");
        
        const RenderTarget::FrameStats& stats = mWindow->getStatistics();
        guiAvg->setCaption(avgFps + StringConverter::toString(stats.avgFPS));
        guiCurr->setCaption(currFps + StringConverter::toString(stats.lastFPS));
        guiBest->setCaption(bestFps + StringConverter::toString(stats.bestFPS)
                            +" "+StringConverter::toString(stats.bestFrameTime)+" ms");
        guiWorst->setCaption(worstFps + StringConverter::toString(stats.worstFPS)
                             +" "+StringConverter::toString(stats.worstFrameTime)+" ms");
        
        OverlayElement* guiTris = OverlayManager::getSingleton().getOverlayElement("Core/NumTris");
        guiTris->setCaption(tris + StringConverter::toString(stats.triangleCount));
        
        OverlayElement* guiBatches = OverlayManager::getSingleton().getOverlayElement("Core/NumBatches");
        guiBatches->setCaption(batches + StringConverter::toString(stats.batchCount));
        
        OverlayElement* guiDbg = OverlayManager::getSingleton().getOverlayElement("Core/DebugText");
        guiDbg->setCaption(mDebugText);
    }
    catch(...) { /* ignore */ }
}

};