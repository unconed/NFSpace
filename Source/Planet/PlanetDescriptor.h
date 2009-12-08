/*
 *  PlanetDescriptor.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 23/11/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PlanetDescriptor_H
#define PlanetDescriptor_H

#include <Ogre/OgrePrerequisites.h>

using namespace Ogre;

namespace NFSpace {
    
    /**
     * Collection of all properties that define a unique planet.
     */
    class PlanetDescriptor {
        public:
            String script;
            int seed;
            int brushes;
            
            int lodLimit;
            Real radius;
            Real height;
    };

}

#endif