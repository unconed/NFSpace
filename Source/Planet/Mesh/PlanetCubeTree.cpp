/*
 *  PlanetCubeTree.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 29/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "PlanetCubeTree.h"
#include "EngineState.h"

namespace NFSpace {

QuadTreeNode::QuadTreeNode(PlanetCube* cube) :
mCube(cube),
mRenderable(0),
mMapTile(0),
mParent(0),
mParentSlot(-1),
mFace(0),
mX(0),
mY(0),
mLOD(0),
mRequestMapTile(false),
mRequestRenderable(false),
mRequestSplit(false) {
    for (int i = 0; i < 4; ++i) {
        mChildren[i] = 0;
    }
    PlanetStats::statsNodes++;
}

QuadTreeNode::~QuadTreeNode() {
    if (mParent) {
        mParent->mChildren[mParentSlot] = 0;
    }
    destroyMapTile();
    destroyRenderable();
    for (int i = 0; i < 4; ++i) {
        detachChild(i);
    }
    PlanetStats::statsNodes--;
}

void QuadTreeNode::propagateLODDistances() {
    if (mRenderable) {
        Real childDistance = 0;
        // Get maximum LOD distance of all children.
        for (int i = 0; i < 4; ++i) {
            if (mChildren[i] && mChildren[i]->mRenderable) {
                // Increase LOD distance w/ centroid distances, to ensure proper LOD nesting.
                childDistance = maxf(childDistance,
                                     mChildren[i]->mRenderable->getLODDistance() +
                                     (mChildren[i]->mRenderable->getCenter() - mRenderable->getCenter()).length());
            }
        }
        // Store in renderable.
        mRenderable->setChildLODDistance(childDistance);
    }
    // Propagate changes to parent.
    if (mParent) {
        mParent->propagateLODDistances();
    }
}

void QuadTreeNode::createMapTile(PlanetMap* map) {
    if (mMapTile) throw "Creating map tile that already exists.";
    mMapTile = map->generateTile(this);
}

void QuadTreeNode::destroyMapTile() {
    if (mMapTile) delete mMapTile;
    mMapTile = 0;
}
    
void QuadTreeNode::createRenderable(PlanetMapTile* map) {
    if (mRenderable) throw "Creating renderable that already exists.";
    mRenderable = new PlanetRenderable(this, map);
    propagateLODDistances();
}

void QuadTreeNode::destroyRenderable() {
    if (mRenderable) delete mRenderable;
    mRenderable = 0;
    propagateLODDistances();
}

void QuadTreeNode::attachChild(QuadTreeNode* child, int position) {
    if (mChildren[position]) throw "Attaching child where one already exists.";
    
    mChildren[position] = child;
    child->mParent = this;
    child->mParentSlot = position;
    
    child->mFace = mFace;
    child->mLOD = mLOD + 1;
    child->mX = mX * 2 + (position % 2);
    child->mY = mY * 2 + (position / 2);
}

void QuadTreeNode::detachChild(int position) {
    if (mChildren[position]) {
        delete mChildren[position];
        mChildren[position] = 0;
    }            
}
    
bool QuadTreeNode::willRender() {
    // Being asked to render ourselves.
    if (!mRenderable) {
        if (!mRequestRenderable) {            
            mRequestRenderable = true;
            mCube->request(this);
        }
        return false;
    }
    return true;
}

void QuadTreeNode::render(RenderQueue* queue, int lodLimit, SimpleFrustum& frustum, Vector3 cameraPosition, Vector3 cameraPlane, Real sphereClip, Real lodDetailFactorSquared) {
    // Determine if this node's children are render-ready.
    bool hasChildren = true;
    bool willRenderChildren = true;
    for (int i = 0; i < 4; ++i) {
        // Note: intentionally call willRender on /all/ children, not just until one fails.
        if (!mChildren[i]) {
            hasChildren = false;
        }
        else if (!mChildren[i]->willRender()) {
            willRenderChildren = false;
        }
    }
    
    // If we are renderable, check LOD/visibility.
    if (mRenderable) {
        mRenderable->setFrameOfReference(frustum, cameraPosition, cameraPlane, sphereClip, lodDetailFactorSquared);
        
        // If invisible, return immediately.
        if (mRenderable->isClipped()) {
            return;
        }
        
        // Whether to recurse down.
        bool recurse = false;

        // If the texture is not fine enough...
        if (!mRenderable->isInMIPRange()) {
            // And this tile is already at native res...
            if (mMapTile) {
                // Split so we can try this again on the child tiles.
                recurse = true;
            }
            else if (!mRequestMapTile) {
                // Request a native res map tile.
                mRequestMapTile = true;
                mCube->request(this);
            }
        }

        // If the geometry is not fine enough...
        if (!mRenderable->isInLODRange()) {
            // Go down an LOD level.
            recurse = true;
        }                

        // If a recursion was requested...
        if (recurse) {
            // And children are available and renderable...
            if (hasChildren) {
                if (willRenderChildren) {
                    // Recurse down.
                    for (int i = 0; i < 4; ++i) {
                        mChildren[i]->render(queue, lodLimit, frustum, cameraPosition, cameraPlane, sphereClip, lodDetailFactorSquared);
                    }
                    return;
                }
            }
            // If no children exist yet, request them.
            else if (!mRequestSplit) {
                mRequestSplit = true;
                mCube->request(this);
            }
        }

        // Otherwise, render ourselves.
        mRenderable->updateRenderQueue(queue);
    }

}

unsigned long QuadTreeNode::getGPUMemoryUsage() {
    unsigned long accum = 0;
    if (mMapTile) {
        accum += mMapTile->getGPUMemoryUsage();
    }
    for (int i = 0; i < 4; ++i) {
        if (mChildren[i]) {
            accum += mChildren[i]->getGPUMemoryUsage();
        }
    }
    return accum;
}
            


QuadTree::QuadTree() : mRoot(0) { }

QuadTree::QuadTree(QuadTreeNode* root) : mRoot(root) {
    mRoot->mLOD = 0;
}

QuadTree::~QuadTree() {
    if (mRoot) delete mRoot;
}

void QuadTree::setRoot(int face, QuadTreeNode* root) {
    if (mRoot) delete mRoot;
    mRoot = root;
    mRoot->mFace = face;
    mRoot->mLOD = 0;
}

};