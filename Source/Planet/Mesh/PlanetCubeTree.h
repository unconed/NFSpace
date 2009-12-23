/*
 *  PlanetCubeTree.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 29/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PlanetCubeTree_H
#define PlanetCubeTree_H

namespace NFSpace {

struct QuadTree;
struct QuadTreeNode;
    
};

#include "SimpleFrustum.h"
#include "PlanetMap.h"
#include "PlanetCube.h"
#include "PlanetRenderable.h"

#include "Utility.h"

namespace NFSpace {
    
// Node inside a quad tree.
struct QuadTreeNode {
    enum Slot { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };
    
    QuadTreeNode(PlanetCube* cube);
    ~QuadTreeNode();
    void propagateLODDistances();
    void createMapTile(PlanetMap* map);
    void destroyMapTile();
    void createRenderable(PlanetMapTile* map);
    void destroyRenderable();
    void attachChild(QuadTreeNode* child, int position);
    void detachChild(int position);
    bool isSplit();
    
    unsigned long getGPUMemoryUsage();
    int render(RenderQueue* queue, int lodLimit, SimpleFrustum& frustum, Vector3 cameraPosition, Vector3 cameraPlane, Real sphereClip, Real lodDetailFactorSquared);
    bool willRender();
    
    int mFace;
    int mLOD;
    int mX;
    int mY;
    
    int mLastRendered;
    int mLastOpened;

    bool mHasChildren;

    bool mPageOut;

    bool mRequestPageOut;
    bool mRequestMapTile;
    bool mRequestRenderable;
    bool mRequestSplit;
    bool mRequestMerge;
    
    PlanetMapTile* mMapTile;
    PlanetRenderable* mRenderable;
    
    PlanetCube* mCube;

    int mParentSlot;
    QuadTreeNode* mParent;
    QuadTreeNode* mChildren[4];
};

// Quadtree
struct QuadTree {
    QuadTree();
    QuadTree(QuadTreeNode* root);
    ~QuadTree();
    void setRoot(int face, QuadTreeNode* root);
    
    QuadTreeNode* mRoot;
};

// Quadtree node comparison for LOD age.
class QuadTreeNodeCompare {
    public:
    bool operator()(const QuadTreeNode* a, const QuadTreeNode* b) const {
        return (a->mLastOpened > b->mLastOpened);
    }
};
    
};

#endif