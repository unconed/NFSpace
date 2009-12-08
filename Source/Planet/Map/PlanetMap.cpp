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

const Real PlanetMap::PLANET_TEXTURE_SIZE = 257;

PlanetMap::PlanetMap(PlanetDescriptor* descriptor) : mDescriptor(descriptor) {
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
                                            PlanetMapBuffer::MAP_TYPE_MONO,
                                            PlanetMap::PLANET_TEXTURE_SIZE,
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
    mHeightMapBrushes = mSceneManager->createSceneNode("heightMapBrushes");

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
    
}

void PlanetMap::deleteHeightMap() {
    SceneNode::ObjectIterator it = mHeightMapBrushes->getAttachedObjectIterator();
    while (it.hasMoreElements()) {
        delete it.getNext();
    }
    mHeightMapBrushes->detachAllObjects();

    mSceneManager->destroySceneNode(mHeightMapBrushes);
}
    
PlanetMapTile* PlanetMap::generateTile(int face, int lod, int x, int y) {
    // Generate height texture and load into system memory for analysis.
    mMapBuffer[FRONT]->render(face, lod, x, y, mHeightMapBrushes);
    TexturePtr heightTexture = mMapBuffer[FRONT]->saveTexture(false);
    Image heightImage = mMapBuffer[FRONT]->saveImage(false);

    // Generate normal map based on heightmap texture.
    mMapBuffer[BACK]->filter(face, lod, x, y, PlanetMapBuffer::FILTER_TYPE_NORMAL, mMapBuffer[FRONT]);
    TexturePtr normalTexture = mMapBuffer[BACK]->saveTexture(false);

    return new PlanetMapTile(heightTexture, heightImage, normalTexture, PlanetMap::PLANET_TEXTURE_SIZE);
}

void PlanetMap::drawBrush(SceneNode* brushesNode, Vector3 position, Vector2 scale, Vector3 up) {
    PlanetBrush *brush = new PlanetBrush();
    SceneNode *node = brushesNode->createChildSceneNode();
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