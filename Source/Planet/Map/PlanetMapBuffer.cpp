/*
 *  PlanetMapBuffer.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 29/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "PlanetMapBuffer.h"
#include "PlanetCube.h"

#include "Application.h"
#include "Utility.h"
#include "Planet.h"
#include "PlanetEdgeFixup.h"

namespace NFSpace {

const unsigned int PlanetMapBuffer::LEVEL_SHIFT = 0;
const unsigned int PlanetMapBuffer::LEVEL_MASK = 0xFFFF;
const unsigned int PlanetMapBuffer::LEVEL_MIN = 0;
const unsigned int PlanetMapBuffer::LEVEL_MID = 0x8000;
const unsigned int PlanetMapBuffer::LEVEL_MAX = 0xFFFF;
const unsigned int PlanetMapBuffer::LEVEL_RANGE = 1;
    
PlanetMapBuffer::PlanetMapBuffer(SceneManager* sceneManager, Camera* camera, int type, int size, int border, Real fill)
: mSize(size), mBorder(border), mType(type), mFill(fill), mSceneManager(sceneManager), mCamera(camera), mMaterialCreated(false) {
    assert(isPowerOf2(size - 1));
    mFullSize = mSize + 2 * mBorder;
    init();
}

PlanetMapBuffer::~PlanetMapBuffer() {
    if (mBorder) {
        // Dilated cubemap
        for (int i = 0; i < 6; ++i) {
            TextureManager::getSingleton().remove(mTextureFaces[i]->getName());
        }
    }
    else {
        TextureManager::getSingleton().remove(mTexture->getName());
    }
    
    if (mMaterialCreated) {
        MaterialManager::getSingleton().remove(mMaterial->getName());
    }
}

void PlanetMapBuffer::init() {
    if (mBorder) {
        // Dilated cubemap
        for (int i = 0; i < 6; ++i) {
            mTextureFaces[i] = TextureManager::getSingleton().createManual(
                                                                   getUniqueId("Buffer"), // Name of texture
                                                                   "PlanetMap", // Name of resource group in which the texture should be created
                                                                   TEX_TYPE_2D, // Texture type
                                                                   mFullSize, // Width
                                                                   mFullSize, // Height
                                                                   1, // Depth (Must be 1 for two dimensional textures)
                                                                   0, // Number of mipmaps
                                                                   getPixelFormat(), // Pixel format
                                                                   TU_RENDERTARGET // usage
                                                                   );
        }
        for (int i = 0; i < 6; ++i) {
            mRenderTexture[i] = mTextureFaces[i]->getBuffer()->getRenderTarget();
            mRenderTexture[i]->setAutoUpdated(FALSE);
        }
    }
    else {
        // Regular cubemap
        mTexture = TextureManager::getSingleton().createManual(
                                                               getUniqueId("Buffer"), // Name of texture
                                                               "PlanetMap", // Name of resource group in which the texture should be created
                                                               TEX_TYPE_CUBE_MAP, // Texture type
                                                               mFullSize, // Width
                                                               mFullSize, // Height
                                                               1, // Depth (Must be 1 for two dimensional textures)
                                                               0, // Number of mipmaps
                                                               getPixelFormat(), // Pixel format
                                                               TU_RENDERTARGET // usage
                                                               );
        for (int i = 0; i < 6; ++i) {
            mRenderTexture[i] = mTexture->getBuffer(i)->getRenderTarget();
            mRenderTexture[i]->setAutoUpdated(FALSE);
        }
    }
}

void PlanetMapBuffer::render(SceneNode* brushes) {
    // Add brushes into the scene.
    SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode();
    node->addChild(brushes);
    
    //DumpScene(mSceneManager);
    
    // Render each cube face from the scene graph.
    for (int i = 0; i < 6; ++i) {
        renderFace(i, true, FBT_COLOUR | FBT_DEPTH);
    }
    
    // Remove brushes.
    node->removeChild(brushes);
    mSceneManager->getRootSceneNode()->removeChild(node);
    mSceneManager->destroySceneNode(node);
    
    edgeFixup();
    prepareMaterial();
}
    
void PlanetMapBuffer::renderFace(int face, bool transform, unsigned int clearFrame) {
    // Set camera to have a perfect 45deg FOV in the non-border area.
    mCamera->setFOVy(Radian(atan(float(mFullSize + 1) / (mSize)) * 2));
    while (mRenderTexture[face]->getNumViewports() > 0) {
        mRenderTexture[face]->removeViewport(0);
    }
    mRenderTexture[face]->addViewport(mCamera);
    mRenderTexture[face]->getViewport(0)->setClearEveryFrame((bool)clearFrame, clearFrame);
    mRenderTexture[face]->getViewport(0)->setBackgroundColour(ColourValue(mFill, mFill, mFill, 1));
    mRenderTexture[face]->getViewport(0)->setOverlaysEnabled(false);
    
    // Set camera to cube face transformation.
    // Note: cubemap space is left-handed when viewed from inside the cube.
    // Flip the basis vector's x coordinates to compensate.
    if (transform) {
        Matrix3 faceMatrix = PlanetCube::getFaceTransform(face, false);
        Quaternion orientation = Quaternion(faceMatrix);
        mCamera->setOrientation(orientation);
    }
    else {
        mCamera->setOrientation(Quaternion(1.f, 0.f, 0.f, 0.f));
    }
    // Render the frame to the texture.
    mRenderTexture[face]->update();
}
    
void PlanetMapBuffer::edgeFixup() {
    if (mBorder) {
        // Prepare materials for rendering the cube face edges.
        for (int i = 0; i < 6; ++i) {
            mMaterialFaces[i] = MaterialManager::getSingleton().create(
                                                              mTextureFaces[i]->getName(), // name
                                                              "PlanetMap");

            Pass* pass = mMaterialFaces[i]->getTechnique(0)->getPass(0);
            pass->setCullingMode(CULL_NONE);
            pass->setSceneBlending(SBT_ADD);
            pass->setDepthCheckEnabled(true);
            pass->setDepthWriteEnabled(true);

            TextureUnitState *textureUnit = pass->createTextureUnitState(mTexture->getName());
            textureUnit->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
            textureUnit->setTextureFiltering(TFO_NONE);
        }
        
        // Add fix-up node.
        SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode();
        
        // Fix up each cube face using fixup edges in the scene graph.
        for (int i = 0; i < 6; ++i) {
            PlanetEdgeFixup* edgeFixup[4];
            for (int j = 0; j < 4; ++j) {
                edgeFixup[j] = new PlanetEdgeFixup(i, j, mSize, mBorder, PlanetEdgeFixup::FIX_BORDER, mMaterialFaces);
                node->attachObject(edgeFixup[j]);
            }
            
            // Update face
            renderFace(i, false, false);

            // Clean-up
            for (int j = 0; j < 4; ++j) {
                node->detachObject(edgeFixup[j]);
                delete edgeFixup[j];
            }

        }
        
        // Clean-up
        mSceneManager->getRootSceneNode()->removeChild(node);
        mSceneManager->destroySceneNode(node);
    }

}
    
void PlanetMapBuffer::prepareMaterial() {
    mMaterialCreated = TRUE;
    
    // Prepare material for cube mapping.
    mMaterial = MaterialManager::getSingleton().create(
                                                               mTexture->getName() + "Material", // name
                                                               "PlanetMap");
    Pass* pass = mMaterial->getTechnique(0)->getPass(0);
    pass->setSceneBlending(SBT_REPLACE);
    pass->setCullingMode(CULL_CLOCKWISE);
    pass->setLightingEnabled(FALSE);
    pass->setDepthWriteEnabled(TRUE);
    pass->setDepthCheckEnabled(TRUE);
    
    TextureUnitState* textureUnit = pass->createTextureUnitState();
    textureUnit->setTextureName(mTexture->getName(), TEX_TYPE_CUBE_MAP);
    textureUnit->setColourOperation(LBO_REPLACE);
    textureUnit->setTextureFiltering(FO_LINEAR, FO_LINEAR, FO_NONE);
    textureUnit->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
}

void PlanetMapBuffer::save(bool border, bool file) {
    std::string names[] = {
        "rt",
        "lf",
        "up",
        "dn",
        "fr",
        "bk",
    };
    
    for (int i = 0; i < 6; ++i) {
        // Create system memory buffer to hold pixel data.
        PixelFormat pf = getPixelFormat();

        uchar *data = OGRE_ALLOC_T(uchar, mRenderTexture[i]->getWidth() * mRenderTexture[i]->getHeight() * PixelUtil::getNumElemBytes(pf), MEMCATEGORY_GENERAL);
        PixelBox pb(mRenderTexture[i]->getWidth(), mRenderTexture[i]->getHeight(), 1, pf, data);
        
        // Load data and create an image object.
        mRenderTexture[i]->copyContentsToMemory(pb, RenderTarget::FB_AUTO);
        mImage[i] = Image().loadDynamicImage(data, mRenderTexture[i]->getWidth(), mRenderTexture[i]->getHeight(), 1, pf, true, 1, 0);

        // Save as file
        if (file) {
            mRenderTexture[i]->writeContentsToFile("cube-" + mTexture->getName() + "-" + names[i] + ".png");
        }
        
        // Crop image if needed.
        if (mBorder && !border) {
            Image cropped;
            cropped = cropImage(mImage[i], mBorder, mBorder, mSize, mSize);
            OGRE_FREE(mImage[i].getData(), MEMCATEGORY_RENDERSYS);
            mImage[i] = cropped;
        }
    }
}

Image* PlanetMapBuffer::getFace(int face) {
    return &mImage[face];
}

std::string PlanetMapBuffer::getMaterial() {
    return mMaterial->getName();//"Planet/Surface";//mMaterial->getName();
}

PixelFormat PlanetMapBuffer::getPixelFormat() {
    switch (mType) {
        default:
        case MAP_TYPE_MONO:
            return PF_FLOAT16_RGBA;
        case MAP_TYPE_NORMAL:
            return PF_FLOAT16_RGBA;
    }
}

String PlanetMapBuffer::getTextureName() {
    return mTexture->getName();
}

}

