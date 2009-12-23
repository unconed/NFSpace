/*
 *  PlanetBrush.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 20/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "PlanetBrush.h"

#include "Utility.h"

namespace NFSpace {

int PlanetBrush::sInstances = 0;
VertexData* PlanetBrush::sVertexData;

PlanetBrush::PlanetBrush() : SimpleRenderable() {
    addInstance();
    initRenderOp();
}

PlanetBrush::~PlanetBrush() {
    removeInstance();
}

void PlanetBrush::initRenderOp() {    
    mRenderOp.operationType = RenderOperation::OT_TRIANGLE_STRIP;
    mRenderOp.useIndexes = FALSE;
    mRenderOp.vertexData = sVertexData;    

    setBoundingBox(AxisAlignedBox(Vector3(-getBoundingRadius()), Vector3(getBoundingRadius())));

    setMaterial("Planet/BrushCarverNoisy");

    /*
     param_named_auto carveIntensity custom 1
     param_named_auto noiseIntensity custom 2
     param_named_auto noiseScale custom 3
     param_named_auto noiseOffset custom 4
        */
	setCustomParameter(1, Vector4((randf() * .5 - .25), 0, 0, 0));
	setCustomParameter(2, Vector4(0.05, 0, 0, 0));
    setCustomParameter(3, Vector4(0.25, 0, 0, 0));
	setCustomParameter(4, Vector4(randf() * 2.0 - 1.0, randf() * 2.0 - 1.0, 0, 0));
}

void PlanetBrush::addInstance() {
    if (!sInstances++) {
        sVertexData = new VertexData();

        VertexDeclaration *vertexDeclaration = sVertexData->vertexDeclaration;
        size_t offset = 0;
        vertexDeclaration->addElement(0, offset, VET_FLOAT3, VES_POSITION);
        offset += VertexElement::getTypeSize(VET_FLOAT3);
        vertexDeclaration->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
        offset += VertexElement::getTypeSize(VET_FLOAT2);
        
        HardwareVertexBufferSharedPtr vertexBuffer = HardwareBufferManager::getSingleton()
            .createVertexBuffer(offset, 4, HardwareBuffer::HBU_STATIC_WRITE_ONLY);

        sVertexData->vertexBufferBinding->setBinding(0, vertexBuffer);

        const VertexElement* poselem = vertexDeclaration->findElementBySemantic(VES_POSITION);
        const VertexElement* texelem = vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES, 0);

        unsigned char* pBase = static_cast<unsigned char*>(vertexBuffer->lock(HardwareBuffer::HBL_DISCARD));

        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                float* pPos;
                float* pTex;
                poselem->baseVertexPointerToElement(pBase, &pPos);
                texelem->baseVertexPointerToElement(pBase, &pTex);
              
                *pPos++ = x * 2 - 1;
                *pPos++ = y * 2 - 1;
                *pPos++ = 0;
                *pTex++ = x;
                *pTex++ = y;

                pBase += vertexBuffer->getVertexSize();
            }
        }
        
        sVertexData->vertexCount = 4;

        vertexBuffer->unlock();
    }
}

void PlanetBrush::removeInstance() {
    if (!--sInstances) {
        delete sVertexData;
    }
}

const String& PlanetBrush::getMovableType(void) const {
    static String movType = "PlanetBrush";
    return movType;
}

Real PlanetBrush::getBoundingRadius() const {
    return 1000.f;
}

Real PlanetBrush::getSquaredViewDepth(const Camera* cam) const {
    return 1;
}

};