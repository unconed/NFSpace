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

    class RequestComparePriority {
    public:
        bool operator()(const Request& a, const Request& b) const;
    }; 

public:
    
    typedef set<PlanetCube*> PlanetCubeSet;
    typedef list<Request> RequestQueue;
    typedef set<QuadTreeNode*> NodeSet;
    typedef priority_queue<QuadTreeNode*, vector<QuadTreeNode*>, QuadTreeNodeCompareLastOpened> NodeHeap;

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
    inline int getFrameCounter() { return mFrameCounter; } 
    
protected:
    void initFace(int face);
    void deleteFace(int face);
    void splitQuadTreeNode(QuadTreeNode* node);
    void mergeQuadTreeNode(QuadTreeNode* node);

    void request(QuadTreeNode* node, int type, bool priority = false);
    void unrequest(QuadTreeNode* node);
    void handleRequests(RequestQueue& queue);
    void handleRenderRequests();
    void handleInlineRequests();
        
    bool handleRenderable(QuadTreeNode* node);
    bool handleMapTile(QuadTreeNode* node);
    bool handleSplit(QuadTreeNode* node);
    bool handleMerge(QuadTreeNode* node);

    void pruneTree();
    void refreshMapTile(QuadTreeNode* node, PlanetMapTile* tile);

    class CubeFrameListener : public FrameListener {
        PlanetCube* mCube;
    public:
        CubeFrameListener(PlanetCube* cube);
        virtual ~CubeFrameListener();
        virtual bool frameStarted(const FrameEvent& evt);
		virtual bool frameRenderingQueued(const FrameEvent& evt);
        virtual bool frameEnded(const FrameEvent& evt);
    };
    
    CubeFrameListener* mFrameListener;

    RequestQueue mInlineRequests;
    RequestQueue mRenderRequests;
    QuadTree* mFaces[6];
    NodeSet mOpenNodes;
    
    MovableObject* mProxy;
    PlanetMap* mMap;

    Camera* mLODCamera;
    PlanetLODConfiguration mLOD;

    int mFrameCounter;
    Timer* mTimer;    
};

};

#endif