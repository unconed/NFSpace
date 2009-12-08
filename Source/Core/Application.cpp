/*
 *  Application.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 9/06/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Application.h"

#include "PlanetFilter.h"

using namespace NFSpace;
using namespace Ogre;

namespace NFSpace {

void DumpScene(SceneManager *sceneManager) {
    LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, DumpNodes(sceneManager->getRootSceneNode()).c_str());
}    

void DumpNodes(std::stringstream &ss, Ogre::Node *n, int level)
{
    for(int i = 0; i < level; i++)
    {
        ss << " ";
    }
    ss << "SceneNode: " << n->getName() << std::endl;
    
    Ogre::SceneNode::ObjectIterator object_it = ((Ogre::SceneNode *)n)->getAttachedObjectIterator();
    Ogre::Node::ChildNodeIterator node_it = n->getChildIterator();
    
    Ogre::MovableObject *m;
    while(object_it.hasMoreElements())
    {
        for(int i = 0; i < level + 2; i++)
        {
            ss << " ";
        }
        m = object_it.getNext();
        ss << m->getMovableType() << ": " << m->getName() << std::endl;
    }
    while(node_it.hasMoreElements())
    {
        DumpNodes(ss, node_it.getNext(), level + 2);
    }
}


std::string DumpNodes(Ogre::Node *n)
{
    std::stringstream ss;
    ss << std::endl << "Node Hierarchy:" << std::endl;
    DumpNodes(ss, n, 0);
    return ss.str();
}

Application::Application()
: mRoot(0), mListener(0), mState(0) {
};

Application::~Application() {
    delete mState;
    delete mRoot;
}

void Application::go() {
    createRoot();
    loadConfig();
    defineResources();
    setupRenderSystem();
    createRenderWindow();
    initializeResourceGroups();
    setupViewport();
    setupScene();
    startRenderLoop();
}

void Application::createRoot() {
    if (mRoot) delete mRoot;
    mRoot = new Root(resourcePath("Config/plugins.cfg"), "");

    // Add factories
    Root::getSingleton().addMovableObjectFactory(new PlanetMovableFactory);
}

void Application::loadConfig() {
    if (!mState) mState = new EngineState();
    
    String secName, typeName, archName;
    ConfigFile cf;
    cf.load(resourcePath("Config/engine.cfg"));
    
    ConfigFile::SectionIterator seci = cf.getSectionIterator();
    while (seci.hasMoreElements()) {
        secName = seci.peekNextKey();
        ConfigFile::SettingsMultiMap *settings = seci.getNext();
        ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i) {
            mState->setValue(i->first, EngineState::VariableValue(i->second));
        }
    }
                            
}
                            
void Application::defineResources() {
    String secName, typeName, archName;
    ConfigFile cf;
    cf.load(resourcePath("Config/resources.cfg"));

    ConfigFile::SectionIterator seci = cf.getSectionIterator();
    while (seci.hasMoreElements()) {
        secName = seci.peekNextKey();
        ConfigFile::SettingsMultiMap *settings = seci.getNext();
        ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i) {
            typeName = i->first;
            archName = i->second;
            ResourceGroupManager::getSingleton().addResourceLocation(resourcePath(archName), typeName, secName);
        }
    }
}

void Application::setupRenderSystem() {
    mState->dumpValues();

    String val = mState->getStringValue("renderSystem");
    RenderSystemList *renderSystems = mRoot->getAvailableRenderers();
    RenderSystemList::iterator r_it;    
    
    // check through the list of available renderers, looking for the one that contains
    // the string in "val" ('renderSystem' option from the engine.cfg file)
    bool renderSystemFound = false;
    for (r_it = renderSystems->begin(); r_it != renderSystems->end(); r_it++) {
        RenderSystem *tmp = *r_it;
        std::string rName(tmp->getName());
        
        // returns -1 if string not found
        if ((int)rName.find(val) >= 0) {
            mRoot->setRenderSystem(*r_it);
            renderSystemFound = true;
            break;
        }
    }        
    if (!renderSystemFound) {
        OGRE_EXCEPT(0, "Specified render system (" + val + ") not found, exiting...", "Application")
    }
}

void Application::createRenderWindow() {
    String appName = "NFSpace Prototype (OGRE)";
    bool fullscreen;
    unsigned int w, h;
    w = mState->getIntValue("screenWidth");
    h = mState->getIntValue("screenHeight");
    fullscreen = mState->getBoolValue("fullscreen");
    
    // false because we are not using an autocreated window
    mRoot->initialise(false);
    mWindow = mRoot->createRenderWindow(appName, w, h, fullscreen, mState->getOgreRootOptions());
}

void Application::initializeResourceGroups() {
    TextureManager::getSingleton().setDefaultNumMipmaps(0);
    ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void Application::setupViewport() {
    mSceneManager = mRoot->createSceneManager(ST_GENERIC, "Default SceneManager");
    mCamera = mSceneManager->createCamera("Camera");
    mCamera->setAutoAspectRatio(true);
    mVp = mWindow->addViewport(mCamera);
    
    mListener = new ApplicationFrameListener(mWindow, mSceneManager);
    mRoot->addFrameListener(mListener);
    mListener->setCamera(mCamera);
}

void Application::setupScene() {
    //mSceneManager->setDisplaySceneNodes(true);
    
    mVp->setBackgroundColour(Ogre::ColourValue(0.3, 0.3, 0.3));
    
    // temporary set up 
    mCamera->setPosition(230.0f, 150.0f, 350.0f);
    mCamera->lookAt(Vector3(0, 0, 0));
    mCamera->setNearClipDistance(0.01f);
    mSceneManager->setAmbientLight(Ogre::ColourValue(1, 1, 1));
    
    SceneNode* planetNode = mSceneManager->getRootSceneNode()->createChildSceneNode("PlanetNode");
    PlanetMovable *planetMovable = static_cast<PlanetMovable*>(mSceneManager->createMovableObject("PlanetEntity", PlanetMovableFactory::FACTORY_TYPE_NAME));
    planetNode->attachObject(planetMovable);
    
    assert(planetNode->isInSceneGraph());
    assert(planetMovable->isInScene());
    assert(planetMovable->isVisible());

    /*
    SceneNode* filterNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
    filterNode->setScale(Vector3(getReal("planet.radius") + getReal("planet.height") + 2.f));
    PlanetFilter* filter = new PlanetFilter(0, 513, 0);
    filter->setMaterial("Planet/NormalMapper");
    filterNode->attachObject(filter);
     */

    /*
    SceneNode *brush = mSceneManager->getRootSceneNode()->createChildSceneNode();
    brush->setScale(Vector3(getReal("planet.radius") + getReal("planet.height")));
    srand(getInt("planet.seed"));
    Vector3 position, rand, up;
    for (int i = 0; i < getInt("planet.brushes"); ++i) {
        position = Vector3((randf() * 2 - 1), (randf() * 2 - 1), (randf() * 2 - 1));
        position.normalise();
        up = Vector3(randf() * 2 - 1, randf() * 2 - 1, randf() * 2 - 1);
        planetMovable->mMap->drawBrush(brush, -position, Vector2(randf(), randf()), -up);
    }
    */

    //DumpScene(mSceneManager);
}

void Application::startRenderLoop() {
    mRoot->startRendering();
}
    
};

#if OGRE_PLATFORM == PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT)
#else
int main(int argc, char **argv)
#endif
{
    try {
        NFSpace::Application app;
        app.go();
    }
    catch(Exception& e) {
#if OGRE_PLATFORM == PLATFORM_WIN32 || OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        MessageBoxA(NULL, e.getFullDescription().c_str(), "An exception has occurred!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
        fprintf(stderr, "An exception has occurred: %s\n",
                e.getFullDescription().c_str());
#endif
    }
    
    return 0;
}

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#include <CoreFoundation/CoreFoundation.h>

namespace NFSpace {
    
std::string resourcePath(std::string file) {
    char path[1024];
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    assert(mainBundle);
	
    CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
    assert(mainBundleURL);
	
    CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
    assert(cfStringRef);
	
    CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);
	
    CFRelease(mainBundleURL);
    CFRelease(cfStringRef);
	
    return std::string(path) + "/Contents/Resources/" + file;
}
    
};

#else 

namespace NFSpace {

std::string resourcePath(std::string file) {
    return file;
}
    
}

#endif

