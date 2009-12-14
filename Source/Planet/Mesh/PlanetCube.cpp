/*
 *  PlanetCube.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 9/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "PlanetCube.h"

#include "EngineState.h"

using namespace Ogre;

namespace NFSpace {

PlanetCube::PlanetCube(MovableObject* proxy, PlanetMap* map)
: mProxy(proxy), mLODCamera(0), mMap(map) {
    for (int i = 0; i < 6; ++i) {
        initFace(i);
    }
}

PlanetCube::~PlanetCube() {
    for (int i = 0; i < 6; ++i) {
        deleteFace(i);
    }
}
    
void PlanetCube::initFace(int face) {
    mFaces[face] = new QuadTree();
    QuadTreeNode* node = new QuadTreeNode(this);
    mFaces[face]->setRoot(face, node);
}

void PlanetCube::deleteFace(int face) {
    delete mFaces[face];
}
    
void PlanetCube::splitQuadTreeNode(QuadTreeNode* node) {
    for (int i = 0; i < 4; ++i) {
        QuadTreeNode* child = new QuadTreeNode(this);
        node->attachChild(child, i);
    }
}
    
void PlanetCube::mergeQuadTreeNode(QuadTreeNode* node) {

}

void PlanetCube::request(QuadTreeNode* node) {
    mRequests.push_back(node);
}
    
void PlanetCube::handleRequests() {
    int limit = 1;
    while (mRequests.size() > 0) {
        QuadTreeNode* node = *(mRequests.begin());
        bool iterate;
        do {
            if (limit-- == 0) return;

            iterate = false;
            
            if (node->mRequestRenderable) {
                // Determine max relative LOD depth between grid and tile
                int maxLODRatio = (PlanetMap::PLANET_TEXTURE_SIZE - 1) / (getInt("planet.gridSize") - 1);
                int maxLOD = 0;
                while (maxLODRatio > 1) {
                    maxLODRatio >>= 1;
                    maxLOD++;
                }
                
                // See if we can find a maptile to derive from.
                QuadTreeNode *ancestor = node;
                while (ancestor->mMapTile == 0 && ancestor->mParent) { ancestor = ancestor->mParent; };
                
                // See if map tile found is in acceptable LOD range (ie. gridsize <= texturesize).
                if (ancestor->mMapTile) {
                    int relativeLOD = node->mLOD - ancestor->mLOD;
                    if (relativeLOD <= maxLOD) {
                        // Replace existing renderable.
                        node->destroyRenderable();
                        // Create renderable relative to the map tile.
                        node->createRenderable(ancestor->mMapTile);
                        node->mRenderable->setProxy(mProxy);
                        node->mRequestRenderable = false;
                    }
                }
                
                // If no renderable was created, try creating a map tile.
                if (node->mRequestRenderable && !node->mMapTile) {
                    // Request a map tile for this node's LOD level.
                    node->mRequestMapTile = true;
                }
            }
            
            if (node->mRequestMapTile) {
                // Generate a map tile for this node.
                node->createMapTile(mMap);
                node->mRequestMapTile = false;
                
                // Request a new renderable to match.
                node->mRequestRenderable = true;
                iterate = true;
            }
            
            if (node->mRequestSplit) {
                if (node->mLOD + 1 <= getInt("planet.lodLimit")) {
                    splitQuadTreeNode(node);
                    node->mRequestSplit = false;
                }
            }
            
        } while (iterate);

        mRequests.pop_front();
    }
}

void PlanetCube::updateRenderQueue(RenderQueue* queue, const Matrix4& fullTransform) {
    // Update LOD state.
    if (mLODCamera && !getBool("planet.lodFreeze")) {
        // TODO: need to compensate for full transform on camera position.
        mLODPosition = mLODCamera->getPosition() - mProxy->getParentNode()->getPosition();
        mLODFrustum.setModelViewProjMatrix(mLODCamera->getProjectionMatrix() * mLODCamera->getViewMatrix() * fullTransform);

        mLODCameraPlane = mLODPosition;
        mLODCameraPlane.normalise();
        
        Real planetRadius = getReal("planet.radius");
        Real planetHeight = getReal("planet.height");
        
        if (mLODPosition.length() > planetRadius) {
            mLODSphereClip = cos(
                            acos(planetRadius / (planetRadius + planetHeight)) +
                            acos(planetRadius / mLODPosition.length())
                        );
        }
        else {
            mLODSphereClip = -1;
        }

    }

    // Update LOD requests.
    if (!getBool("planet.pageFreeze")) {
        handleRequests();
    }

    PlanetStats::gpuMemoryUsage = 0;
///    PlanetStats::gpuMemoryUsage += mFaces[i]->mRoot->getGPUMemoryUsage();

    for (int i = 0; i < 6; ++i) {
        if (mFaces[i]->mRoot->willRender()) {
            mFaces[i]->mRoot->render(queue, getInt("planet.lodLimit"), mLODFrustum, mLODPosition, mLODCameraPlane, mLODSphereClip, mLODPixelFactor * mLODPixelFactor);
        }
    }    
}

const Real PlanetCube::getScale() const {
    return getReal("planet.radius") + getReal("planet.height");
}

const Quaternion PlanetCube::getFaceCamera(int face) {
    // The camera is looking at the specified planet face from the inside.
    switch (face) {
        default:
        case Planet::RIGHT:
            return Quaternion(0.707f, 0.0f,-0.707f, 0.0f);
        case Planet::LEFT:
            return Quaternion(0.707f, 0.0f, 0.707f, 0.0f);
        case Planet::TOP:
            return Quaternion(0.707f,-0.707f, 0.0f, 0.0f);
        case Planet::BOTTOM:
            return Quaternion(0.707f, 0.707f, 0.0f, 0.0f);
        case Planet::FRONT:
            return Quaternion(0.0f, 0.0f, 1.0f, 0.0f);
        case Planet::BACK:
            return Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
    }
}
    
const Matrix3 PlanetCube::getFaceTransform(int face) {
    // Note, these are LHS transforms because the cube is rendered from the inside, but seen from the outside.
    // Hence there needs to be a parity switch for each face.
    switch (face) {
        default:
        case Planet::RIGHT:
            return Matrix3( 0, 0, 1,
                            0, 1, 0,
                            1, 0, 0
                           );
        case Planet::LEFT:
            return Matrix3( 0, 0,-1,
                            0, 1, 0,
                           -1, 0, 0
                           );
        case Planet::TOP:
            return Matrix3( 1, 0, 0,
                            0, 0, 1,
                            0, 1, 0
                           );
        case Planet::BOTTOM:
            return Matrix3( 1, 0, 0,
                            0, 0,-1,
                            0,-1, 0
                           );
        case Planet::FRONT:
            return Matrix3(-1, 0, 0,
                            0, 1, 0,
                            0, 0, 1
                           );
        case Planet::BACK:
            return Matrix3( 1, 0, 0,
                            0, 1, 0,
                            0, 0,-1
                           );
    }
}

void PlanetCube::setCamera(Camera* camera) {
    mLODCamera = camera;
    if (camera) {
        Real mLODLevel = EngineState::getSingleton().getRealValue("planet.lodDetail");
        if (mLODLevel <= 0.0) mLODLevel = 1.0;
        int height = EngineState::getSingleton().getIntValue("screenHeight");
        mLODPixelFactor = height / (2 * (mLODLevel) * tan(camera->getFOVy().valueRadians()));
    }
}

};