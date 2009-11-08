/*
 *  Application.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 9/06/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef Application_H
#define Application_H

#include <Ogre/Ogre.h>
#include <OIS/OIS.h>

#include "EngineState.h"
#include "PlanetMovable.h"
#include "ApplicationFrameListener.h"

namespace NFSpace {

using namespace Ogre;

class Application {
public:
    void go();
    Application();
    ~Application();
    
private:
    Root *mRoot;
    ApplicationFrameListener *mListener;
    EngineState *mState;
    RenderWindow *mWindow;
    SceneManager *mSceneManager;
    Camera *mCamera;
    Viewport *mVp;
    
    void createRoot();    
    void loadConfig();    
    void defineResources();
    void setupRenderSystem();
    void createRenderWindow();
    void initializeResourceGroups();
    void setupViewport();
    void setupScene();
    void startRenderLoop();
};

std::string resourcePath(std::string file);
void DumpScene(SceneManager *sceneManager);
std::string DumpNodes(Ogre::Node *n);
void DumpNodes(std::stringstream &ss, Ogre::Node *n, int level);

};

#endif