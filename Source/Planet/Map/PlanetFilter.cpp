/*
 *  PlanetFilter.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 6/09/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "PlanetFilter.h"

#include "PlanetCube.h"
#include "EngineState.h"

using namespace Ogre;

namespace NFSpace {
    
    PlanetFilter::PlanetFilter(int face, int size, int border) : SimpleRenderable() {
        initRenderOp(face);
        initVertexData(face, size, border);

        Real height = getReal("planet.height");
        Real radius = getReal("planet.radius");
        
        // SampleDistance
        setCustomParameter(1, Vector4(2.0 / size, 0, 0, 0));
        // inverseSampleDistance
        setCustomParameter(2, Vector4(size * .5, 0, 0, 0));
        // heightScale
        setCustomParameter(3, Vector4(height / radius, 0, 0, 0));

        Matrix3 faceTransform = PlanetCube::getFaceTransform(face, false);
        setCustomParameter(4, Vector4(faceTransform[0][0], faceTransform[0][1], faceTransform[0][2], 0));
        setCustomParameter(5, Vector4(faceTransform[1][0], faceTransform[1][1], faceTransform[1][2], 0));
        setCustomParameter(6, Vector4(faceTransform[2][0], faceTransform[2][1], faceTransform[2][2], 0));
    }
    
    PlanetFilter::~PlanetFilter() {
        delete mRenderOp.vertexData;
    }
    
    void PlanetFilter::initRenderOp(int face) {    
        mRenderOp.operationType = RenderOperation::OT_TRIANGLE_STRIP;
        mRenderOp.useIndexes = FALSE;
        mRenderOp.vertexData = new VertexData();
        
        setBoundingBox(AxisAlignedBox(Vector3(-getBoundingRadius()), Vector3(getBoundingRadius())));
    }
    
    
    void PlanetFilter::initVertexData(int face, int size, int border) {
        // Prepare new vertex buffer
        VertexData* vertexData = mRenderOp.vertexData;
        
        VertexDeclaration *vertexDeclaration = vertexData->vertexDeclaration;
        size_t offset = 0;
        vertexDeclaration->addElement(0, offset, VET_FLOAT3, VES_POSITION);
        offset += VertexElement::getTypeSize(VET_FLOAT3);
        vertexDeclaration->addElement(0, offset, VET_FLOAT3, VES_TEXTURE_COORDINATES, 0);
        offset += VertexElement::getTypeSize(VET_FLOAT3);
        vertexDeclaration->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 1);
        offset += VertexElement::getTypeSize(VET_FLOAT2);
        
        HardwareVertexBufferSharedPtr vertexBuffer = HardwareBufferManager::getSingleton()
        .createVertexBuffer(offset, 4, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
        
        vertexData->vertexBufferBinding->setBinding(0, vertexBuffer);
        vertexData->vertexCount = 0;
        
        // Get offsets / pointers
        const VertexElement* poselem = vertexDeclaration->findElementBySemantic(VES_POSITION);
        const VertexElement* texelem1 = vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES, 0);
        const VertexElement* texelem2 = vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES, 1);
        
        unsigned char* pBase = static_cast<unsigned char*>(vertexBuffer->lock(HardwareBuffer::HBL_DISCARD));
        
        Real scaleAdjust = 1.f / (size + 2 * border);
        Real pixelUV = 2.f * scaleAdjust;
        Real uvScale = float(size) * scaleAdjust * float(size - 1) / float(size);

        Matrix3 faceTransform = PlanetCube::getFaceTransform(face, false);
        
        // Draw a full size quad.
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                float* pPos;
                float* pTex1;
                float* pTex2;
                poselem->baseVertexPointerToElement(pBase, &pPos);
                texelem1->baseVertexPointerToElement(pBase, &pTex1);
                texelem2->baseVertexPointerToElement(pBase, &pTex2);

                Vector3 point = Vector3(x ? 1.0f : -1.0f, y ? 1.0f : -1.0f, 1.0f);

                *pTex2++ = point.x;
                *pTex2++ = point.y;

                point = faceTransform * point;
                
                *pPos++ = point.x;
                *pPos++ = point.y;
                *pPos++ = point.z;
                
                point = point * uvScale;
                *pTex1++ = -point.x;
                *pTex1++ = -point.y;
                *pTex1++ = -point.z;

                pBase += vertexBuffer->getVertexSize();
            }
        }
        
        vertexData->vertexCount += 4;
        
        vertexBuffer->unlock();
    }
    
    const String& PlanetFilter::getMovableType(void) const {
        static String movType = "PlanetFilter";
        return movType;
    }
    
    Real PlanetFilter::getBoundingRadius() const {
        return 1;
    }
    
    Real PlanetFilter::getSquaredViewDepth(const Camera* cam) const {
        return 1;
    }
    
};
