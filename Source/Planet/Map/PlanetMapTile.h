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

using namespace Ogre;

namespace NFSpace {
    
class PlanetMapTile {
public:
    PlanetMapTile(TexturePtr heightTexture, Image heightImage, TexturePtr normalTexture, int size);
    ~PlanetMapTile();
    String getMaterial();    
    Image* getHeightMap();

protected:
    void prepareMaterial();

    bool mMaterialCreated;
    
    TexturePtr mHeightTexture;
    Image mHeightImage;
    TexturePtr mNormalTexture;
    MaterialPtr mMaterial;
    int mSize;
};

}

#endif
