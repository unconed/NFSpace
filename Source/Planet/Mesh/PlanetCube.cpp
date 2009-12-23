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
    
    PlanetCube::PlanetCubeSet PlanetCube::sCubes;

PlanetCube::PlanetCube(MovableObject* proxy, PlanetMap* map)
: mProxy(proxy), mLODCamera(0), mMap(map), mFrameCounter(0), mPruneOffset(0) {
    for (int i = 0; i < 6; ++i) {
        initFace(i);
    }
    mTimer = OGRE_NEW Timer();
    sCubes.insert(this);
}

PlanetCube::~PlanetCube() {
    sCubes.erase(this);
    for (int i = 0; i < 6; ++i) {
        deleteFace(i);
    }
    OGRE_DELETE mTimer;
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
    // Parent is no longer an open node, now has at least one child.
    if (node->mParent)
        mOpenNodes.erase(node->mParent);
    // This node is now open.
    mOpenNodes.insert(node);
    // Create children.
    for (int i = 0; i < 4; ++i) {
        QuadTreeNode* child = new QuadTreeNode(this);
        node->attachChild(child, i);
    }
}
    
void PlanetCube::mergeQuadTreeNode(QuadTreeNode* node) {
    // Delete children.
    for (int i = 0; i < 4; ++i) {
        if (node->mChildren[i]->mChildren[0] ||
            node->mChildren[i]->mChildren[1] ||
            node->mChildren[i]->mChildren[2] ||
            node->mChildren[i]->mChildren[3]) {
            printf("Lickety split. Faulty merge.\n");
            assert(false);
        }
        node->detachChild(i);
    }
    // This node is now closed.
    mOpenNodes.erase(node);
    if (node->mParent) {
        // Check to see if any siblings are split.
        for (int i = 0; i < 4; ++i) {
            if (node->mParent->mChildren[i]->isSplit()) {
                return;
            }
        }
        // If not, the parent is now open.
        mOpenNodes.insert(node->mParent);
    }
}

void PlanetCube::request(QuadTreeNode* node, int type, bool priority) {
    if (priority) {
        mRequests.push_front(Request(node, type));
    }
    else {
        mRequests.push_back(Request(node, type));
    }
}

void PlanetCube::unrequest(QuadTreeNode* node) {
    RequestQueue::iterator i = mRequests.begin(), j;
    while (i != mRequests.end()) {
        if ((*i).mNode == node) {
            if (i == mRequests.begin()) {
                mRequests.erase(i);
                i = mRequests.begin();
                continue;
            }
            else {
                j = i;
                --i;
                mRequests.erase(j);
            }
        }
        ++i;
    }
}

void PlanetCube::handleRequests() {

    // Ensure we only use up x time per frame.
    //Real limit = EngineState::getSingleton().getRealValue("planet.pagerTimeSlot");
    //mTimer->reset();
    int weights[] = { 10, 10, 1, 1 };
    void(PlanetCube::*handlers[4])(QuadTreeNode*) = {
        &PlanetCube::handleRenderable,
        &PlanetCube::handleMapTile,
        &PlanetCube::handleSplit,
        &PlanetCube::handleMerge
    };
    int limit = 10;
    
    while (mRequests.size() > 0) {
        Request request = *mRequests.begin();
        QuadTreeNode* node = request.mNode;

        // If not a root level task.
        if (node->mParent) {
            // Verify job limits.
            if (limit <= 0) return;
            limit -= weights[request.mType];
        }
        
        //if (mTimer->getMilliseconds() > limit) return;

        mRequests.pop_front();
  
        (this->*handlers[request.mType])(node);        
    }
}
    
void PlanetCube::handleRenderable(QuadTreeNode* node) {
#ifdef NF_DEBUG_TREEMGT
    printf("handleRenderable (f%d @ %d - %d, %d)\n", node->mFace, node->mLOD, node->mX, node->mY);
#endif

    // Determine max relative LOD depth between grid and tile
    int maxLODRatio = (getInt("planet.textureSize") - 1) / (getInt("planet.gridSize") - 1);
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
    if (node->mRequestRenderable && !node->mMapTile && !node->mRequestMapTile) {
        // Request a map tile for this node's LOD level.
        node->mRequestMapTile = true;
        request(node, REQUEST_MAPTILE, true);
    }
}

void PlanetCube::handleMapTile(QuadTreeNode* node) {
#ifdef NF_DEBUG_TREEMGT
    printf("handleMapTile (f%d @ %d - %d, %d)\n", node->mFace, node->mLOD, node->mX, node->mY);
#endif

    // Generate a map tile for this node.
    node->createMapTile(mMap);
    node->mRequestMapTile = false;
    
    // Request a new renderable to match.
    node->mRequestRenderable = true;
    request(node, REQUEST_RENDERABLE, true);

    // See if any child renderables use the old maptile.
    PlanetMapTile* oldTile;
    refreshMapTile(node, oldTile);
}

