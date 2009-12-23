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

int PlanetRenderable::sInstances = 0;
VertexData* PlanetRenderable::sVertexData;
IndexData* PlanetRenderable::sIndexData;
HardwareVertexBufferSharedPtr PlanetRenderable::sVertexBuffer;
HardwareIndexBufferSharedPtr PlanetRenderable::sIndexBuffer;
int PlanetRenderable::sVertexBufferCapacity;
int PlanetRenderable::sIndexBufferCapacity;
int PlanetRenderable::sGridSize = 0; 
VertexDeclaration *PlanetRenderable::sVertexDeclaration;

/**
 * Constructor.
 */
PlanetRenderable::PlanetRenderable(QuadTreeNode* node, PlanetMapTile* mapTile)
: mProxy(0), mQuadTreeNode(node), mMapTile(mapTile), mChildDistance(0), mChildDistanceSquared(0), mWireBoundingBox(0)
{
    mMap = mMapTile->getHeightMap();
    
    mPlanetRadius = getReal("planet.radius");
    mPlanetHeight = getReal("planet.height");
    
    assert(isPowerOf2(mMap->getWidth() - 1));
    assert(isPowerOf2(mMap->getHeight() - 1));

    addInstance();

    mRenderOp.operationType = RenderOperation::OT_TRIANGLE_LIST;
    mRenderOp.useIndexes = TRUE;
    mRenderOp.vertexData = sVertexData;
    mRenderOp.indexData = sIndexData;

    PlanetStats::totalRenderables++;

    setMaterial("BaseWhiteNoLighting");
    setMaterial(mMapTile->getMaterial());
    fillHardwareBuffers();
    analyseTerrain();
    initDisplacementMapping();
}
    
