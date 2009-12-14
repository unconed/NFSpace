/*
 *  Planet.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 17/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef Planet_H
#define Planet_H

namespace NFSpace {

    class PlanetBrush;
    class PlanetEdgeFixup;
    class PlanetFilter;
    class PlanetMap;
    class PlanetMapBuffer;
    class PlanetMapTile;
    class PlanetCube;
    class QuadTree;
    class QuadTreeNode;
    class PlanetRenderable;

namespace Planet {
    enum Face { RIGHT, LEFT, TOP, BOTTOM, FRONT, BACK };
};
    
    struct PlanetStats {
        static int statsNodes;
        static int statsTiles;
        static int statsRenderables;
        static int gpuMemoryUsage;
    };
    
};

#endif