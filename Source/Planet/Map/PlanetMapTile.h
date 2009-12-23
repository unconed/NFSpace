/*
 *  PlanetMapTile.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 26/11/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PlanetMapTile_H
#define PlanetMapTile_H

#include <Ogre/Ogre.h>
#include "Planet.h"

using namespace Ogre;

namespace NFSpace {
    
class PlanetMapTile {
public:
    PlanetMapTile(QuadTreeNode* node, TexturePtr heightTexture, Image heightImage, TexturePtr normalTexture, int size);
    ~PlanetMapTile();
    String getMaterial();    
    Image* getHeightMap();
    const QuadTreeNode* getNode();
    size_t getGPUMemoryUsage();
    void addReference();
    void removeReference();
    int getReferences();

protected:
    void prepareMaterial();

    bool mMaterialCreated;
    
    QuadTreeNode* mNode;
    TexturePtr mHeightTexture;
    Image mHeightImage;
    TexturePtr mNormalTexture;
    MaterialPtr mMaterial;
    int mSize;
    int mReferences;
};

}

#endif
