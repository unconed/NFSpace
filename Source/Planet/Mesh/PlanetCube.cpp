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
: mProxy(proxy), mLODCamera(0), mMap(map), mFrameCounter(0) {
    for (int i = 0; i < 6; ++i) {
        initFace(i);
    }

    mTimer = OGRE_NEW Timer();

    mFrameListener = OGRE_NEW_T(CubeFrameListener, MEMCATEGORY_GENERAL)(this);
}

PlanetCube::~PlanetCube() {
    OGRE_DELETE_T(mFrameListener, CubeFrameListener, MEMCATEGORY_GENERAL);

    OGRE_DELETE mTimer;

    for (int i = 0; i < 6; ++i) {
        deleteFace(i);
    }
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
    RequestQueue& requestQueue = (type == REQUEST_MAPTILE) ? mRenderRequests : mInlineRequests;
    if (priority) {
        requestQueue.push_front(Request(node, type));
    }
    else {
        requestQueue.push_back(Request(node, type));
    }
}

void PlanetCube::unrequest(QuadTreeNode* node) {
    RequestQueue* requestQueues[] = { &mRenderRequests, &mInlineRequests };
    for (int q = 0; q < 2; q++) {
        RequestQueue::iterator i = (*requestQueues[q]).begin(), j;
        while (i != (*requestQueues[q]).end()) {
            if ((*i).mNode == node) {
                // Remove item at front of queue.
                if (i == (*requestQueues[q]).begin()) {
                    // Special case: if unrequesting a maptile current being generated,
                    // make sure temp/unclaimed resources are cleaned up.
                    if ((*i).mType == REQUEST_MAPTILE) {
                        mMap->resetTile();
                    }

                    (*requestQueues[q]).erase(i);
                    i = (*requestQueues[q]).begin();
                    continue;
                }
                // Remove item mid-queue.
                else {
                    j = i;
                    --i;
                    (*requestQueues[q]).erase(j);
                }
            }
            ++i;
        }
    }
}
    
void PlanetCube::handleRenderRequests() {
    handleRequests(mRenderRequests);
}

void PlanetCube::handleInlineRequests() {
    handleRequests(mInlineRequests);
}

void PlanetCube::handleRequests(RequestQueue& requests) {

    // Ensure we only use up x time per frame.
    int weights[] = { 10, 10, 1, 2 };
    bool(PlanetCube::*handlers[4])(QuadTreeNode*) = {
        &PlanetCube::handleRenderable,
        &PlanetCube::handleMapTile,
        &PlanetCube::handleSplit,
        &PlanetCube::handleMerge
    };
    int limit = 10;
    bool sorted = false;
    
    while (requests.size() > 0) {
        Request request = *requests.begin();
        QuadTreeNode* node = request.mNode;

        // If not a root level task.
        if (node->mParent) {
            // Verify job limits.
            if (limit <= 0) return;
            limit -= weights[request.mType];
        }
        
        requests.pop_front();
        // Call handler.
        if ((this->*handlers[request.mType])(node)) {
            // Job was completed. We can re-sort the priority queue.
            if (!sorted) {
                requests.sort(RequestComparePriority());
                sorted = true;
            }
        }
    }
}
    
bool PlanetCube::handleRenderable(QuadTreeNode* node) {
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
    return true;
}

bool PlanetCube::handleMapTile(QuadTreeNode* node) {
#ifdef NF_DEBUG_TREEMGT
    printf("handleMapTile (f%d @ %d - %d, %d)\n", node->mFace, node->mLOD, node->mX, node->mY);
#endif

    // See if the map tile object for this node is ready yet.
    if (!node->prepareMapTile(mMap)) {
        // Needs more work.
        request(node, REQUEST_MAPTILE, true);
        return false;
    }
    else {
        // Assemble a map tile object for this node.
        node->createMapTile(mMap);
        node->mRequestMapTile = false;

        // Request a new renderable to match.
        node->mRequestRenderable = true;
        request(node, REQUEST_RENDERABLE, true);

        // See if any child renderables use the old maptile.
        PlanetMapTile* oldTile;
        refreshMapTile(node, oldTile);
        return true;
    }
}

