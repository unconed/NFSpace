/*
 *  PlanetEdgeFixup.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 29/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PlanetEdgeFixup_H
#define PlanetEdgeFixup_H

#include <Ogre/Ogre.h>
#include <Ogre/OgreSimpleRenderable.h>

using namespace Ogre;

namespace NFSpace {
    
    class PlanetEdgeFixup : public SimpleRenderable {
        static int sEdgeFace[6][4];
        VertexData* sVertexData[6][4];
        
        void initVertexData(int face, int edge, int size, int border, int fix);
        void initRenderOp(int face, int edge);
    public:
        enum {
            FIX_BORDER = 1,
            FIX_SIDES = 2,
        };
        
        PlanetEdgeFixup(int face, int edge, int size, int border, int fix, MaterialPtr* materialList);
        ~PlanetEdgeFixup();
        
        virtual Real getBoundingRadius() const;
        virtual Real getSquaredViewDepth(const Camera* cam) const;
        virtual const String& getMovableType(void) const;
    };
    
};

#endif