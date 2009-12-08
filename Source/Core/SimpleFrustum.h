/*
 *  SimpleFrustum.h
 *  NFSpace
 *
 *  Source: http://www.ogre3d.org/wiki/index.php/Frustum_Culling_In_Object_Space
 *
 *  Created by Steven Wittens on 26/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef SimpleFrustum_H
#define SimpleFrustum_H

#include <Ogre/Ogre.h>

using namespace Ogre;

namespace NFSpace {

class SimpleFrustum
    {
    public:
        SimpleFrustum();
        ~SimpleFrustum();
        
        void setModelViewProjMatrix(Matrix4 m);

        inline bool isVisible(const AxisAlignedBox& bound) {
            // Get centre of the box
            Vector3 centre = bound.getCenter();
            // Get the half-size of the box
            Vector3 halfSize = bound.getHalfSize();
            
            // For each plane, see if all points are on the negative side
            // If so, object is not visible
            for (int plane = 0; plane < 6; ++plane) {
                Plane::Side side = mPlanes[plane].getSide(centre, halfSize);
                if (side == Plane::NEGATIVE_SIDE) {
                    return false;
                }
                
            }
            return true;
        }
        
        inline bool isVisible(const Sphere* s) {
            Vector3 position = s->getCenter();
            Real radius      = s->getRadius();
            
            for(int i = 0; i < 6; ++i) {
                if(mPlanes[i].getDistance(position) < -radius) {
                    return false;
                }
            }
            
            return true;
        }
        
    private:
        Plane mPlanes[6];
    };

};

#endif