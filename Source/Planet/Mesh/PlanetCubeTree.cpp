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
mRequestSplit(false),
mRequestMerge(false),
mPageOut(false),
mHasChildren(false)
{
    mLastOpened = mLastRendered = mCube->getFrameCounter();
        
    for (int i = 0; i < 4; ++i) {
        mChildren[i] = 0;
    }
    PlanetStats::totalNodes++;
}

QuadTreeNode::~QuadTreeNode() {
    mCube->unrequest(this);
    if (mPageOut) {
        PlanetStats::totalPagedOut--;
    }
    if (mParent) {
        mParent->mChildren[mParentSlot] = 0;
    }
    destroyMapTile();
    destroyRenderable();
    for (int i = 0; i < 4; ++i) {
        detachChild(i);
    }
    PlanetStats::totalNodes--;
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
                                     0 * (mChildren[i]->mRenderable->getCenter() - mRenderable->getCenter()).length());
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
    if (mMapTile) {
        throw "Creating map tile that already exists.";
    }
    mMapTile = map->generateTile(this);
}

void QuadTreeNode::destroyMapTile() {
    if (mMapTile) delete mMapTile;
    mMapTile = 0;
}
    
void QuadTreeNode::createRenderable(PlanetMapTile* map) {
    if (mRenderable) {
        throw "Creating renderable that already exists.";
    }
    if (mPageOut) {
        PlanetStats::totalPagedOut--;
        mPageOut = false;
    }
    mRenderable = new PlanetRenderable(this, map);
    propagateLODDistances();
}

void QuadTreeNode::destroyRenderable() {
    if (mRenderable) delete mRenderable;
    mRenderable = 0;
    propagateLODDistances();
}

void QuadTreeNode::attachChild(QuadTreeNode* child, int position) {
    if (mChildren[position]) {
        throw "Attaching child where one already exists.";
    }
    
    mChildren[position] = child;
    child->mParent = this;
    child->mParentSlot = position;
    
    child->mFace = mFace;
    child->mLOD = mLOD + 1;
    child->mX = mX * 2 + (position % 2);
    child->mY = mY * 2 + (position / 2);
    
    mHasChildren = true;
}

void QuadTreeNode::detachChild(int position) {
    if (mChildren[position]) {
        delete mChildren[position];
        mChildren[position] = 0;
        
        mHasChildren = mChildren[0] || mChildren[1] || mChildren[2] || mChildren[3];
    }
}
    
    
bool QuadTreeNode::willRender() {
    // Being asked to render ourselves.
    if (!mRenderable) {
        mLastOpened = mLastRendered = mCube->getFrameCounter();

        if (mPageOut && mHasChildren) {
            return true;
        }
        
        if (!mRequestRenderable) {            
            mRequestRenderable = true;
            mCube->request(this, PlanetCube::REQUEST_RENDERABLE);
        }
        return false;
    }
    return true;
}

int QuadTreeNode::render(RenderQueue* queue, int lodLimit, SimpleFrustum& frustum, Vector3 cameraPosition, Vector3 cameraPlane, Real sphereClip, Real lodDetailFactorSquared) {
    // Determine if this node's children are render-ready.
    bool willRenderChildren = true;
    for (int i = 0; i < 4; ++i) {
        // Note: intentionally call willRender on /all/ children, not just until one fails,
        // to ensure all 4 children are queued in immediately.
        if (!mChildren[i] || !mChildren[i]->willRender()) {
            willRenderChildren = false;
        }
    }

    // If node is paged out, always recurse.
    if (mPageOut) {
        // Recurse down, calculating min recursion level of all children.
        int level = 9999;
        for (int i = 0; i < 4; ++i) {
            level = min(level, mChildren[i]->render(queue, lodLimit, frustum, cameraPosition, cameraPlane, sphereClip, lodDetailFactorSquared));
        }
        // If we are a shallow node.
        if (!mRequestRenderable && level <= 1) {
            mRequestRenderable = true;
            mCube->request(this, PlanetCube::REQUEST_RENDERABLE);
        }
        return level + 1;
    }
    
    // If we are renderable, check LOD/visibility.
    if (mRenderable) {
        mRenderable->setFrameOfReference(frustum, cameraPosition, cameraPlane, sphereClip, lodDetailFactorSquared);
        
        // If invisible, return immediately.
        if (mRenderable->isClipped()) {
            return 1;
        }

        // Whether to recurse down.
        bool recurse = false;

        // If the texture is not fine enough...
        if (!mRenderable->isInMIPRange()) {
            // If there is already a native res map-tile...
            if (mMapTile) {
                // Make sure the renderable is up-to-date.
                if (mRenderable->getMapTile() == mMapTile) {
                    // Split so we can try this again on the child tiles.
                    recurse = true;
                }
            }
            // Otherwise try to get native res tile data.
            else {
                // Make sure no parents are waiting for tile data.
                QuadTreeNode *ancestor = this;
                bool parentRequest = false;
                while (ancestor && !ancestor->mMapTile && !ancestor->mPageOut) {
                    if (ancestor->mRequestMapTile) {
                        parentRequest = true;
                        break;
                    }
                    ancestor = ancestor->mParent;
                }
                
                if (!parentRequest) {
                    // Request a native res map tile.
                    mRequestMapTile = true;
                    mCube->request(this, PlanetCube::REQUEST_MAPTILE);
                }
            }
        }

        // If the geometry is not fine enough...
        if ((mHasChildren || !mRequestMapTile) && !mRenderable->isInLODRange()) {
            // Go down an LOD level.
            recurse = true;
        }

        // If a recursion was requested...
        if (recurse) {
            // Update recursion counter, used to find least recently used nodes to page out.
            mLastOpened = mCube->getFrameCounter();
            
            // And children are available and renderable...
            if (mHasChildren) {
                if (willRenderChildren) {
                    // Recurse down, calculating min recursion level of all children.
                    int level = 9999;
                    for (int i = 0; i < 4; ++i) {
                        level = min(level, mChildren[i]->render(queue, lodLimit, frustum, cameraPosition, cameraPlane, sphereClip, lodDetailFactorSquared));
                    }
                    // If we are a shallow node with a tile that is not being rendered or close to being rendered.
                    if (level > 1 && mMapTile && mMapTile->getReferences() == 1) {
                        PlanetStats::totalPagedOut++;
                        mPageOut = true;
                        destroyRenderable();
                        destroyMapTile();
                    }
                    return level + 1;
                }
            }
            // If no children exist yet, request them.
            else if (!mRequestSplit) {
                mRequestSplit = true;
                mCube->request(this, PlanetCube::REQUEST_SPLIT);

#ifdef NF_DEBUG_TREEMGT
                QuadTreeNode* node = this;
                printf("requestSplit (f%d @ %d - %d, %d) - Vis,LOD,MIP: %d, %d, %d\n",
                       node->mFace, node->mLOD, node->mX, node->mY,
                       !node->mRenderable->isClipped(),
                       node->mRenderable->isInLODRange(),
                       node->mRenderable->isInMIPRange()//
                       );
#endif
            }
        }
        
        // Last rendered flag, used to find ancestor patches that can be paged out.
        mLastRendered = mCube->getFrameCounter();

        // Otherwise, render ourselves.
        mRenderable->updateRenderQueue(queue);
        PlanetStats::renderedRenderables++;

        return 1;
    }
    return 0;
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
          
bool QuadTreeNode::isSplit() {
    return mChildren[0] || mChildren[1] || mChildren[2] || mChildren[3];
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