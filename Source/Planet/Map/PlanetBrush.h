/*
 *  PlanetBrush.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 20/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PlanetBrush_H
#define PlanetBrush_H

#include <Ogre/Ogre.h>
#include <Ogre/OgreSimpleRenderable.h>

using namespace Ogre;

namespace NFSpace {

class PlanetBrush : public SimpleRenderable {
    static int sInstances;
    static VertexData* sVertexData;
    
    static void addInstance();
    static void removeInstance();
    
    void initRenderOp();
public:
    PlanetBrush();
    ~PlanetBrush();

    virtual Real getBoundingRadius() const;
    virtual Real getSquaredViewDepth(const Camera* cam) const;
    virtual const String& getMovableType(void) const;
};
    
};

#endif