bool PlanetCube::handleSplit(QuadTreeNode* node) {
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
    return true;
}

bool PlanetCube::handleMerge(QuadTreeNode* node) {
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
    return true;
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
            oldNode->mRenderable->setFrameOfReference(mLOD);
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

void PlanetCube::updateRenderQueue(RenderQueue* queue, const Matrix4& fullTransform) {
    // Update LOD state.
    if (mLODCamera && !getBool("planet.lodFreeze")) {
        Matrix4 viewMatrix = mLODCamera->getViewMatrix();
        
        // TODO: need to compensate for full transform on camera position.
        mLOD.mCameraPosition = mLODCamera->getPosition() - mProxy->getParentNode()->getPosition();
        mLOD.mCameraFrustum.setModelViewProjMatrix(mLODCamera->getProjectionMatrix() * viewMatrix * fullTransform);
        mLOD.mCameraFront = Vector3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);

        mLOD.mSpherePlane = mLOD.mCameraPosition;
        mLOD.mSpherePlane.normalise();
        
        Real planetRadius = getReal("planet.radius");
        Real planetHeight = getReal("planet.height");
        
        if (mLOD.mCameraPosition.length() > planetRadius) {
            mLOD.mSphereClip = cos(
                            acos((planetRadius + planetHeight / 2) / (planetRadius + planetHeight)) +
                            acos(planetRadius / mLOD.mCameraPosition.length())
                        );
        }
        else {
            mLOD.mSphereClip = -1;
        }

    }

    PlanetStats::renderedRenderables = 0;
    PlanetStats::gpuMemoryUsage = 0;
    PlanetStats::totalOpenNodes = mOpenNodes.size();
    PlanetStats::requestQueue = mInlineRequests.size() + mRenderRequests.size();

    for (int i = 0; i < 6; ++i) {
        PlanetStats::gpuMemoryUsage += mFaces[i]->mRoot->getGPUMemoryUsage();
        if (mFaces[i]->mRoot->willRender()) {
            mFaces[i]->mRoot->render(queue, mLOD);
        }
    }
    
    mFrameCounter++;
}
    
void PlanetCube::setCamera(Camera* camera) {
    mLODCamera = camera;
    if (camera) {
        int height = EngineState::getSingleton().getIntValue("screenHeight");
        Real fov = 2.0 * tan(camera->getFOVy().valueRadians());
        
        Real geoDetail = maxf(1.0, EngineState::getSingleton().getRealValue("planet.geoDetail"));
        mLOD.mGeoFactor = height / (geoDetail * fov);
        mLOD.mGeoFactorSquared = mLOD.mGeoFactor * mLOD.mGeoFactor;

        Real texDetail = maxf(1.0, EngineState::getSingleton().getRealValue("planet.texDetail"));
        mLOD.mTexFactor = height / (texDetail * fov);
        mLOD.mTexFactorSquared = mLOD.mTexFactor * mLOD.mTexFactor;
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


    PlanetCube::CubeFrameListener::CubeFrameListener(PlanetCube* cube) : mCube(cube) {
        Ogre::Root::getSingleton().addFrameListener(this);
    }
    
    PlanetCube::CubeFrameListener::~CubeFrameListener() {
        Ogre::Root::getSingleton().removeFrameListener(this);
    }
    
    bool PlanetCube::CubeFrameListener::frameStarted(const FrameEvent& evt) {
        // Handle delayed requests (for rendering new tiles).
        mCube->handleRenderRequests();
        return true;
    }
    
	bool PlanetCube::CubeFrameListener::frameRenderingQueued(const FrameEvent& evt) {
        if (!getBool("planet.treeFreeze")) {
            // Prune the LOD tree
            mCube->pruneTree();

            // Update LOD requests.
            mCube->handleInlineRequests();
        }
        return true;
    }
    
    bool PlanetCube::CubeFrameListener::frameEnded(const FrameEvent& evt) {
        return true;
    }
    

    bool PlanetCube::RequestComparePriority::operator()(const Request& a, const Request& b) const {
        return (a.mNode->getPriority() > b.mNode->getPriority());
    }
};