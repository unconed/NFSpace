/*
 *  Planet.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 14/12/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */


#include "Planet.h"
#include "PlanetCube.h"

namespace NFSpace {
    
    int PlanetStats::totalNodes = 0;
    int PlanetStats::totalOpenNodes = 0;
    int PlanetStats::totalPagedOut = 0;
    int PlanetStats::totalTiles = 0;
    int PlanetStats::totalRenderables = 0;
    int PlanetStats::requestQueue = 0;
    int PlanetStats::renderedRenderables = 0;
    int PlanetStats::hotTiles = 0;
    int PlanetStats::gpuMemoryUsage = 0;

    namespace Planet {
        void doMaintenance() {
            PlanetCube::doMaintenance();
        }
    }
    
};