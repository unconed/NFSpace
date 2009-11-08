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
    
const Real PlanetMap::PLANET_TEXTURE_SIZE = 1025;

PlanetMap::PlanetMap() {
    // Generate heightmap.
    initHelperScene();
    generateHeightMap();
    generateNormalMap();
}

PlanetMap::~PlanetMap() {
    if (mSceneManager) {
        Root::getSingleton().destroySceneManager(mSceneManager);
    }
    if (mHeightMap) {
        delete mHeightMap;
    }
    if (mNormalMap) {
        delete mNormalMap;
    }
}

void PlanetMap::initHelperScene() {
    mSceneManager = Root::getSingleton().createSceneManager(ST_GENERIC, "PlanetMap SceneManager");
    mCamera = mSceneManager->createCamera("PlanetMap Camera");
    mCamera->setAspectRatio(1);
    mCamera->setPosition(0, 0, 0);
    mCamera->setNearClipDistance(0.01);
}

void PlanetMap::generateHeightMap() {
    Vector3 position, rand, up;
    
    SceneNode* brushesNode = mSceneManager->createSceneNode("brushes");

    // Draw N random brushes.
    srand(getInt("planet.seed"));
    for (int i = 0; i < getInt("planet.brushes"); ++i) {
        position = Vector3((randf() * 2 - 1), (randf() * 2 - 1), (randf() * 2 - 1));
        position.normalise();
        up = Vector3(randf() * 2 - 1, randf() * 2 - 1, randf() * 2 - 1);
        float scale = randf() * .95 + .05;
        drawBrush(brushesNode, position, Vector2(scale, scale * (randf() + .5)), up);
    }
    
    /*
    float scale = 0.45;
    drawBrush(brushesNode, Vector3( 1.0, 0.0, 0.0), Vector2(scale, scale), Vector3(0.0, 1.0, 0.0));
    drawBrush(brushesNode, Vector3(-1.0, 0.0, 0.0), Vector2(scale, scale), Vector3(0.0, 1.0, 0.0));
    drawBrush(brushesNode, Vector3(0.0, 1.0, 0.0), Vector2(scale, scale), Vector3(0.0, 0.0, 1.0));
    drawBrush(brushesNode, Vector3(0.0,-1.0, 0.0), Vector2(scale, scale), Vector3(0.0, 0.0, 1.0));
    drawBrush(brushesNode, Vector3(0.0, 0.0, 1.0), Vector2(scale, scale), Vector3(1.0, 0.0, 0.0));
    drawBrush(brushesNode, Vector3(0.0, 0.0,-1.0), Vector2(scale, scale), Vector3(1.0, 0.0, 0.0));
    */
    
    mHeightMap = new PlanetMapBuffer(mSceneManager,
                                     mCamera,
                                     PlanetMapBuffer::MAP_TYPE_MONO,
                                     PlanetMap::PLANET_TEXTURE_SIZE,
                                     0,
                                     0.5f);
    mHeightMap->render(brushesNode);
    mHeightMap->save(false);

    SceneNode::ObjectIterator it = brushesNode->getAttachedObjectIterator();
    while (it.hasMoreElements()) {
        delete it.getNext();
    }
    brushesNode->detachAllObjects();

    mSceneManager->destroySceneNode(brushesNode);
}
    
    
void PlanetMap::generateNormalMap() {
    // Create new buffer for the normal map.
    mNormalMap = new PlanetMapBuffer(mSceneManager,
                                     mCamera,
                                     PlanetMapBuffer::MAP_TYPE_NORMAL,
                                     PlanetMap::PLANET_TEXTURE_SIZE,
                                     0,
                                     1.f);

    // Prepare texture substitution list.
    AliasTextureNamePairList AliasList;
    AliasList.insert(AliasTextureNamePairList::value_type("source", mHeightMap->getTextureName()));

    // Alter the material to use the height map as its source texture.
    MaterialPtr normalMapperMaterial;
    normalMapperMaterial = MaterialManager::getSingleton().getByName("Planet/NormalMapper");
    normalMapperMaterial->applyTextureAliases(AliasList);
    
    // Create scene node to hold all the renderables.
    SceneNode* filterNode = mSceneManager->createSceneNode("filterSet");

    // Create 6 filter faces to represent the cubemap environment.
    for (int face = 0; face < 6; ++face) {
        PlanetFilter *filter = new PlanetFilter(face, PlanetMap::PLANET_TEXTURE_SIZE, 0);
        filter->setMaterial("Planet/NormalMapper");
        filterNode->attachObject(filter);
    }
    
    // Render the scene to the new map.
    mNormalMap->render(filterNode);
    mNormalMap->save(false);

    // Clean-up the renderables and detach them.
    SceneNode::ObjectIterator it = filterNode->getAttachedObjectIterator();
    while (it.hasMoreElements()) {
        delete it.getNext();
    }
    filterNode->detachAllObjects();

    // Destroy the scene node.
    mSceneManager->destroySceneNode(filterNode);
}
    
Image* PlanetMap::getHeightMap(int face) {
    return mHeightMap->getFace(face);
}
    
std::string PlanetMap::getMaterial() {
//    return mHeightMap->getMaterial();
    return mNormalMap->getMaterial();
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