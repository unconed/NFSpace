/*
 *  PlanetEdgeFixup.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 29/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "PlanetEdgeFixup.h"
#include "Planet.h"
#include "PlanetMap.h"
#include "PlanetCube.h"

namespace NFSpace {
    
    /**
     * Adjacent face mappings. In order of down, up, right, left.
     */
    int PlanetEdgeFixup::sEdgeFace[6][4] = {
    // Right
    Planet::BOTTOM, Planet::TOP, Planet::FRONT, Planet::BACK,
    // Left
    Planet::BOTTOM, Planet::TOP, Planet::BACK, Planet::FRONT,
    // Top
    Planet::FRONT, Planet::BACK, Planet::LEFT, Planet::RIGHT,
    // Bottom
    Planet::BACK, Planet::FRONT, Planet::LEFT, Planet::RIGHT, 
    // Front
    Planet::BOTTOM, Planet::TOP, Planet::LEFT, Planet::RIGHT,
    // Back
    Planet::BOTTOM, Planet::TOP, Planet::RIGHT, Planet::LEFT,
    };

    
    PlanetEdgeFixup::PlanetEdgeFixup(int face, int edge, int size, int border, int fix, MaterialPtr* materialList) : SimpleRenderable() {
        initRenderOp(face, edge);
        initVertexData(face, edge, size, border, fix);
        setMaterial(materialList[sEdgeFace[face][edge]]->getName());
    }
    
    PlanetEdgeFixup::~PlanetEdgeFixup() {
    }
    
    void PlanetEdgeFixup::initRenderOp(int face, int edge) {    
        mRenderOp.operationType = RenderOperation::OT_TRIANGLE_STRIP;
        mRenderOp.useIndexes = FALSE;
        mRenderOp.vertexData = new VertexData();
        
        setBoundingBox(AxisAlignedBox(Vector3(-getBoundingRadius()), Vector3(getBoundingRadius())));
    }

    
    void PlanetEdgeFixup::initVertexData(int face, int edge, int size, int border, int fix) {
        // Prepare new vertex buffer
        VertexData* vertexData = mRenderOp.vertexData;
            
        VertexDeclaration *vertexDeclaration = vertexData->vertexDeclaration;
        size_t offset = 0;
        vertexDeclaration->addElement(0, offset, VET_FLOAT3, VES_POSITION);
        offset += VertexElement::getTypeSize(VET_FLOAT3);
        vertexDeclaration->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
        offset += VertexElement::getTypeSize(VET_FLOAT2);
        
        HardwareVertexBufferSharedPtr vertexBuffer = HardwareBufferManager::getSingleton()
        .createVertexBuffer(offset, 4, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
        
        vertexData->vertexBufferBinding->setBinding(0, vertexBuffer);
        vertexData->vertexCount = 0;
        
        // Get offsets / pointers
        const VertexElement* poselem = vertexDeclaration->findElementBySemantic(VES_POSITION);
        const VertexElement* texelem = vertexDeclaration->findElementBySemantic(VES_TEXTURE_COORDINATES, 0);
        
        unsigned char* pBase = static_cast<unsigned char*>(vertexBuffer->lock(HardwareBuffer::HBL_DISCARD));

        Real scaleAdjust = 1.f / (size + 2 * border);
        Real pixelXYZ = 2.f * scaleAdjust;
        Real pixelUV = 2.f * scaleAdjust;
        Real overflowXYZ = border * pixelXYZ;
        Real overflowUV = -pixelUV;
        Real uvScale = float(size) * scaleAdjust;
        
        // Definition of border vertices to fix up.
        Real borderPositions[4][2][2][3] = {
            -1.f, -1.f, -1.f + overflowXYZ,  1.f, -1.f,-1.f + overflowXYZ,  -1.f, -1.f,-1.f,   1.f, -1.f,-1.f,
             1.f, 1.f, -1.f + overflowXYZ,  -1.f, 1.f,-1.f + overflowXYZ,    1.f, 1.f,-1.f,   -1.f, 1.f,-1.f,
            -1.f, 1.f, -1.f + overflowXYZ,  -1.f, -1.f,-1.f + overflowXYZ,  -1.f, 1.f,-1.f,   -1.f, -1.f,-1.f,
             1.f,-1.f, -1.f + overflowXYZ,   1.f, 1.f,-1.f + overflowXYZ,    1.f,-1.f,-1.f,    1.f, 1.f,-1.f,
        };

        // Definition of overlap vertices to fix up.
        Real overlapPositions[4][2][2][3] = {
            -1.f, -1.f,-1.f,   1.f, -1.f,-1.f,  -1.f, -1.f, -1.f + overflowUV,  1.f, -1.f,-1.f + overflowUV,  
            1.f, 1.f,-1.f,   -1.f, 1.f,-1.f,    1.f, 1.f, -1.f + overflowUV,  -1.f, 1.f,-1.f + overflowUV,    
            -1.f, 1.f,-1.f,   -1.f, -1.f,-1.f,  -1.f, 1.f, -1.f + overflowUV,  -1.f, -1.f,-1.f + overflowUV,  
            1.f,-1.f,-1.f,    1.f, 1.f,-1.f,    1.f,-1.f, -1.f + overflowUV,   1.f, 1.f,-1.f + overflowUV,    
        };
        
        // Hackity hack. Something's going wrong here with left vs right handed.
        Matrix3 fixAlign;
        switch (face >> 1) {
            default:
            case 0:
                fixAlign = Matrix3(1, 0, 0,
                                   0,-1, 0,
                                   0, 0,-1);
                break;
            case 1:
                fixAlign = Matrix3(1, 0, 0,
                                   0,-1, 0,
                                   0, 0, 1);
                break;
        }

        // Find the transform from the current face to the adjacent one.
        Matrix3 faceTransform = PlanetCube::getFaceTransform(face);
        Matrix3 edgeTransform = PlanetCube::getFaceTransform(sEdgeFace[face][edge]);
        edgeTransform = fixAlign * (edgeTransform.Transpose() * faceTransform);
        
        if (fix & FIX_SIDES) {
            // Draw the skewed fix-up quad for the sides.
            for (int y = 0; y < 2; ++y) {
                for (int x = 0; x < 2; ++x) {
                    float* pPos;
                    float* pTex;
                    poselem->baseVertexPointerToElement(pBase, &pPos);
                    texelem->baseVertexPointerToElement(pBase, &pTex);
                    
                    *pPos++ = borderPositions[edge][y][x][0];
                    *pPos++ = borderPositions[edge][y][x][1];
                    *pPos++ = borderPositions[edge][y][x][2];
                    
                    Vector3 uvw = edgeTransform * Vector3(borderPositions[edge][y][x][0],
                                                          borderPositions[edge][y][x][1],
                                                          borderPositions[edge][y][x][2]) * uvScale;
                    
                    *pTex++ = uvw.x * .5f + .5f;
                    *pTex++ = uvw.y * .5f + .5f;
                    
                    pBase += vertexBuffer->getVertexSize();
                }
            }
            
            vertexData->vertexCount += 4;
        }
        if (fix & FIX_BORDER) {
            // Draw the 1px overlapping edge for perfect wrapping.
            for (int y = 0; y < 2; ++y) {
                for (int x = 0; x < 2; ++x) {
                    float* pPos;
                    float* pTex;
                    poselem->baseVertexPointerToElement(pBase, &pPos);
                    texelem->baseVertexPointerToElement(pBase, &pTex);
                    
                    *pPos++ = overlapPositions[edge][y][x][0];
                    *pPos++ = overlapPositions[edge][y][x][1];
                    *pPos++ = overlapPositions[edge][y][x][2];
                    
                    Vector3 uvw = edgeTransform * Vector3(overlapPositions[edge][y][x][0],
                                                          overlapPositions[edge][y][x][1],
                                                          -1.f) * uvScale;
                    
                    *pTex++ = uvw.x * .5f + .5f;
                    *pTex++ = uvw.y * .5f + .5f;
                    
                    pBase += vertexBuffer->getVertexSize();
                }
            }
            
            vertexData->vertexCount += 4;
        }
        
        vertexBuffer->unlock();
    }
    
    const String& PlanetEdgeFixup::getMovableType(void) const {
        static String movType = "PlanetEdgeFixup";
        return movType;
    }
    
    Real PlanetEdgeFixup::getBoundingRadius() const {
        return 1;
    }
    
    Real PlanetEdgeFixup::getSquaredViewDepth(const Camera* cam) const {
        return 1;
    }
    
};