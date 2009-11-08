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

#include "PlanetBrush.h"
#include "PlanetFilter.h"
#include "PlanetMapBuffer.h"

using namespace Ogre;

namespace NFSpace {

/**
 * Controller for creating the surface map for a planet.
 */
class PlanetMap {
public:
    static const Real PLANET_TEXTURE_SIZE;
    
    PlanetMap();
    ~PlanetMap();
    
    Image* getHeightMap(int face);

    std::string getMaterial();

    void drawBrush(SceneNode* brushesNode, Vector3 position, Vector2 scale, Vector3 up);

protected:
    void initHelperScene();
    void generateHeightMap();
    void generateNormalMap();
    
    SceneManager *mSceneManager;
    Camera *mCamera;

    PlanetMapBuffer *mHeightMap;
    PlanetMapBuffer *mNormalMap;
};
    
};

#endif