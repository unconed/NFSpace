/*
 *  PlanetRenderable.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 4/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Utility.h"
#include "PlanetRenderable.h"
#include "PlanetCube.h"
#include "EngineState.h"

#include "Ogre/OgreBitwise.h"

using namespace Ogre;

namespace NFSpace {
    
/**
 * Constructor.
 */
PlanetRenderable::PlanetRenderable(QuadTreeNode* node, Image* map)
: mProxy(0), mQuadTreeNode(node), mMap(map), mChildDistance(0), mChildDistanceSquared(0) {
    mPlanetRadius = getReal("planet.radius");
    mPlanetHeight = getReal("planet.height");
    mGridSize = getInt("planet.gridSize");
    
    assert(isPowerOf2(mGridSize - 1));
    assert(isPowerOf2(map->getWidth() - 1));
    assert(isPowerOf2(map->getHeight() - 1));
    
    initialize(RenderOperation::OT_TRIANGLE_LIST, TRUE);
    setMaterial("BaseWhiteNoLighting");
    fillHardwareBuffers();
}

Real PlanetRenderable::getLODDistance() {
    return maxf(mDistance, mChildDistance);
}

void PlanetRenderable::setChildLODDistance(Real lodDistance) {
    mChildDistance = lodDistance;
    mChildDistanceSquared = mChildDistanceSquared;
}

void PlanetRenderable::setProxy(MovableObject *proxy) {
    mProxy = proxy;
}

/** 
 * Get World transform. Overloaded to allow both rendering as literal scenenode as well as virtual renderable for PlanetCube/PlanetMovable.
 */
void PlanetRenderable::getWorldTransforms(Matrix4* xform) const {
    if (mProxy) {
        *xform = mProxy->getParentNode()->_getFullTransform();
    }
    else {
        *xform = m_matWorldTransform * mParentNode->_getFullTransform();
    }
}

Vector3& PlanetRenderable::getCenter() {
    return mCenter;
}
    
Real PlanetRenderable::getBoundingRadius() {
    return mBoundingRadius;
}

/**
 * Creates the vertex declaration.
 */
void PlanetRenderable::createVertexDeclaration() {
    mDeclaration = mRenderOp.vertexData->vertexDeclaration;
    size_t offset = 0;
    mDeclaration->addElement(0, offset, VET_FLOAT3, VES_POSITION);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
    // Cube map coords
    mDeclaration->addElement(0, offset, VET_FLOAT3, VES_TEXTURE_COORDINATES, 0);
    offset += VertexElement::getTypeSize(VET_FLOAT3);    
}

/**
 * Fills the hardware vertex and index buffers with data.
 */
