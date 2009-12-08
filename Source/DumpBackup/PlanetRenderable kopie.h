/*
 *  PlanetRenderable.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 4/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 *  Based on TerrainRenderable.h (LGPL)
 */

#ifndef __PLANET_RENDERABLE_H
#define __PLANET_RENDERABLE_H

#include "Ogre/OgreSceneNode.h"
#include "Ogre/OgreRenderable.h"
#include "Ogre/OgreRenderQueue.h"
#include "Ogre/OgreRenderOperation.h"
#include "Ogre/OgreCamera.h"
#include "Ogre/OgreRoot.h"
#include "Ogre/OgreLogManager.h"
#include "Ogre/OgreStringConverter.h"
#include "Ogre/OgreViewport.h"
#include "Ogre/OgreException.h"
#include "Ogre/OgreRenderSystem.h"

using namespace Ogre;

class PlanetOptions : public GeneralAllocatedObject {
public:
    PlanetOptions()
    {
        pageSize = 0;
        tileSize = 0;
        tilesPerPage = 0;
        maxGeoMipMapLevel = 0;
        scale = Vector3::UNIT_SCALE;
        maxPixelError = 4;
        detailTile = 1;
        lit = false;
        coloured = false;
        lodMorph = false;
        lodMorphStart = 0.5;
        useTriStrips = false;
        primaryCamera = 0;
        planetMaterial.setNull();
    };
    /// The size of one edge of a terrain page, in vertices
    size_t pageSize;
    /// The size of one edge of a terrain tile, in vertices
    size_t tileSize; 
    /// Precalculated number of tiles per page
    size_t tilesPerPage;
    /// The primary camera, used for error metric calculation and page choice
    const Camera* primaryCamera;
    /// The maximum terrain geo-mipmap level
    size_t maxGeoMipMapLevel;
    /// The scale factor to apply to the terrain (each vertex is 1 unscaled unit
    /// away from the next, and height is from 0 to 1)
    Vector3 scale;
    /// The maximum pixel error allowed
    size_t maxPixelError;
    /// Whether we should use triangle strips
    bool useTriStrips;
    /// The number of times to repeat a detail texture over a tile
    size_t detailTile;
    /// Whether LOD morphing is enabled
    bool lodMorph;
    /// At what point (parametric) should LOD morphing start
    Real lodMorphStart;
    /// Whether dynamic lighting is enabled
    bool lit;
    /// Whether vertex colours are enabled
    bool coloured;
    /// Pointer to the material to use to render the terrain
    MaterialPtr planetMaterial;
    
};

/**
 Represents a terrain tile.
 @remarks
 A TerrainRenderable represents a tile used to render a block of terrain using the geomipmap approach
 for LOD.
 *@author Jon Anderson
 */

class PlanetRenderable : public Renderable, public MovableObject
{
public:
    
    PlanetRenderable(const String& name, SceneManager* sceneManager);
    ~PlanetRenderable();
    
    void deleteGeometry();
    
    /** Initializes the PlanetRenderable.
     @param startx, startz 
     The starting points of the top-left of this tile, in terms of the
     number of vertices.
     @param pageHeightData The source height data for the entire parent page
     */
    void initialise(int startx, int startz, Real* pageHeightData);
    
    //movable object methods
    
    /** Returns the type of the movable. */
    virtual const String& getMovableType(void) const {
        return mType;
    };
    
    /** Returns the bounding box of this PlanetRenderable */
    const AxisAlignedBox& getBoundingBox(void) const {
        return mBounds;
    };
    
    /** Updates the level of detail to be used for rendering this PlanetRenderable based on the passed in Camera */
    virtual void _notifyCurrentCamera(Camera* cam);
    
    virtual void _updateRenderQueue(RenderQueue* queue);
    
    /// @copydoc MovableObject::visitRenderables
    void visitRenderables(Renderable::Visitor* visitor, 
                          bool debugRenderables = false);
    
    /**
     Constructs a RenderOperation to render the TerrainRenderable.
     @remarks
     Each TerrainRenderable has a block of vertices that represent the terrain.  Index arrays are dynamically
     created for mipmap level, and then cached.
     */
    virtual void getRenderOperation(RenderOperation& rend);
    
    virtual const MaterialPtr& getMaterial(void) const {
        return mMaterial;
    };
    
    virtual void getWorldTransforms(Matrix4* xform) const;
    
    /** Returns the mipmap level that will be rendered for this frame */
    inline int getRenderLevel() const {
        return mRenderLevel;
    };
    
    /** Forces the LOD to the given level from this point on */
    inline void setForcedRenderLevel(int i) {
        mForcedRenderLevel = i;
    }
    
    /** Intersects the segment witht he terrain tile
     */
    bool intersectSegment(const Vector3& start, const Vector3& end, Vector3* result);
    
    void setMaterial(const MaterialPtr& m) {
        mMaterial = m;
    };
    
    /** Overridden, see Renderable */
    Real getSquaredViewDepth(const Camera* cam) const;
    
    /** Overridden from MovableObject */
    Real getBoundingRadius(void) const { return mBoundingRadius; }
    
    /** @copydoc Renderable::getLights */
    const LightList& getLights(void) const;
    
    /// Overridden from Renderable to allow the morph LOD entry to be set
    void _updateCustomGpuParameter(
                                   const GpuProgramParameters::AutoConstantEntry& constantEntry,
                                   GpuProgramParameters* params) const;
    /// @see MovableObject
    unsigned int getTypeFlags(void) const;
protected:
    /// Parent SceneManager
    SceneManager* mSceneManager;
    /// Link to shared options
    const PlanetOptions* mOptions;
    
    void _calculateMinLevelDist2(Real C);
    
    Real _calculateCFactor();
    
    VertexData* mTerrain;
    
    /// The current LOD level
    int mRenderLevel;
    /// The previous 'next' LOD level down, for frame coherency
    int mLastNextLevel; 
    /// The morph factor between this and the next LOD level down
    Real mLODMorphFactor;
    /// List of squared distances at which LODs change
    Real *mMinLevelDistSqr;
    /// Whether light list need to re-calculate
    mutable bool mLightListDirty;
    /// Cached light list
    mutable LightList mLightList;
    /// The bounding radius of this tile
    Real mBoundingRadius;
    /// Bounding box of this tile
    AxisAlignedBox mBounds;
    /// The center point of this tile
    Vector3 mCenter;
    /// The MovableObject type
    static String mType;
    /// Current material used by this tile
    MaterialPtr mMaterial;    
    /// Whether this tile has been initialised    
    bool mInit;
    /// The buffer with all the renderable geometry in it
    HardwareVertexBufferSharedPtr mMainBuffer;
    /// Optional set of delta buffers, used to morph from one LOD to the next
    typedef std::vector<HardwareVertexBufferSharedPtr> VertexBufferList;
    VertexBufferList mDeltaBuffers;
    /// System-memory buffer with just positions in it, for CPU operations
    float* mPositionBuffer;
    /// Forced rendering LOD level, optional
    int mForcedRenderLevel;
    /// Array of LOD indexes specifying which LOD is the next one down
    /// (deals with clustered error metrics which cause LODs to be skipped)
    int mNextLevelDown[10];
    /// Gets the index data for this tile based on current settings
    IndexData* getIndexData(void);
    /// Internal method for generating stripified terrain indexes
    IndexData* generateTriStripIndexes(unsigned int stitchFlags);
    /// Internal method for generating triangle list terrain indexes
    IndexData* generateTriListIndexes(unsigned int stitchFlags);
    
};

#endif
