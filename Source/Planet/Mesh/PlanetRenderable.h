/*
 *  PlanetRenderable.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 4/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 *  Based on DynamicRenderable.h (ogre wiki code sample)
 */

#ifndef PlanetRenderable_H
#define PlanetRenderable_H

namespace NFSpace {
    class PlanetRenderable;
};

#include "Ogre/OgreSimpleRenderable.h"
#include "Ogre/OgreWireBoundingBox.h"
#include "SimpleFrustum.h"
#include "PlanetCube.h"

using namespace Ogre;

namespace NFSpace {
    
/**
 * Renderable entity representing a single planet tile at a given face, grid square and scale.
 */
class PlanetRenderable : public SimpleRenderable {
public:
    /**
     * Constructor
     */
    PlanetRenderable(QuadTreeNode* node, Image* map);
    virtual ~PlanetRenderable();

    virtual void setProxy(MovableObject* proxy);
    virtual void getWorldTransforms(Matrix4* xform) const;
    
    Real getLODDistance();
    void setChildLODDistance(Real lodDistance);

    Vector3& getCenter();
    Real getBoundingRadius();

    void setFrameOfReference(SimpleFrustum& frustum, Vector3 cameraPosition, Vector3 cameraPlane, Real sphereClip, Real lodDetailFactorSquared);
    const bool isInLODRange() const;
    const bool isClipped() const;
    
    virtual void updateRenderQueue(RenderQueue* queue);
    Vector3 mSurfaceNormal;

protected:
    static int sInstances;
    static VertexData* sVertexData;
    static IndexData* sIndexData;
    static HardwareVertexBufferSharedPtr sVertexBuffer;
    static HardwareIndexBufferSharedPtr sIndexBuffer;
    static int sVertexBufferCapacity;
    static int sIndexBufferCapacity;
    static int sGridSize;
    static VertexDeclaration *sVertexDeclaration;
    
    static void addInstance();
    static void removeInstance();
        
    Real mBoundingRadius;
    Vector3 mCenter;
    Vector3 mBoxCenter;

    Real mChildDistance;
    Real mChildDistanceSquared;
    Real mLODDifference;
    Real mDistance;
    Real mDistanceSquared;
    Real mCurrentDistance;
    
    MovableObject* mProxy;
    Image* mMap;

    Real mPlanetRadius;
    Real mPlanetHeight;
        
    bool mIsInLODRange;
    bool mIsClipped;
    
    WireBoundingBox* mWireBoundingBox;
    const QuadTreeNode* mQuadTreeNode;

    /**
     * Creates the vertex declaration.
     */
    static void createVertexDeclaration();
    
    /**
     * Fills the hardware vertex and index buffers with data.
     */
    static void fillHardwareBuffers();

    virtual const String& getMovableType(void) const;

    virtual bool preRender(SceneManager* sm, RenderSystem* rsys);
    virtual void postRender(SceneManager* sm, RenderSystem* rsys);
    virtual void _updateRenderQueue(RenderQueue* queue);

    Real PlanetRenderable::getBoundingRadius(void) const;    
    Real getSquaredViewDepth(const Camera* cam) const;
    
    void initDisplacementMapping();
    virtual void analyseTerrain();
};

};

#endif