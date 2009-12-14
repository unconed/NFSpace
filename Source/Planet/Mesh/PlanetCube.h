/*
 *  PlanetCube.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 9/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PlanetCube_H
#define PlanetCube_H

class PlanetCube;

#include <deque>
#include <Ogre/Ogre.h>

#include "Planet.h"
#include "PlanetMap.h"
#include "PlanetCubeTree.h"

using namespace Ogre;
using namespace std;

namespace NFSpace {
    
/**
 * Data structure for loading and storing the cube-based tesselation of a planet surface.
 */
class PlanetCube {
    friend class PlanetRenderable;
    friend class QuadTreeNode;

public:
    typedef deque<QuadTreeNode*> RequestQueue;

    /**
     * Constructor
     */
    PlanetCube(MovableObject* proxy, PlanetMap* map);
    ~PlanetCube();

    static const Quaternion getFaceCamera(int face);
    static const Matrix3 getFaceTransform(int face);
        
    const Real getScale() const;
    virtual void updateRenderQueue(RenderQueue* queue, const Matrix4& fullTransform);
    void setCamera(Camera* camera);
    Renderable* getRenderableForNode(QuadTreeNode* node);

protected:
    void initFace(int face);
    void deleteFace(int face);
    void splitQuadTreeNode(QuadTreeNode* node);
    void mergeQuadTreeNode(QuadTreeNode* node);

    void request(QuadTreeNode* node);
    void handleRequests();
    
    QuadTree* mFaces[6];
    RequestQueue mRequests;
    MovableObject* mProxy;
    PlanetMap* mMap;
    
    SimpleFrustum mLODFrustum;
    Camera* mLODCamera;
    Vector3 mLODPosition;
    Real mLODPixelFactor;
    Vector3 mLODCameraPlane;
    Real mLODSphereClip;
};

};

#endif