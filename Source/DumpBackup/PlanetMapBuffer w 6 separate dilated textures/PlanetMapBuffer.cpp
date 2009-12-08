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
    
const HeightMapPixel PlanetMapBuffer::LEVEL_MIN = 0x0;
const HeightMapPixel PlanetMapBuffer::LEVEL_MID = 0x8000;
const HeightMapPixel PlanetMapBuffer::LEVEL_MAX = 0xFFFF;
const HeightMapPixel PlanetMapBuffer::LEVEL_RANGE = 0xFFFF;
    
PlanetMapBuffer::PlanetMapBuffer(SceneManager* sceneManager, Camera* camera, int type, int size, int border, HeightMapPixel fill)
: mSize(size), mBorder(border), mType(type), mSceneManager(sceneManager), mCamera(camera) {
    mFill = (float(fill) - LEVEL_MIN) / LEVEL_RANGE;
    assert(isPowerOf2(size - 1));
    mFullSize = mSize + 2 * mBorder;
    init();
}

PlanetMapBuffer::~PlanetMapBuffer() {
    for (int i = 0; i < 6; ++i) {
        if (mImage[i].getData()) {
            OGRE_FREE(mImage[i].getData(), MEMCATEGORY_RENDERSYS);
        }
    }
}

void PlanetMapBuffer::init() {
    for (int i = 0; i < 6; ++i) {
        mTexture[i] = TextureManager::getSingleton().createManual(
                                                                     getUniqueId("Buffer"), // Name of texture
                                                                     "PlanetMap", // Name of resource group in which the texture should be created
                                                                     TEX_TYPE_2D, // Texture type
                                                                     mFullSize, // Width
                                                                     mFullSize, // Height
                                                                     1, // Depth (Must be 1 for two dimensional textures)
                                                                     0, // Number of mipmaps
                                                                     PF_L16, // Pixel format
                                                                     TU_RENDERTARGET // usage
                                                                     );
        
        mRenderTexture[i] = mTexture[i]->getBuffer()->getRenderTarget();
    }
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
    
void PlanetMapBuffer::render(SceneNode* brushes) {
    // Add brushes into the scene.
    SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode();
    node->addChild(brushes);
    
    DumpScene(mSceneManager);
    
    // Render each cube face from the scene graph.
    for (int i = 0; i < 6; ++i) {
        renderFace(mSceneManager, mCamera, i, true, FBT_COLOUR | FBT_DEPTH);
    }
    
    // Remove brushes.
    node->removeChild((short unsigned int)(0));
    delete node;
    
    edgeFixup();
}
    
void PlanetMapBuffer::edgeFixup() {
    // Prepare materials for rendering the cube face edges.
    MaterialPtr materialList[6];
    for (int i = 0; i < 6; ++i) {
        materialList[i] = MaterialManager::getSingleton().create(
                                                          mTexture[i]->getName(), // name
                                                          "PlanetMaterial");

        Pass* pass = materialList[i]->getTechnique(0)->getPass(0);
        pass->setCullingMode(CULL_NONE);
        pass->setSceneBlending(SBT_REPLACE);
        pass->setDepthCheckEnabled(true);
        pass->setDepthWriteEnabled(true);

        TextureUnitState *textureUnit = pass->createTextureUnitState(mTexture[i]->getName());
        textureUnit->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
        textureUnit->setTextureFiltering(TFO_NONE);
    }
    
    // Add fix-up node.
    SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode();
    
    // Fix up each cube face using fixup edges in the scene graph.
    for (int i = 0; i < 6; ++i) {
        PlanetEdgeFixup* edgeFixup[4];
        for (int j = 0; j < 4; ++j) {
            edgeFixup[j] = new PlanetEdgeFixup(i, j, mSize, mBorder, PlanetEdgeFixup::FIX_BORDER, materialList);
            node->attachObject(edgeFixup[j]);
        }
        
        // Update face
        renderFace(i, false, false);

        // Clean-up
        node->detachAllObjects();
        for (int j = 0; j < 4; ++j) {
            delete edgeFixup[j];
        }

    }
    
    // Clean-up
    delete node;
}
    
void PlanetMapBuffer::save(bool border) {
    std::string names[] = {
        "right",
        "left",
        "top",
        "bottom",
        "front",
        "back",
    };
    
    for (int i = 0; i < 6; ++i) {
        // Create system memory buffer to hold pixel data.
        PixelFormat pf = PF_L16;
        uchar *data = OGRE_ALLOC_T(uchar, mRenderTexture[i]->getWidth() * mRenderTexture[i]->getHeight() * PixelUtil::getNumElemBytes(pf), MEMCATEGORY_RENDERSYS);
        PixelBox pb(mRenderTexture[i]->getWidth(), mRenderTexture[i]->getHeight(), 1, pf, data);
        
        // Load data and create an image object.
        mRenderTexture[i]->copyContentsToMemory(pb, RenderTarget::FB_AUTO);
        mImage[i] = Image().loadDynamicImage(data, mRenderTexture[i]->getWidth(), mRenderTexture[i]->getHeight(), 1, pf, false, 1, 0);
        
        // Save as file
        mRenderTexture[i]->writeContentsToFile("cube-" + names[i] + ".png");
    
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
    
int PlanetMapBuffer::getPixelFormat() {
    switch (mType) {
        case MAP_TYPE_MONO:
            return PF_L16;
        case MAP_TYPE_NORMAL:
    }
}

}