void PlanetRenderable::fillHardwareBuffers() {
    // Examine pixel buffer to identify
    HeightMapPixel* pMap = (HeightMapPixel*)(mMap->getData());
    
    // Allocate enough buffer space.
    int n = mGridSize * mGridSize + mGridSize * 4;
    int i = ((mGridSize - 1) * (mGridSize - 1) + 4 * (mGridSize - 1) ) * 6;
    prepareHardwareBuffers(n, i);

    // Get pointers into buffers.
    const VertexElement* poselem = mDeclaration->findElementBySemantic(VES_POSITION);
    const VertexElement* texelem = mDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES, 0);
    unsigned char* pBase = static_cast<unsigned char*>(mVertexBuffer->lock(HardwareBuffer::HBL_DISCARD));
    unsigned int* pIndex = static_cast<unsigned int*>(mIndexBuffer->lock(HardwareBuffer::HBL_DISCARD));

    // Calculate scales, offsets and steps.
    const Real scale = (Real)(1 << mQuadTreeNode->mLOD);
    const Real invScale = 2.0f / scale;
    const Real positionX = -1.f + invScale * mQuadTreeNode->mX;
    const Real positionY = -1.f + invScale * mQuadTreeNode->mY;
    const int stepX = (mMap->getWidth() - 1) / (1 << mQuadTreeNode->mLOD) / (mGridSize - 1);
    const int stepY = (mMap->getHeight() - 1) / (1 << mQuadTreeNode->mLOD) / (mGridSize - 1);
    const int pixelX = mQuadTreeNode->mX * stepX * (mGridSize - 1);
    const int pixelY = mQuadTreeNode->mY * stepY * (mGridSize - 1);
    const int offsetX = stepX;
    const int offsetY = stepY * mMap->getWidth();
    
    HeightMapPixel* pMapCorner = &pMap[pixelY * mMap->getWidth() + pixelX];
    
    // Keep track of extents.
    Vector3 min = Vector3(1e8), max = Vector3(-1e8);
    mCenter = Vector3(0, 0, 0);

    Matrix3 faceTransform = PlanetCube::getFaceTransform(mQuadTreeNode->mFace, true);

    //#define getOffsetPixel(x) ((float)(((*(pMapRow + (x))) & PlanetMapBuffer::LEVEL_MASK) >> PlanetMapBuffer::LEVEL_SHIFT))
    #define getOffsetPixel(x) Bitwise::halfToFloat(*((unsigned short*)(pMapRow + (x))))
    //#define getOffsetPixel(x) (*((float*)(pMapRow + (x))))
    
    // Lossy representation of heightmap
    const int offsetX2 = offsetX * 2;
    const int offsetY2 = offsetY * 2;
    Real diff = 0;
    for (int j = 0; j < (mGridSize - 1); j += 2) {
        HeightMapPixel* pMapRow = &pMap[(pixelY + j * stepY) * mMap->getWidth() + pixelX];
        for (int i = 0; i < (mGridSize - 1); i += 2) {
            // dx
            diff = maxf(diff, fabs((getOffsetPixel(0) + getOffsetPixel(offsetX2)) / 2.0f - getOffsetPixel(offsetX)));
            diff = maxf(diff, fabs((getOffsetPixel(offsetY2) + getOffsetPixel(offsetY2 + offsetX2)) / 2.0f - getOffsetPixel(offsetY + offsetX)));
            // dy
            diff = maxf(diff, fabs((getOffsetPixel(0) + getOffsetPixel(offsetY2)) / 2.0f - getOffsetPixel(offsetY)));
            diff = maxf(diff, fabs((getOffsetPixel(offsetX2) + getOffsetPixel(offsetX2 + offsetY2)) / 2.0f - getOffsetPixel(offsetX + offsetY)));
            // diag
            diff = maxf(diff, fabs((getOffsetPixel(offsetX2) + getOffsetPixel(offsetY2)) / 2.0f - getOffsetPixel(offsetY + offsetX)));
            
            pMapRow += offsetX2;
        }
    }
    mLODDifference = diff / PlanetMapBuffer::LEVEL_RANGE;

    // Calculate LOD error of sphere.
    Real angle = Math::PI / (mGridSize << maxi(0, mQuadTreeNode->mLOD - 1));
    Real sphereError = (1 - cos(angle)) * 1.4f * mPlanetRadius;
    if (mPlanetHeight) {
        mLODDifference += sphereError / mPlanetHeight;
        // Convert to world units.
        mDistance = mLODDifference * mPlanetHeight;
    }
    else {
        mDistance = sphereError;
    }
    
    // Cache square.
    mDistanceSquared = mDistance * mDistance;
    
    srand(2134);
    
    //#define getPixel() ((((float)(((*(pMapRow)) & PlanetMapBuffer::LEVEL_MASK) >> PlanetMapBuffer::LEVEL_SHIFT)) - PlanetMapBuffer::LEVEL_MIN) / PlanetMapBuffer::LEVEL_RANGE)
    #define getPixel() ((Bitwise::halfToFloat(*((unsigned short*)(pMapRow))) - PlanetMapBuffer::LEVEL_MIN) / PlanetMapBuffer::LEVEL_RANGE)
    //#define getPixel() (((*((float*)(pMapRow))) - PlanetMapBuffer::LEVEL_MIN) / PlanetMapBuffer::LEVEL_RANGE)
    
    // (nVidia cards) cubemapping fills in the edge/corner texels of cubemaps with 0.5 pixel on both sides.
    float uvCorrection = (PlanetMap::PLANET_TEXTURE_SIZE - 1.0f) / PlanetMap::PLANET_TEXTURE_SIZE;
    
    // Output vertex data for regular grid.
    for (int j = 0; j < mGridSize; j++) {
        HeightMapPixel* pMapRow = pMapCorner + j * offsetY;
        for (int i = 0; i < mGridSize; i++) {
            float* pPos;
            float* pTex;
            poselem->baseVertexPointerToElement(pBase, &pPos);
            texelem->baseVertexPointerToElement(pBase, &pTex);

            Real height = getPixel();
            Real x = (float) i / (float) (mGridSize - 1);
            Real y = (float) j / (float) (mGridSize - 1);
            
            Vector3 spherePoint(x * invScale + positionX, y * invScale + positionY, 1);
            spherePoint.normalise();

            Vector3 texturePoint = spherePoint;
            texturePoint.x *= uvCorrection;
            texturePoint.y *= uvCorrection;
            texturePoint = faceTransform * texturePoint;
            *pTex++ = texturePoint.x;
            *pTex++ = texturePoint.y;
            *pTex++ = texturePoint.z;
            
            spherePoint = faceTransform * spherePoint;
            spherePoint *= mPlanetRadius + height * mPlanetHeight;
            *pPos++ = spherePoint.x;
            *pPos++ = spherePoint.y;
            *pPos++ = spherePoint.z;

            mCenter += spherePoint;

            if (spherePoint.x < min.x) {
                min.x = spherePoint.x;
            }
            if (spherePoint.y < min.y) {
                min.y = spherePoint.y;
            }
            if (spherePoint.z < min.z) {
                min.z = spherePoint.z;
            }
            if (spherePoint.x > max.x) {
                max.x = spherePoint.x;
            }
            if (spherePoint.y > max.y) {
                max.y = spherePoint.y;
            }
            if (spherePoint.z > max.z) {
                max.z = spherePoint.z;
            }
            
            pBase += mVertexBuffer->getVertexSize();
            pMapRow += offsetX;
        }
    }
    // Output vertex data for x skirts.
    for (int j = 0; j < mGridSize; j += (mGridSize - 1)) {
        HeightMapPixel* pMapRow = pMapCorner + j * offsetY;
        for (int i = 0; i < mGridSize; i++) {
            float* pPos;
            float* pTex;
            poselem->baseVertexPointerToElement(pBase, &pPos);
            texelem->baseVertexPointerToElement(pBase, &pTex);
            
            Real height = getPixel();
            Real x = (float) i / (float) (mGridSize - 1);
            Real y = (float) j / (float) (mGridSize - 1);
            
            Vector3 spherePoint(x * invScale + positionX, y * invScale + positionY, 1);
            spherePoint.normalise();
            spherePoint = faceTransform * spherePoint;
            
            *pTex++ = spherePoint.x;
            *pTex++ = spherePoint.y;
            *pTex++ = spherePoint.z;
            
            spherePoint *= mPlanetRadius + (height - mLODDifference * 2) * mPlanetHeight;
            
            mCenter += spherePoint;
            
            *pPos++ = spherePoint.x;
            *pPos++ = spherePoint.y;
            *pPos++ = spherePoint.z;
            
            if (spherePoint.x < min.x) {
                min.x = spherePoint.x;
            }
            if (spherePoint.y < min.y) {
                min.y = spherePoint.y;
            }
            if (spherePoint.z < min.z) {
                min.z = spherePoint.z;
            }
            if (spherePoint.x > max.x) {
                max.x = spherePoint.x;
            }
            if (spherePoint.y > max.y) {
                max.y = spherePoint.y;
            }
            if (spherePoint.z > max.z) {
                max.z = spherePoint.z;
            }
            
            pBase += mVertexBuffer->getVertexSize();
            pMapRow += offsetX;
        }
    }
    // Output vertex data for y skirts.
    for (int i = 0; i < mGridSize; i += (mGridSize - 1)) {
        HeightMapPixel* pMapRow = pMapCorner + i * offsetX;
        for (int j = 0; j < mGridSize; j++) {
            float* pPos;
            float* pTex;
            poselem->baseVertexPointerToElement(pBase, &pPos);
            texelem->baseVertexPointerToElement(pBase, &pTex);
            
            Real height = getPixel();
            Real x = (float) i / (float) (mGridSize - 1);
            Real y = (float) j / (float) (mGridSize - 1);
            
            Vector3 spherePoint(x * invScale + positionX, y * invScale + positionY, 1);
            spherePoint.normalise();
            spherePoint = faceTransform * spherePoint;
            
            *pTex++ = spherePoint.x;
            *pTex++ = spherePoint.y;
            *pTex++ = spherePoint.z;
            
            spherePoint *= mPlanetRadius + (height - mLODDifference * 2) * mPlanetHeight;
            
            mCenter += spherePoint;
            
            *pPos++ = spherePoint.x;
            *pPos++ = spherePoint.y;
            *pPos++ = spherePoint.z;
            
            if (spherePoint.x < min.x) {
                min.x = spherePoint.x;
            }
            if (spherePoint.y < min.y) {
                min.y = spherePoint.y;
            }
            if (spherePoint.z < min.z) {
                min.z = spherePoint.z;
            }
            if (spherePoint.x > max.x) {
                max.x = spherePoint.x;
            }
            if (spherePoint.y > max.y) {
                max.y = spherePoint.y;
            }
            if (spherePoint.z > max.z) {
                max.z = spherePoint.z;
            }
            
            pBase += mVertexBuffer->getVertexSize();
            pMapRow += offsetY;
        }
    }
    
    // Calculate center.
    mSurfaceNormal = mCenter /= n;
    mSurfaceNormal.normalise();

    int index = 0, skirtIndex = 0;
    // Output indices for regular surface.
    for (int j = 0; j < (mGridSize - 1); j++) {
        for (int i = 0; i < (mGridSize - 1); i++) {
            *pIndex++ = index;
            *pIndex++ = index + mGridSize;
            *pIndex++ = index + 1;

            *pIndex++ = index + mGridSize;
            *pIndex++ = index + mGridSize + 1;
            *pIndex++ = index + 1;
            
            index++;
        }
        index++;
    }
    // Output indices for x skirts.
    index = 0;
    skirtIndex = mGridSize * mGridSize;
    for (int i = 0; i < (mGridSize - 1); i++) {
        *pIndex++ = index;
        *pIndex++ = index + 1;
        *pIndex++ = skirtIndex;
        
        *pIndex++ = skirtIndex;
        *pIndex++ = index + 1;
        *pIndex++ = skirtIndex + 1;
        
        index++;
        skirtIndex++;
    }
    index = mGridSize * (mGridSize - 1);
    skirtIndex = mGridSize * (mGridSize + 1);
    for (int i = 0; i < (mGridSize - 1); i++) {
        *pIndex++ = index;
        *pIndex++ = skirtIndex;
        *pIndex++ = index + 1;
        
        *pIndex++ = skirtIndex;
        *pIndex++ = skirtIndex + 1;
        *pIndex++ = index + 1;
        
        index++;
        skirtIndex++;
    }
    // Output indices for y skirts.
    index = 0;
    skirtIndex = mGridSize * (mGridSize + 2);
    for (int i = 0; i < (mGridSize - 1); i++) {
        *pIndex++ = index;
        *pIndex++ = skirtIndex;
        *pIndex++ = index + mGridSize;
        
        *pIndex++ = skirtIndex;
        *pIndex++ = skirtIndex + 1;
        *pIndex++ = index + mGridSize;
        
        index += mGridSize;
        skirtIndex++;
    }
    index = (mGridSize - 1);
    skirtIndex = mGridSize * (mGridSize + 3);
    for (int i = 0; i < (mGridSize - 1); i++) {
        *pIndex++ = index;
        *pIndex++ = index + mGridSize;
        *pIndex++ = skirtIndex;
        
        *pIndex++ = skirtIndex;
        *pIndex++ = index + mGridSize;
        *pIndex++ = skirtIndex + 1;
        
        index += mGridSize;
        skirtIndex++;
    }
    
    // Set bounding box/radius.
    setBoundingBox(AxisAlignedBox(min, max));
    mBoundingRadius = ((max - min).length() / 2);

    // Release buffers.
    mIndexBuffer->unlock();
    mVertexBuffer->unlock();
        
}

