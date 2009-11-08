/*
 *  SimpleFrustum.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 26/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "SimpleFrustum.h"

namespace NFSpace {

SimpleFrustum::SimpleFrustum()
{
}

SimpleFrustum::~SimpleFrustum()
{
}

void SimpleFrustum::setModelViewProjMatrix(Matrix4 m)
{
    // Left clipping plane
    mPlanes[0].normal.x = m[3][0] + m[0][0];
    mPlanes[0].normal.y = m[3][1] + m[0][1];
    mPlanes[0].normal.z = m[3][2] + m[0][2];
    mPlanes[0].d        = m[3][3] + m[0][3];
    
    // Right clipping plane
    mPlanes[1].normal.x = m[3][0] - m[0][0];
    mPlanes[1].normal.y = m[3][1] - m[0][1];
    mPlanes[1].normal.z = m[3][2] - m[0][2];
    mPlanes[1].d        = m[3][3] - m[0][3];
    
    // Top clipping plane
    mPlanes[2].normal.x = m[3][0] - m[1][0];
    mPlanes[2].normal.y = m[3][1] - m[1][1];
    mPlanes[2].normal.z = m[3][2] - m[1][2];
    mPlanes[2].d        = m[3][3] - m[1][3];
    
    // Bottom clipping plane
    mPlanes[3].normal.x = m[3][0] + m[1][0];
    mPlanes[3].normal.y = m[3][1] + m[1][1];
    mPlanes[3].normal.z = m[3][2] + m[1][2];
    mPlanes[3].d        = m[3][3] + m[1][3];
    
    // Near clipping plane
    mPlanes[4].normal.x = m[3][0] + m[2][0];
    mPlanes[4].normal.y = m[3][1] + m[2][1];
    mPlanes[4].normal.z = m[3][2] + m[2][2];
    mPlanes[4].d        = m[3][3] + m[2][3];
    
    // Far clipping plane
    mPlanes[5].normal.x = m[3][0] - m[2][0];
    mPlanes[5].normal.y = m[3][1] - m[2][1];
    mPlanes[5].normal.z = m[3][2] - m[2][2];
    mPlanes[5].d        = m[3][3] - m[2][3];
    
    // we do need to normalize the planes because
    // we need to get real distances to compare bounding spheres
    for(int i = 0; i < 6; ++i) {
        mPlanes[i].normalise();
    }
}

};