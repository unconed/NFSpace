/*
 *  Utility.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 20/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef Utility_H
#define Utility_H

#include <Ogre/Ogre.h>

using namespace Ogre;

namespace NFSpace {
    
    Ogre::Image cropImage(const Ogre::Image& source, size_t offsetX, size_t offsetY, size_t width, size_t height);
    
    inline int maxi(int a, int b) {
        return a > b ? a : b;
    }
    
    inline int mini(int a, int b) {
        return a < b ? a : b;
    }
    
    inline Real maxf(Real a, Real b) {
        return a > b ? a : b;
    }

    inline Real minf(Real a, Real b) {
        return a < b ? a : b;
    }

    inline float randf() {
        return float(rand()) / RAND_MAX;
    }

    inline bool isPowerOf2(int x) {
        return !(x & (x - 1));
    }
        
    inline std::string getUniqueId(std::string prefix) {
        static int sObjectId = 0;
        std::ostringstream name;
        name << prefix << (++sObjectId);
        return name.str();
    }
        
    inline void log(std::string str) {
        LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, str);
    }
        
};

#endif