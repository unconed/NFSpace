/*
 *  PlanetCubeTree.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 29/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "PlanetCubeTree.h"

namespace NFSpace {


QuadTreeNode::QuadTreeNode() :
mRenderable(0),
mParent(0),
mParentSlot(-1),
mFace(0),
mX(0),
mY(0),
mLOD(0) {
    for (int i = 0; i < 4; ++i) {
        mChildren[i] = 0;
    }
}

QuadTreeNode::~QuadTreeNode() {
    if (mParent) {
        mParent->mChildren[mParentSlot] = 0;
    }
    destroyRenderable();
    for (int i = 0; i < 4; ++i) {
        detachChild(i);
    }
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

void QuadTreeNode::createRenderable(Image* map) {
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

void QuadTreeNode::render(RenderQueue* queue, int lodLimit, SimpleFrustum& frustum, Vector3 cameraPosition, Vector3 cameraPlane, Real sphereClip, Real lodDetailFactorSquared) {
    if (mRenderable) {
        mRenderable->setFrameOfReference(frustum, cameraPosition, cameraPlane, sphereClip, lodDetailFactorSquared);
        if (mRenderable->isClipped()) {
            return;
        }
        if (mLOD == lodLimit || mRenderable->isInLODRange()) {
            queue->addRenderable(mRenderable);
            return;
        }                
    }
    for (int i = 0; i < 4; ++i) {
        if (mChildren[i]) {
            mChildren[i]->render(queue, lodLimit, frustum, cameraPosition, cameraPlane, sphereClip, lodDetailFactorSquared);
        }
    }
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