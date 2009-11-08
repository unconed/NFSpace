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