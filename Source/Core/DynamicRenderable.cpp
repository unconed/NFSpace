/*
 *  DynamicRenderable.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 30/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 *  Source: http://www.ogre3d.org/wiki/index.php/DynamicGrowingBuffers
 *  * Changed index type to 32-bit.
 */

#include "DynamicRenderable.h"
#include <Ogre/OgreCamera.h>
#include <Ogre/OgreHardwareBufferManager.h>

using namespace Ogre;

namespace NFSpace {

DynamicRenderable::DynamicRenderable()
{
}

DynamicRenderable::~DynamicRenderable()
{
    delete mRenderOp.vertexData;
    delete mRenderOp.indexData;
}

void DynamicRenderable::initialize(RenderOperation::OperationType operationType,
                                   bool useIndices)
{
    // Initialize render operation
    mRenderOp.operationType = operationType;
    mRenderOp.useIndexes = useIndices;
    mRenderOp.vertexData = new VertexData;
    if (mRenderOp.useIndexes)
        mRenderOp.indexData = new IndexData;
    
    // Reset buffer capacities
    mVertexBufferCapacity = 0;
    mIndexBufferCapacity = 0;
    
    // Create vertex declaration
    createVertexDeclaration();
}

void DynamicRenderable::prepareHardwareBuffers(size_t vertexCount, 
                                               size_t indexCount)
{
    // Prepare vertex buffer
    size_t newVertCapacity = mVertexBufferCapacity;
    if ((vertexCount > mVertexBufferCapacity) ||
        (!mVertexBufferCapacity))
    {
        // vertexCount exceeds current capacity!
        // It is necessary to reallocate the buffer.
        
        // Check if this is the first call
        if (!newVertCapacity)
            newVertCapacity = 1;
        
        // Make capacity the next power of two
        while (newVertCapacity < vertexCount)
            newVertCapacity <<= 1;
    }
    else if (vertexCount < mVertexBufferCapacity>>1) {
        // Make capacity the previous power of two
        while (vertexCount < newVertCapacity>>1)
            newVertCapacity >>= 1;
    }
    if (newVertCapacity != mVertexBufferCapacity) 
    {
        mVertexBufferCapacity = newVertCapacity;
        // Create new vertex buffer
        mVertexBuffer =
        HardwareBufferManager::getSingleton().createVertexBuffer(
                                                                 mRenderOp.vertexData->vertexDeclaration->getVertexSize(0),
                                                                 mVertexBufferCapacity,
                                                                 HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY); // TODO: Custom HBU_?
        
        // Bind buffer
        mRenderOp.vertexData->vertexBufferBinding->setBinding(0, mVertexBuffer);
    }
    // Update vertex count in the render operation
    mRenderOp.vertexData->vertexCount = vertexCount;
    
    if (mRenderOp.useIndexes)
    {
        OgreAssert(indexCount <= std::numeric_limits<unsigned long>::max(), "indexCount exceeds 32 bit");
        
        size_t newIndexCapacity = mIndexBufferCapacity;
        // Prepare index buffer
        if ((indexCount > newIndexCapacity) ||
            (!newIndexCapacity))
        {
            // indexCount exceeds current capacity!
            // It is necessary to reallocate the buffer.
            
            // Check if this is the first call
            if (!newIndexCapacity)
                newIndexCapacity = 1;
            
            // Make capacity the next power of two
            while (newIndexCapacity < indexCount)
                newIndexCapacity <<= 1;
            
        }
        else if (indexCount < newIndexCapacity>>1) 
        {
            // Make capacity the previous power of two
            while (indexCount < newIndexCapacity>>1)
                newIndexCapacity >>= 1;
        }
        
        if (newIndexCapacity != mIndexBufferCapacity)
        {
            mIndexBufferCapacity = newIndexCapacity;
            // Create new index buffer
            mIndexBuffer =
            HardwareBufferManager::getSingleton().createIndexBuffer(
                                                                    HardwareIndexBuffer::IT_32BIT,
                                                                    mIndexBufferCapacity,
                                                                    HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY); // TODO: Custom HBU_?
            mRenderOp.indexData->indexBuffer = mIndexBuffer;
        }
        
        // Update index count in the render operation
        mRenderOp.indexData->indexCount = indexCount;
    }
}

Real DynamicRenderable::getBoundingRadius(void) const
{
    return Math::Sqrt(std::max(mBox.getMaximum().squaredLength(), mBox.getMinimum().squaredLength()));
}

Real DynamicRenderable::getSquaredViewDepth(const Camera* cam) const
{
    Vector3 vMin, vMax, vMid, vDist;
    vMin = mBox.getMinimum();
    vMax = mBox.getMaximum();
    vMid = ((vMax - vMin) * 0.5) + vMin;
    vDist = cam->getDerivedPosition() - vMid;
    
    return vDist.squaredLength();
}

};