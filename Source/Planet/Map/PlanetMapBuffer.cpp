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

const unsigned int PlanetMapBuffer::LEVEL_MIN = 0;
const unsigned int PlanetMapBuffer::LEVEL_RANGE = 1;

PlanetMapBuffer::PlanetMapBuffer(SceneManager* sceneManager, Camera* camera, int type, int size, int border, Real fill)
: mSize(size), mBorder(border), mType(type), mFill(fill), mSceneManager(sceneManager), mCamera(camera) {
    assert(isPowerOf2(size - 1));
    mFullSize = mSize + 2 * mBorder;
    init();
}

PlanetMapBuffer::~PlanetMapBuffer() {
    TextureManager::getSingleton().remove(mTexture->getName());
}

void PlanetMapBuffer::init() {
    mTexture = TextureManager::getSingleton().createManual(
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
    mRenderTexture = mTexture->getBuffer()->getRenderTarget();
    mRenderTexture->setAutoUpdated(FALSE);
}

void PlanetMapBuffer::render(int face, int lod, int x, int y, SceneNode* brushes) {
    // Add brushes into the scene.
    SceneNode* node = mSceneManager->getRootSceneNode()->createChildSceneNode();
    node->addChild(brushes);

    //DumpScene(mSceneManager);
    
    // Render each cube face from the scene graph.
    renderTile(face, lod, x, y, true, FBT_COLOUR | FBT_DEPTH);

    // Remove brushes.
    node->removeChild(brushes);
    mSceneManager->getRootSceneNode()->removeChild(node);
    mSceneManager->destroySceneNode(node);
}
    
void PlanetMapBuffer::filter(int face, int lod, int x, int y, int type, PlanetMapBuffer* source) {
    if (type != FILTER_TYPE_NORMAL) return;

    // Prepare texture substitution list.
    AliasTextureNamePairList aliasList;
    aliasList.insert(AliasTextureNamePairList::value_type("source", source->getTextureName()));
    
    // Alter the material to use the height map as its source texture.
    MaterialPtr normalMapperMaterial;
    normalMapperMaterial = MaterialManager::getSingleton().getByName("Planet/NormalMapper");
    normalMapperMaterial->applyTextureAliases(aliasList);
    
    // Create scene node to hold all the renderables.
    SceneNode* filterNode = mSceneManager->getRootSceneNode()->createChildSceneNode();
    
    // Create a filter face (fullscreen quad).
    PlanetFilter *filter = new PlanetFilter(face, lod, x, y, mSize, mBorder);
    filter->setMaterial("Planet/NormalMapper");
    filterNode->attachObject(filter);
    
    renderTile(face, lod, x, y, false, true);

    // Clean-up the renderables and detach them.
    SceneNode::ObjectIterator it = filterNode->getAttachedObjectIterator();
    while (it.hasMoreElements()) {
        delete it.getNext();
    }
    filterNode->detachAllObjects();
    
    // Destroy the scene node.
    mSceneManager->getRootSceneNode()->removeChild(filterNode);
    mSceneManager->destroySceneNode(filterNode);
}
    
TexturePtr PlanetMapBuffer::saveTexture(bool border) {
    // Alloc write once texture at right size
    int size = border ? mFullSize : mSize;
    int edge = border ? 0 : mBorder ;
    TexturePtr texture = TextureManager::getSingleton().createManual(
                                                                     getUniqueId("Tile"), // Name of texture
                                                                     "PlanetMap", // Name of resource group in which the texture should be created
                                                                     TEX_TYPE_2D, // Texture type
                                                                     size, // Width
                                                                     size, // Height
                                                                     1, // Depth (Must be 1 for two dimensional textures)
                                                                     0, // Number of mipmaps
                                                                     getPixelFormat(), // Pixel format
                                                                     TU_STATIC_WRITE_ONLY // usage
                                                                     );
    // Blit current front buffer contents into new texture.
    texture->getBuffer()->blit(mTexture->getBuffer(), 
                               Box(edge, edge, 0, size + edge, size + edge, 1),
                               Box(0, 0, 0, size, size, 1)
                              );
    
    return texture;
}

Image PlanetMapBuffer::saveImage(bool border) {
    // Create system memory buffer to hold pixel data.
    PixelFormat pf = getPixelFormat();
    
    uchar *data = OGRE_ALLOC_T(uchar, mRenderTexture->getWidth() * mRenderTexture->getHeight() * PixelUtil::getNumElemBytes(pf), MEMCATEGORY_GENERAL);
    PixelBox pb(mRenderTexture->getWidth(), mRenderTexture->getHeight(), 1, pf, data);
    
    // Load data and create an image object.
    mRenderTexture->copyContentsToMemory(pb, RenderTarget::FB_AUTO);

    if (mBorder && !border) {
        // Crop image.
        Image uncropped = Image().loadDynamicImage(data, mRenderTexture->getWidth(), mRenderTexture->getHeight(), 1, pf, false, 1, 0);
        Image image = cropImage(uncropped, mBorder, mBorder, mSize, mSize);
        OGRE_FREE(uncropped.getData(), MEMCATEGORY_GENERAL);
        return image;
    }
    else {
        // Return unmodified.
        return Image().loadDynamicImage(data, mRenderTexture->getWidth(), mRenderTexture->getHeight(), 1, pf, false, 1, 0);
    }        
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

void PlanetMapBuffer::renderTile(int face, int lod, int x, int y, bool transform, unsigned int clearFrame) {
    // Ensure viewport is set up correctly.
    while (mRenderTexture->getNumViewports() > 0) {
        mRenderTexture->removeViewport(0);
    }
    mRenderTexture->addViewport(mCamera);
    mRenderTexture->getViewport(0)->setClearEveryFrame((bool)clearFrame, clearFrame);
    mRenderTexture->getViewport(0)->setBackgroundColour(ColourValue(mFill, mFill, mFill, 1));
    mRenderTexture->getViewport(0)->setOverlaysEnabled(false);

    // Reset camera to default behaviour.
    mCamera->setCustomProjectionMatrix(false);

    // Set camera to have a perfect 45deg FOV in the non-border area.
    mCamera->setFOVy(Radian(atan(1) * 2));
    
    if (transform) {
        // Set camera to cube face transformation.
        // Note: cubemap space is left-handed when viewed from inside the cube.
        // Flip the basis vector's x coordinates to compensate.
        Quaternion orientation = PlanetCube::getFaceCamera(face);
        mCamera->setOrientation(orientation);

        // Fetch normal projection matrix
        Ogre::Matrix4 projMatrix;
        projMatrix = mCamera->getProjectionMatrix();
        
        // Focus on an (x, y) subtile of the normalized projective space by scaling + translating
        Ogre::Matrix4 tileSkew = Matrix4(
                                         1 << lod, 0, 0, (1 << lod) - x * 2 - 1,
                                         0, 1 << lod, 0, (1 << lod) - ((1 << lod) - y - 1) * 2 - 1,
                                         0, 0, 1, 0,
                                         0, 0, 0, 1//
                                         );

        // Dilate projection to fit exactly 90 degrees of FOV in the texture without its border.
        Real dilation = mSize / float(mFullSize + 1);
        Ogre::Matrix4 borderDilate = Matrix4(
                                             dilation, 0, 0, 0,
                                             0, dilation, 0, 0,
                                             0, 0, 1, 0,
                                             0, 0, 0, 1//
                                             );
        
        mCamera->setCustomProjectionMatrix(true, borderDilate * tileSkew * projMatrix);
    }
    else {
        // Viewport/source is pre-transformed.
        mCamera->setOrientation(Quaternion(1.f, 0.f, 0.f, 0.f));
    }
    
    
    // Render the frame to the texture.
    mRenderTexture->update();
}

}

