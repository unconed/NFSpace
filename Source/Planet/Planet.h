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
    class QuadTreeNodeCompare;
    class PlanetRenderable;

namespace Planet {
    enum Face { RIGHT, LEFT, TOP, BOTTOM, FRONT, BACK };
    
    void doMaintenance();
};
    
    struct PlanetStats {
        static int totalNodes;
        static int totalOpenNodes;
        static int totalPagedOut;
        static int totalTiles;
        static int totalRenderables;
        static int requestQueue;
        static int hotTiles;
        static int renderedRenderables;
        static int gpuMemoryUsage;
    };
    
};

#endif