PlanetRenderable::~PlanetRenderable() {
    removeInstance();
    if (mWireBoundingBox) {
        OGRE_DELETE mWireBoundingBox;
    }

    PlanetStats::totalRenderables--;
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

void PlanetRenderable::addInstance() {
    if (!sInstances++) {
        sGridSize = getInt("planet.gridSize");
        assert(isPowerOf2(sGridSize - 1));

        sVertexData = new VertexData;
        sIndexData = new IndexData;    

        createVertexDeclaration();
        fillHardwareBuffers();
    }
}

void PlanetRenderable::removeInstance() {
    if (!--sInstances) {
        delete sVertexData;
    }
}

/**
 * Set up the parameters for the vertex shader warp / displacement mapping.
 */
void PlanetRenderable::initDisplacementMapping() {
    // Calculate scales, offsets for tile position on cube face.
    const Real invScale = 2.0f / (1 << mQuadTreeNode->mLOD);
    const Real positionX = -1.f + invScale * mQuadTreeNode->mX;
    const Real positionY = -1.f + invScale * mQuadTreeNode->mY;

    // Correct for GL texture mapping at borders.
    const Real uvCorrection = .05f / (mMap->getWidth() + 1);

    // Calculate scales, offset for tile position in map tile.
    int relativeLOD = mQuadTreeNode->mLOD - mMapTile->getNode()->mLOD;
    const Real invTexScale = 1.0f / (1 << relativeLOD) * (1.0f - uvCorrection);
    const Real textureX = invTexScale * (mQuadTreeNode->mX - (mMapTile->getNode()->mX << relativeLOD)) + uvCorrection;
    const Real textureY = invTexScale * (mQuadTreeNode->mY - (mMapTile->getNode()->mY << relativeLOD)) + uvCorrection;

    
    setCustomParameter(1, Vector4(invScale, invScale, invTexScale, invTexScale));
    setCustomParameter(2, Vector4(positionX, positionY, textureX, textureY));
    setCustomParameter(3, Vector4(mPlanetRadius, 0, 0, 0));
    setCustomParameter(4, Vector4(mPlanetHeight, 0, 0, 0));
    setCustomParameter(5, Vector4(mDistance, 0, 0, 0));
    
    // Pass in face transform as 3 vectors :/
    Matrix3 faceTransform = PlanetCube::getFaceTransform(mQuadTreeNode->mFace);
    setCustomParameter(6, Vector4(faceTransform[0][0], faceTransform[1][0], faceTransform[2][0], 0));
    setCustomParameter(7, Vector4(faceTransform[0][1], faceTransform[1][1], faceTransform[2][1], 0));
    setCustomParameter(8, Vector4(faceTransform[0][2], faceTransform[1][2], faceTransform[2][2], 0));

    // Set color/tint
    double r = cos(mMapTile->getNode()->mLOD * 0.7) * .35 + .85;
    double g = cos(mMapTile->getNode()->mLOD * 1.71) * .35 + .85;
    double b = cos(mMapTile->getNode()->mLOD * 2.64) * .35 + .85;
//    setCustomParameter(9, Vector4(r, g, b, 1));
    setCustomParameter(9, Vector4(1, 1, 1, 1));

    // Calculate area of tile relative to even face division.
    mScaleFactor = sqrt(1.0 / (positionX * positionX + 1) / (positionY * positionY + 1));
    
}

/**
 * Analyse the terrain for this tile.
 */
void PlanetRenderable::analyseTerrain() {
    // Examine pixel buffer to identify
    HeightMapPixel* pMap = (HeightMapPixel*)(mMap->getData());
    
    // Calculate scales, offsets for tile position on cube face.
    const Real invScale = 2.0f / (1 << mQuadTreeNode->mLOD);
    const Real positionX = -1.f + invScale * mQuadTreeNode->mX;
    const Real positionY = -1.f + invScale * mQuadTreeNode->mY;

    // Calculate scales, offset for tile position in map tile.
    int relativeLOD = mQuadTreeNode->mLOD - mMapTile->getNode()->mLOD;
    const int relativeX = (mQuadTreeNode->mX - (mMapTile->getNode()->mX << relativeLOD));
    const int relativeY = (mQuadTreeNode->mY - (mMapTile->getNode()->mY << relativeLOD));
    
    // Calculate offsets and strides for the tile's heightmap buffer.
    const int stepX = (mMap->getWidth() - 1) / (1 << relativeLOD) / (sGridSize - 1);
    const int stepY = (mMap->getHeight() - 1) / (1 << relativeLOD) / (sGridSize - 1);
    const int pixelX = relativeX * stepX * (sGridSize - 1);
    const int pixelY = relativeY * stepY * (sGridSize - 1);
    const int offsetX = stepX;
    const int offsetY = stepY * mMap->getWidth();

    // Index into heightmap data.
    HeightMapPixel* pMapCorner = &pMap[pixelY * mMap->getWidth() + pixelX];
    
    // Keep track of extents.
    Vector3 min = Vector3(1e8), max = Vector3(-1e8);
    mCenter = Vector3(0, 0, 0);
    
    Matrix3 faceTransform = PlanetCube::getFaceTransform(mQuadTreeNode->mFace);
    
    //#define getOffsetPixel(x) ((float)(((*(pMapRow + (x))) & PlanetMapBuffer::LEVEL_MASK) >> PlanetMapBuffer::LEVEL_SHIFT))
    #define getOffsetPixel(x) Bitwise::halfToFloat(*((unsigned short*)(pMapRow + (x))))
    //#define getOffsetPixel(x) (*((float*)(pMapRow + (x))))
    
    // Lossy representation of heightmap
    const int offsetX2 = offsetX * 2;
    const int offsetY2 = offsetY * 2;
    Real diff = 0;
    for (int j = 0; j < (sGridSize - 1); j += 2) {
        HeightMapPixel* pMapRow = &pMap[(pixelY + j * stepY) * mMap->getWidth() + pixelX];
        for (int i = 0; i < (sGridSize - 1); i += 2) {
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
    Real angle = Math::PI / (sGridSize << maxi(0, mQuadTreeNode->mLOD - 1));
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
    
    // Process vertex data for regular grid.
    for (int j = 0; j < sGridSize; j++) {
        HeightMapPixel* pMapRow = pMapCorner + j * offsetY;
        for (int i = 0; i < sGridSize; i++) {
            Real height = getPixel();
            Real x = (float) i / (float) (sGridSize - 1);
            Real y = (float) j / (float) (sGridSize - 1);
            
            Vector3 spherePoint(x * invScale + positionX, y * invScale + positionY, 1);
            spherePoint.normalise();
            spherePoint = faceTransform * spherePoint;
            spherePoint *= mPlanetRadius + height * mPlanetHeight;
            
            mCenter += spherePoint;
            
            min.makeFloor(spherePoint);
            max.makeCeil(spherePoint);
            
            pMapRow += offsetX;
        }
    }
    
    // Calculate center.
    mSurfaceNormal = mCenter /= (sGridSize * sGridSize);
    mSurfaceNormal.normalise();
    
    // Set bounding box/radius.
    setBoundingBox(AxisAlignedBox(min, max));
    mBoundingRadius = (max - min).length() / 2;
    mBoxCenter = (max + min) / 2;    
}
    
/**
 * Creates the vertex declaration.
 */
void PlanetRenderable::createVertexDeclaration() {
    sVertexDeclaration = sVertexData->vertexDeclaration;
    size_t offset = 0;
    sVertexDeclaration->addElement(0, offset, VET_FLOAT3, VES_POSITION);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
    // Cube map coords
    sVertexDeclaration->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
    offset += VertexElement::getTypeSize(VET_FLOAT2);
}

/**
 * Fills the hardware vertex and index buffers with data.
 */
void PlanetRenderable::fillHardwareBuffers() {
    // Allocate enough buffer space.
    int n = sGridSize * sGridSize + sGridSize * 4;
    int i = ((sGridSize - 1) * (sGridSize - 1) + 4 * (sGridSize - 1) ) * 6;
    sVertexBufferCapacity = n;
    sIndexBufferCapacity = i;

    // Create vertex buffer
    sVertexBuffer =
    HardwareBufferManager::getSingleton().createVertexBuffer(
                                                             sVertexDeclaration->getVertexSize(0),
                                                             sVertexBufferCapacity,
                                                             HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY);
    // Bind vertex buffer.
    sVertexData->vertexBufferBinding->setBinding(0, sVertexBuffer);
    sVertexData->vertexCount = sVertexBufferCapacity;
    
    // Create index buffer
    sIndexBuffer =
    HardwareBufferManager::getSingleton().createIndexBuffer(
                                                            HardwareIndexBuffer::IT_32BIT,
                                                            sIndexBufferCapacity,
                                                            HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY);
    sIndexData->indexBuffer = sIndexBuffer;
    sIndexData->indexCount = sIndexBufferCapacity;
    
    // Get pointers into buffers.
    const VertexElement* poselem = sVertexDeclaration->findElementBySemantic(VES_POSITION);
    //const VertexElement* texelem = sVertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES, 0);
    unsigned char* pBase = static_cast<unsigned char*>(sVertexBuffer->lock(HardwareBuffer::HBL_DISCARD));
    unsigned int* pIndex = static_cast<unsigned int*>(sIndexBuffer->lock(HardwareBuffer::HBL_DISCARD));
    
    // Output vertex data for regular grid.
    for (int j = 0; j < sGridSize; j++) {
        for (int i = 0; i < sGridSize; i++) {
            float* pPos;
            //float* pTex;
            poselem->baseVertexPointerToElement(pBase, &pPos);
            //texelem->baseVertexPointerToElement(pBase, &pTex);
            
            Real x = (float) i / (float) (sGridSize - 1);
            Real y = (float) j / (float) (sGridSize - 1);
            
            *pPos++ = x;
            *pPos++ = y;
            *pPos++ = 0.0f;

            //*pTex++ = x;
            //*pTex++ = y;
                        
            pBase += sVertexBuffer->getVertexSize();
        }
    }
    // Output vertex data for x skirts.
    for (int j = 0; j < sGridSize; j += (sGridSize - 1)) {
        for (int i = 0; i < sGridSize; i++) {
            float* pPos;
            //float* pTex;
            poselem->baseVertexPointerToElement(pBase, &pPos);
            //texelem->baseVertexPointerToElement(pBase, &pTex);
            
            Real x = (float) i / (float) (sGridSize - 1);
            Real y = (float) j / (float) (sGridSize - 1);
            
            *pPos++ = x;
            *pPos++ = y;
            *pPos++ = -1.0f;
            
            //*pTex++ = x;
            //*pTex++ = y;
            
            pBase += sVertexBuffer->getVertexSize();
        }
    }
    // Output vertex data for y skirts.
    for (int i = 0; i < sGridSize; i += (sGridSize - 1)) {
        for (int j = 0; j < sGridSize; j++) {
            float* pPos;
            //float* pTex;
            poselem->baseVertexPointerToElement(pBase, &pPos);
            //texelem->baseVertexPointerToElement(pBase, &pTex);
            
            Real x = (float) i / (float) (sGridSize - 1);
            Real y = (float) j / (float) (sGridSize - 1);
            
            *pPos++ = x;
            *pPos++ = y;
            *pPos++ = -1.0f;
            
            //*pTex++ = x;
            //*pTex++ = y;
            
            pBase += sVertexBuffer->getVertexSize();
        }
    }
    
    int index = 0, skirtIndex = 0;
    // Output indices for regular surface.
    for (int j = 0; j < (sGridSize - 1); j++) {
        for (int i = 0; i < (sGridSize - 1); i++) {
            *pIndex++ = index;
            *pIndex++ = index + sGridSize;
            *pIndex++ = index + 1;
            
            *pIndex++ = index + sGridSize;
            *pIndex++ = index + sGridSize + 1;
            *pIndex++ = index + 1;
            
            index++;
        }
        index++;
    }
    // Output indices for x skirts.
    index = 0;
    skirtIndex = sGridSize * sGridSize;
    for (int i = 0; i < (sGridSize - 1); i++) {
        *pIndex++ = index;
        *pIndex++ = index + 1;
        *pIndex++ = skirtIndex;
        
        *pIndex++ = skirtIndex;
        *pIndex++ = index + 1;
        *pIndex++ = skirtIndex + 1;
        
        index++;
        skirtIndex++;
    }
    index = sGridSize * (sGridSize - 1);
    skirtIndex = sGridSize * (sGridSize + 1);
    for (int i = 0; i < (sGridSize - 1); i++) {
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
    skirtIndex = sGridSize * (sGridSize + 2);
    for (int i = 0; i < (sGridSize - 1); i++) {
        *pIndex++ = index;
        *pIndex++ = skirtIndex;
        *pIndex++ = index + sGridSize;
        
        *pIndex++ = skirtIndex;
        *pIndex++ = skirtIndex + 1;
        *pIndex++ = index + sGridSize;
        
        index += sGridSize;
        skirtIndex++;
    }
    index = (sGridSize - 1);
    skirtIndex = sGridSize * (sGridSize + 3);
    for (int i = 0; i < (sGridSize - 1); i++) {
        *pIndex++ = index;
        *pIndex++ = index + sGridSize;
        *pIndex++ = skirtIndex;
        
        *pIndex++ = skirtIndex;
        *pIndex++ = index + sGridSize;
        *pIndex++ = skirtIndex + 1;
        
        index += sGridSize;
        skirtIndex++;
    }
    
    // Release buffers.
    sIndexBuffer->unlock();
    sVertexBuffer->unlock();
}

bool PlanetRenderable::preRender(SceneManager* sm, RenderSystem* rsys) {
    return true;
}

void PlanetRenderable::postRender(SceneManager* sm, RenderSystem* rsys) {
}

void PlanetRenderable::updateRenderQueue(RenderQueue* queue) {
    _updateRenderQueue(queue);
}

void PlanetRenderable::_updateRenderQueue(RenderQueue* queue) {
    SimpleRenderable::_updateRenderQueue(queue);

    if (mWireBoundingBox == NULL) {
        mWireBoundingBox = OGRE_NEW WireBoundingBox();
        mWireBoundingBox->setupBoundingBox(mBox);
    }
    queue->addRenderable(mWireBoundingBox);
}

const String& PlanetRenderable::getMovableType(void) const {
    static String movType = "PlanetRenderable";
    return movType;
}

void PlanetRenderable::setFrameOfReference(SimpleFrustum& frustum, Vector3 cameraPosition, Vector3 cameraPlane, Real sphereClip, Real lodDetailFactorSquared) {
    // Bounding box clipping.
    mIsClipped = !frustum.isVisible(mBox);

    // Get vector from center to camera and normalize it.
    Vector3 positionOffset = cameraPosition - mCenter;
    Vector3 viewDirection = positionOffset;
    
    // Find the offset between the center of the grid and the grid point closest to the camera (rough approx).
    Vector3 referenceOffset = .5 * (viewDirection - mSurfaceNormal.dotProduct(viewDirection) * mSurfaceNormal);
    float tileRadius = mPlanetRadius / (1 << mQuadTreeNode->mLOD);
    if (referenceOffset.length() > tileRadius) {
        referenceOffset.normalise();
        referenceOffset = referenceOffset * tileRadius;
    }

    // Spherical distance map clipping.
    Vector3 referenceCoordinate = mCenter + referenceOffset;
    referenceCoordinate.normalise();
    mIsFarAway = (cameraPlane.dotProduct(referenceCoordinate) < sphereClip);
    mIsClipped = mIsClipped || mIsFarAway;
    
    // Now find the closest point to the camera (approx).
    positionOffset += referenceOffset;
    
    // Determine factor to shrink LOD by due to perspective foreshortening.
    // Pad shortening factor based on LOD level to compensate for grid curving.
    Real lodSpan = mPlanetRadius / ((1 << mQuadTreeNode->mLOD) * positionOffset.length());
    viewDirection.normalise();
    Real lodShorten = minf(1.0f, mSurfaceNormal.crossProduct(viewDirection).length() + lodSpan);
    Real lodShortenSquared = lodShorten * lodShorten;
    mIsInLODRange = positionOffset.squaredLength() > maxf(mDistanceSquared, mChildDistanceSquared) * lodDetailFactorSquared * lodShortenSquared;

    // Calculate texel resolution
//    positionOffset -= referenceOffset;
    float distance = positionOffset.length(); // Distance to tile center
    float faceSize = mScaleFactor * (mPlanetRadius * Math::PI / 2); // Curved width/height of texture cube face on the sphere
    float res = faceSize / (1 << mMapTile->getNode()->mLOD) / mMap->getWidth(); // Size of a single texel in world units
    float screenres = distance / getInt("screenWidth"); // Size of a screen pixel in world units at given distance.
    
    mIsInMIPRange = res < screenres;
}

const bool PlanetRenderable::isClipped() const {
    return mIsClipped;
}

const bool PlanetRenderable::isFarAway() const {
    return mIsFarAway;
}
    
const bool PlanetRenderable::isInLODRange() const {
    return mIsInLODRange;
}

const bool PlanetRenderable::isInMIPRange() const {
    return mIsInMIPRange;
}
    
Real PlanetRenderable::getBoundingRadius(void) const
{
    return Math::Sqrt(std::max(mBox.getMaximum().squaredLength(), mBox.getMinimum().squaredLength()));
}

Real PlanetRenderable::getSquaredViewDepth(const Camera* cam) const
{
    Vector3 vMin, vMax, vMid, vDist;
    vDist = cam->getDerivedPosition() - mCenter;
    
    return vDist.squaredLength();
}
    
const PlanetMapTile* PlanetRenderable::getMapTile() {
    return mMapTile;
}

};
