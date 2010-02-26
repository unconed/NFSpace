/*
 *  PlanetMap.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 17/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "PlanetMap.h"

#include "Application.h"
#include "Utility.h"

namespace NFSpace {

PlanetMap::PlanetMap(PlanetDescriptor* descriptor) : mDescriptor(descriptor), mStep(0) {
    initHelperScene();
    initBuffers();
    prepareHeightMap();
}

PlanetMap::~PlanetMap() {
    deleteHeightMap();
    deleteBuffers();
    deleteHelperScene();
}

void PlanetMap::initHelperScene() {
    mSceneManager = Root::getSingleton().createSceneManager(ST_GENERIC, "PlanetMap SceneManager");
    mCamera = mSceneManager->createCamera("PlanetMap Camera");
    mCamera->setAspectRatio(1);
    mCamera->setPosition(0, 0, 0);
    mCamera->setNearClipDistance(0.01);
}

void PlanetMap::deleteHelperScene() {
    if (mSceneManager) {
        Root::getSingleton().destroySceneManager(mSceneManager);
    }
}

void PlanetMap::initBuffers() {
    for (int i = 0; i < 2; ++i) {
        mMapBuffer[i] = new PlanetMapBuffer(mSceneManager,
                                            mCamera,
                                            getInt("planet.textureSize"),
                                            1,
                                            0.5f);
    }
}

void PlanetMap::swapBuffers() {
    PlanetMapBuffer* temp = mMapBuffer[1];
    mMapBuffer[1] = mMapBuffer[0];
    mMapBuffer[0] = temp;
}
    
void PlanetMap::deleteBuffers() {
    for (int i = 0; i < 2; ++i) {
        delete mMapBuffer[i];
    }
}
    
void PlanetMap::prepareHeightMap() {
#ifdef NF_DEBUG_TIMING
    std::ostringstream msg;

    msg.str("");
    msg << "prepareHeightMap";
    log(msg.str());
    
    unsigned long delta, start = Root::getSingleton().getTimer()->getMilliseconds();
#endif
    
    mHeightMapBrushes = mSceneManager->getRootSceneNode()->createChildSceneNode("heightMapBrushes");
    mHeightMapBrushes->setVisible(false, false);

    Vector3 position, rand, up;
    
    // TODO: run real script w/ real descriptor
    
    // Draw N random brushes.
    srand(mDescriptor->seed);
    for (int i = 0; i < mDescriptor->brushes; ++i) {
        position = Vector3((randf() * 2 - 1), (randf() * 2 - 1), (randf() * 2 - 1));
        position.normalise();
        up = Vector3(randf() * 2 - 1, randf() * 2 - 1, randf() * 2 - 1);
        float scale = randf() * .95 + .05;
        drawBrush(mHeightMapBrushes, position, Vector2(scale, scale * (randf() + .5)), up);
    }

#ifdef NF_DEBUG_TIMING
    delta = Root::getSingleton().getTimer()->getMilliseconds() - start;
    msg.str("");
    msg << " => Brush generation (" << delta << "ms)";
    log(msg.str());
    start = Root::getSingleton().getTimer()->getMilliseconds();
#endif    
}

void PlanetMap::deleteHeightMap() {
    SceneNode::ObjectIterator it = mHeightMapBrushes->getAttachedObjectIterator();
    while (it.hasMoreElements()) {
        delete it.getNext();
    }
    mHeightMapBrushes->detachAllObjects();
    
    mSceneManager->destroySceneNode(mHeightMapBrushes);
}
    
void PlanetMap::resetTile() {
    if (mStep > 1) {
        TextureManager::getSingleton().remove(mHeightTexture->getName());
    }
    if (mStep > 2) {
        OGRE_FREE(mHeightImage.getData(), MEMCATEGORY_GENERAL);
    }
    mStep = 0;
}

bool PlanetMap::prepareTile(QuadTreeNode* node) {
    int face = node->mFace,
        lod  = node->mLOD,
        x    = node->mX,
        y    = node->mY;

#ifdef NF_DEBUG_TIMING
    std::ostringstream msg;
    
    msg.str("");
    msg << "generateTile ("
    << face << ", " << lod << ", " << x << ", " << y << ") @ "
    << getInt("planet.textureSize") << "x" << getInt("planet.textureSize")
    log(msg.str());
    
    unsigned long delta, start = Root::getSingleton().getTimer()->getMilliseconds();
#endif
    
    switch (mStep) {
        case 0:
            // Generate height texture in working buffer.
            mMapBuffer[FRONT]->render(face, lod, x, y, mHeightMapBrushes);
            //saveTexture(mMapBuffer[FRONT]->mTexture);
            break;
        
        case 1:
            // Save height texture.
            mHeightTexture = mMapBuffer[FRONT]->saveTexture(false, PlanetMapBuffer::MAP_TYPE_HEIGHT);
            //saveTexture(heightTexture);
            break;
        
        case 2:
            // Download height texture to Image().
            mHeightImage = mMapBuffer[FRONT]->saveImage(false, PlanetMapBuffer::MAP_TYPE_HEIGHT);
            break;

        case 3:
            // Generate normal map.
            mMapBuffer[BACK]->filter(face, lod, x, y, PlanetMapBuffer::FILTER_TYPE_NORMAL, mMapBuffer[FRONT]);
            break;
            
        case 4:
            // Save normal texture.
            mNormalTexture = mMapBuffer[BACK]->saveTexture(false, PlanetMapBuffer::MAP_TYPE_NORMAL);
            break;
    }
#ifdef NF_DEBUG_TIMING
    delta = Root::getSingleton().getTimer()->getMilliseconds() - start;
    msg.str("");
    msg << " => Step " << mStep << " (" << delta << "ms)";
    log(msg.str());
#endif

    mStep++;
    return mStep > 4;
}

PlanetMapTile* PlanetMap::finalizeTile(QuadTreeNode* node) {
    mStep = 0;
    return new PlanetMapTile(node, mHeightTexture, mHeightImage, mNormalTexture, getInt("planet.textureSize"));
}

void PlanetMap::drawBrush(SceneNode* brushesNode, Vector3 position, Vector2 scale, Vector3 up) {
    PlanetBrush* brush = new PlanetBrush();
    SceneNode* node = brushesNode->createChildSceneNode();
    node->setPosition(position);
    node->setScale(scale.x, scale.y, 1);
    
    Vector3 right = position.crossProduct(up);
    Vector3 front = position.crossProduct(right);
    up = right.crossProduct(front);
    right.normalise(); front.normalise(); up.normalise();
    node->setOrientation(Quaternion(right, front, up));
    
    node->attachObject(brush);
}

};