bool PlanetRenderable::preRender(SceneManager* sm, RenderSystem* rsys) {
    return true;
}

void PlanetRenderable::postRender(SceneManager* sm, RenderSystem* rsys) {
}

void PlanetRenderable::_updateRenderQueue(RenderQueue* queue) {
    SimpleRenderable::_updateRenderQueue(queue);
}

const String& PlanetRenderable::getMovableType(void) const {
    static String movType = "PlanetRenderable";
    return movType;
}

void PlanetRenderable::setFrameOfReference(SimpleFrustum& frustum, Vector3 cameraPosition, Vector3 cameraPlane, Real sphereClip, Real lodDetailFactorSquared) {
    // Bounding box clipping.
    Sphere boundingSphere = Sphere(mCenter, mBoundingRadius);
    if (!frustum.isVisible(&boundingSphere)) {
        mIsClipped = true;
        return;
    }

    // Get vector from center to camera and normalize it.
    Vector3 positionOffset = cameraPosition - mCenter;
    Vector3 viewDirection = positionOffset;
    viewDirection.normalise();
    
    // Find the offset between the center of the grid and the point furthest away from the camera (rough approx).
    Vector3 referenceOffset = (viewDirection - mSurfaceNormal.dotProduct(viewDirection) * mSurfaceNormal) * (mPlanetRadius / (1 << mQuadTreeNode->mLOD));

    // Spherical distance map clipping.
    Vector3 referenceCoordinate = mCenter + referenceOffset;
    referenceCoordinate.normalise();
    if (cameraPlane.dotProduct(referenceCoordinate) < sphereClip) {
        mIsClipped = true;
        return;
    }
    
    mIsClipped = false;
    
    // Now find the closet point to the camera (approx).
    positionOffset -= referenceOffset;
    
    // Determine factor to shrink LOD by due to perspective foreshortening.
    // Pad shortening factor based on LOD level to compensate for grid curving.
    Real lodSpan = mPlanetRadius / ((1 << mQuadTreeNode->mLOD) * positionOffset.length());
    Real lodShorten = minf(1.0f, mSurfaceNormal.crossProduct(viewDirection).length() + lodSpan);
    Real lodShortenSquared = lodShorten * lodShorten;
    mIsInLODRange = positionOffset.squaredLength() > maxf(mDistanceSquared, mChildDistanceSquared) * lodDetailFactorSquared * lodShortenSquared;
}

const bool PlanetRenderable::isClipped() const {
    return mIsClipped;
}


const bool PlanetRenderable::isInLODRange() const {
    return mIsInLODRange;
}

};