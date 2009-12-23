/*
 *  PlanetMap.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 17/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PlanetMap_H
#define PlanetMap_H

#include <Ogre/Ogre.h>
#include <Ogre/OgreEntity.h>

#include "PlanetDescriptor.h"
#include "PlanetBrush.h"
#include "PlanetFilter.h"
#include "PlanetMapBuffer.h"
#include "PlanetMapTile.h"

using namespace Ogre;

namespace NFSpace {

/**
 * Generator for creating surface tiles for planets.
 */
class PlanetMap {
public:
    static const Real PLANET_TEXTURE_SIZE;
    
    enum {
        FRONT,
        BACK
    };
    
    PlanetMap(PlanetDescriptor* descriptor);
    ~PlanetMap();
    
    void drawBrush(SceneNode* brushesNode, Vector3 position, Vector2 scale, Vector3 up);
    PlanetMapTile* generateTile(QuadTreeNode* node);
    PlanetMapTile* generateTile(int face, int lod, int x, int y);

protected:
    void initHelperScene();
    void deleteHelperScene();

    void initBuffers();
    void swapBuffers();
    void deleteBuffers();

    void prepareHeightMap();
    void deleteHeightMap();
    
    PlanetDescriptor* mDescriptor;
    SceneManager* mSceneManager;
    Camera* mCamera;

    SceneNode* mHeightMapBrushes;

    PlanetMapBuffer* mMapBuffer[2];
};
    
};

#endif