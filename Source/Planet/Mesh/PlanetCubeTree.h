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
#include "PlanetCube.h"
#include "PlanetRenderable.h"

#include "Utility.h"

namespace NFSpace {
    
// Node inside a quad tree.
struct QuadTreeNode {
    enum Slot { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };
    
    QuadTreeNode();
    ~QuadTreeNode();
    void propagateLODDistances();
    void createRenderable(Image* map);
    void destroyRenderable();
    void attachChild(QuadTreeNode* child, int position);
    void detachChild(int position);
    void render(RenderQueue* queue, int lodLimit, SimpleFrustum& frustum, Vector3 cameraPosition, Vector3 cameraPlane, Real sphereClip, Real lodDetailFactorSquared);
    
    int mFace;
    int mLOD;
    int mX;
    int mY;
    PlanetRenderable* mRenderable;
    
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
    
};

#endif