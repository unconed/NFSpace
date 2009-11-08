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
    QuadTreeNode* node = new QuadTreeNode();
    mFaces[face]->setRoot(face, node);
    initQuadTreeNode(node);
}

void PlanetCube::deleteFace(int face) {
    delete mFaces[face];
}
    
void PlanetCube::initQuadTreeNode(QuadTreeNode* node) {
    node->createRenderable(mMap->getHeightMap(node->mFace));
    node->mRenderable->setProxy(mProxy);
    node->mRenderable->setMaterial(mMap->getMaterial());
    if (node->mLOD < getInt("planet.lodLimit")) {
        for (int i = 0; i < 4; ++i) {
            QuadTreeNode* child = new QuadTreeNode();
            node->attachChild(child, i);
            initQuadTreeNode(child);
        }
    }
}

void PlanetCube::updateRenderQueue(RenderQueue* queue, const Matrix4& fullTransform) {
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
    for (int i = 0; i < 6; ++i) {
        mFaces[i]->mRoot->render(queue, getInt("planet.lodLimit"), mLODFrustum, mLODPosition, mLODCameraPlane, mLODSphereClip, mLODPixelFactor * mLODPixelFactor);
    }    
}

const Real PlanetCube::getScale() const {
    return getReal("planet.radius") + getReal("planet.height");
}

const Matrix3 PlanetCube::getFaceTransform(int face, bool lhs) {
    if (!lhs) {
        Matrix3 faceTransform = getFaceTransform(face, true);
        faceTransform = faceTransform * Matrix3(-1, 0, 0,
                                                0, 1, 0,
                                                0, 0, 1);
        return faceTransform;
    }
    // Note, these are LHS transforms due to following the renderman convention of texture mapping a cubemap.
    switch (face) {
        default:
        case Planet::RIGHT:
            return Matrix3( 0, 0, 1,
                            0,-1, 0,
                           -1, 0, 0
                           );
        case Planet::LEFT:
            return Matrix3( 0, 0,-1,
                            0,-1, 0,
                            1, 0, 0
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
            return Matrix3( 1, 0, 0,
                            0,-1, 0,
                            0, 0, 1
                           );
        case Planet::BACK:
            return Matrix3(-1, 0, 0,
                            0,-1, 0,
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