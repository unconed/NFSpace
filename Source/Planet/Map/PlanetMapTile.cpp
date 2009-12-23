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
        mReferences = 0;
        
        PlanetStats::totalTiles++;

        prepareMaterial();
    }
    
    PlanetMapTile::~PlanetMapTile() {
        OGRE_FREE(mHeightImage.getData(), MEMCATEGORY_GENERAL);

        if (mMaterialCreated) {
            MaterialManager::getSingleton().remove(mMaterial->getName());
        }
        TextureManager::getSingleton().remove(mHeightTexture->getName());
        TextureManager::getSingleton().remove(mNormalTexture->getName());

        PlanetStats::totalTiles--;
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

        // Clear out pass caches between scene managers.
        updateSceneManagersAfterMaterialsChange();
    }
    
    const QuadTreeNode* PlanetMapTile::getNode() {
        return mNode;
    }
    
    size_t PlanetMapTile::getGPUMemoryUsage() {
        return 1.3125 * (
            mHeightTexture->getWidth() * mHeightTexture->getHeight() * Ogre::PixelUtil::getNumElemBytes(mHeightTexture->getFormat()) +
            mNormalTexture->getWidth() * mNormalTexture->getHeight() * Ogre::PixelUtil::getNumElemBytes(mNormalTexture->getFormat()));
    }

    void PlanetMapTile::addReference() {
        mReferences++;
    }
    void PlanetMapTile::removeReference() {
        mReferences--;
    }
    int PlanetMapTile::getReferences() {
        return mReferences;
    }
    
}
