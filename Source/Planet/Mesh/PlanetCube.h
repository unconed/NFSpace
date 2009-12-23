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

#include <list>
#include <set>
#include <queue>
#include <Ogre/Ogre.h>

#include "Planet.h"
#include "PlanetMap.h"
#include "PlanetCubeTree.h"

using namespace Ogre;
using namespace std;

namespace NFSpace {
    
/**
 * Data structure for loading and storing the cube-based tesselation of a planet surface.
 *
 * - holds the tree structure, which is linked to tiles and renderables.
 * - processes requests to expand/shrink the tree
 * - passes of render requests to the tree
 * - manages planet-wide LOD state
 */
class PlanetCube {
    friend class PlanetRenderable;
    friend class QuadTreeNode;
    
    enum {
        REQUEST_RENDERABLE,
        REQUEST_MAPTILE,
        REQUEST_SPLIT,
        REQUEST_MERGE,
    };

    struct Request {
        QuadTreeNode* mNode;
        int mType;
        
        Request(QuadTreeNode* node, int type) : mNode(node), mType(type) {};
    };

public:
    
    typedef set<PlanetCube*> PlanetCubeSet;
    typedef list<Request> RequestQueue;
    typedef set<QuadTreeNode*> NodeSet;
    typedef priority_queue<QuadTreeNode*, vector<QuadTreeNode*>, QuadTreeNodeCompare> NodeHeap;

    /**
     * Constructor
     */
    PlanetCube(MovableObject* proxy, PlanetMap* map);
    ~PlanetCube();

    static const Quaternion getFaceCamera(int face);
    static const Matrix3 getFaceTransform(int face);
    static void doMaintenance();
        
    static PlanetCubeSet sCubes;
    
    const Real getScale() const;
    virtual void updateRenderQueue(RenderQueue* queue, const Matrix4& fullTransform);
    void setCamera(Camera* camera);
    inline int getFrameCounter() { return mFrameCounter; } 
    
protected:
    void initFace(int face);
    void deleteFace(int face);
    void splitQuadTreeNode(QuadTreeNode* node);
    void mergeQuadTreeNode(QuadTreeNode* node);

    void request(QuadTreeNode* node, int type, bool priority = false);
    void unrequest(QuadTreeNode* node);
    void handleRequests();
    void handleRenderable(QuadTreeNode* node);
    void handleMapTile(QuadTreeNode* node);
    void handleSplit(QuadTreeNode* node);
    void handleMerge(QuadTreeNode* node);

    void pruneTree();
    void refreshMapTile(QuadTreeNode* node, PlanetMapTile* tile);
    
    QuadTree* mFaces[6];
    RequestQueue mRequests;
    NodeSet mOpenNodes;
    int mPruneOffset;
    
    MovableObject* mProxy;
    PlanetMap* mMap;
    
    int mFrameCounter;
    Timer* mTimer;
    
    SimpleFrustum mLODFrustum;
    Camera* mLODCamera;
    Vector3 mLODPosition;
    Real mLODPixelFactor;
    Vector3 mLODCameraPlane;
    Real mLODSphereClip;
};

};

#endif