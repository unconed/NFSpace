/*
 *  PlanetFilter.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 6/09/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PlanetFilter_H
#define PlanetFilter_H

#include <Ogre/Ogre.h>
#include <Ogre/OgreSimpleRenderable.h>

using namespace Ogre;

namespace NFSpace {
    
    class PlanetFilter : public SimpleRenderable {
        VertexData* sVertexData;
        
        void initVertexData(int lod, int x, int y, int size, int border);
        void initRenderOp();
    public:
        enum {
            FILTER_NORMAL_MAP = 1,
        };
        
        PlanetFilter(int face, int lod, int x, int y, int size, int border);
        ~PlanetFilter();
        
        virtual Real getBoundingRadius() const;
        virtual Real getSquaredViewDepth(const Camera* cam) const;
        virtual const String& getMovableType(void) const;
    };
    
};

#endif