void PlanetCube::handleSplit(QuadTreeNode* node) {
#ifdef NF_DEBUG_TREEMGT
    printf("handleSplit (f%d @ %d - %d, %d) - Vis,LOD,MIP: %d, %d, %d\n",
           node->mFace, node->mLOD, node->mX, node->mY,
           !node->mRenderable->isClipped(),
           node->mRenderable->isInLODRange(),
           node->mRenderable->isInMIPRange()//
           );
#endif
    if (node->mLOD + 1 <= getInt("planet.lodLimit")) {
        splitQuadTreeNode(node);
        node->mRequestSplit = false;
    }
    else {
        // mRequestSplit is stuck on, so will not be requested again.
    }
}

void PlanetCube::handleMerge(QuadTreeNode* node) {
#ifdef NF_DEBUG_TREEMGT
    printf("handleMerge (f%d @ %d - %d, %d) - Vis,LOD,MIP: %d, %d, %d\n",
           node->mFace, node->mLOD, node->mX, node->mY,
           !node->mRenderable->isClipped(),
           node->mRenderable->isInLODRange(),
           node->mRenderable->isInMIPRange()//
           );
#endif
    mergeQuadTreeNode(node);
    node->mRequestMerge = false;
}

void PlanetCube::refreshMapTile(QuadTreeNode* node, PlanetMapTile* tile) {
    for (int i = 0; i < 4; ++i) {
        QuadTreeNode* child = node->mChildren[i];
        if (child && child->mRenderable && child->mRenderable->getMapTile() == tile) {
            child->mRequestRenderable = true;
            request(child, REQUEST_RENDERABLE, true);

            // Recurse
            refreshMapTile(child, tile);
        }
    }
}

void PlanetCube::pruneTree() {
    NodeHeap heap;
    
    NodeSet::iterator openNode = mOpenNodes.begin();
    while (openNode != mOpenNodes.end()) {
        heap.push(*openNode);
        ++openNode;
    };
    
    while (heap.size() > 0) {
        QuadTreeNode* oldNode = heap.top();
        if (!oldNode->mPageOut && !oldNode->mRequestMerge && (getFrameCounter() - oldNode->mLastOpened > 100)) {
            oldNode->mRenderable->setFrameOfReference(mLODFrustum, mLODPosition, mLODCameraPlane, mLODSphereClip, mLODPixelFactor * mLODPixelFactor);
            // Make sure node's children are too detailed rather than just invisible.
            if (oldNode->mRenderable->isFarAway() ||
                (oldNode->mRenderable->isInLODRange() && oldNode->mRenderable->isInMIPRange())
               ) {
                oldNode->mRequestMerge = true;
                //handleMerge(oldNode);
                request(oldNode, REQUEST_MERGE, TRUE);
                return;
            }
            else {
                oldNode->mLastOpened = getFrameCounter();
            }

#ifdef NF_DEBUG_TREEMGT
            QuadTreeNode* node = oldNode;
            printf("requestMerge (f%d @ %d - %d, %d) - Vis,LOD,MIP: %d, %d, %d\n",
                   node->mFace, node->mLOD, node->mX, node->mY,
                   !node->mRenderable->isClipped(),
                   node->mRenderable->isInLODRange(),
                   node->mRenderable->isInMIPRange()//
                   );
#endif
        }
        heap.pop();
    }
}

void PlanetCube::doMaintenance() {
    for (PlanetCube::PlanetCubeSet::iterator i = PlanetCube::sCubes.begin(); i != sCubes.end(); ++i) {
        // Prune the LOD tree
        if (!getBool("planet.pruneFreeze")) {
            (*i)->pruneTree();
        }
        
        // Update LOD requests.
        if (!getBool("planet.pageFreeze")) {
            (*i)->handleRequests();
        }
    }

    updateSceneManagersAfterMaterialsChange();
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
                            acos((planetRadius + planetHeight / 2) / (planetRadius + planetHeight)) +
                            acos(planetRadius / mLODPosition.length())
                        );
        }
        else {
            mLODSphereClip = -1;
        }

    }

    PlanetStats::renderedRenderables = 0;
    PlanetStats::gpuMemoryUsage = 0;
    PlanetStats::totalOpenNodes = mOpenNodes.size();
    PlanetStats::requestQueue = mRequests.size();

    for (int i = 0; i < 6; ++i) {
        PlanetStats::gpuMemoryUsage += mFaces[i]->mRoot->getGPUMemoryUsage();
        if (mFaces[i]->mRoot->willRender()) {
            mFaces[i]->mRoot->render(queue, getInt("planet.lodLimit"), mLODFrustum, mLODPosition, mLODCameraPlane, mLODSphereClip, mLODPixelFactor * mLODPixelFactor);
        }
    }
    
    mFrameCounter++;
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


};