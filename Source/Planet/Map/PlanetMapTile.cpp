/*
 *  PlanetMapTile.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 26/11/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "PlanetMapTile.h"
#include "Utility.h"

namespace NFSpace {
    
    PlanetMapTile::PlanetMapTile(QuadTreeNode* node, TexturePtr heightTexture, Image heightImage, TexturePtr normalTexture, int size) {
        mNode = node;
        
        mHeightTexture = heightTexture;
        mHeightImage   = heightImage;
        mNormalTexture = normalTexture;
        mSize = size;

        PlanetStats::statsTiles++;

        prepareMaterial();
    }
    
    PlanetMapTile::~PlanetMapTile() {
        OGRE_FREE(mHeightImage.getData(), MEMCATEGORY_GENERAL);

        if (mMaterialCreated) {
            MaterialManager::getSingleton().remove(mMaterial->getName());
        }
        TextureManager::getSingleton().remove(mHeightTexture->getName());
        TextureManager::getSingleton().remove(mNormalTexture->getName());

        PlanetStats::statsTiles--;
    }

    String PlanetMapTile::getMaterial() {
        if (!mMaterialCreated) prepareMaterial();
        return mMaterial->getName();//"Planet/Surface";//mMaterial->getName();
    }

    Image* PlanetMapTile::getHeightMap() {
        return &mHeightImage;
    }

    void PlanetMapTile::prepareMaterial() {
        mMaterialCreated = TRUE;

        // Get original planet/surface material and clone it.
        MaterialPtr planetSurface = MaterialManager::getSingleton().getByName("Planet/Surface");
        mMaterial = planetSurface->clone("Planet/Surface/" + getUniqueId(""));
        
        // Prepare texture substitution list.
        AliasTextureNamePairList aliasList;
        aliasList.insert(AliasTextureNamePairList::value_type("heightMap", mHeightTexture->getName()));
        aliasList.insert(AliasTextureNamePairList::value_type("normalMap", mNormalTexture->getName()));
        
        mMaterial->applyTextureAliases(aliasList);
    }
    
    const QuadTreeNode* PlanetMapTile::getNode() {
        return mNode;
    }
    
    size_t PlanetMapTile::getGPUMemoryUsage() {
        return
            mHeightTexture->getWidth() * mHeightTexture->getHeight() * 8 +
            mNormalTexture->getWidth() * mNormalTexture->getHeight() * 8;
    }
    
}
