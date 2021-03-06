/*
 *  PlanetMapBuffer.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 29/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PlanetMapBuffer_H
#define PlanetMapBuffer_H

#include <Ogre/Ogre.h>

using namespace Ogre;

namespace NFSpace {
    
    typedef unsigned short HeightMapPixel[4];

    /**
     * Working buffer for creating maps for a planet surface.
     *
     * Executes drawing operations into itself using the provided scenemanager and camera.
     */
    class PlanetMapBuffer {
        int mSize;
        int mBorder;
        int mFullSize;
        Real mFill;
        
        SceneManager* mSceneManager;
        Camera* mCamera;
        
    public:
        static const unsigned int LEVEL_MIN;
        static const unsigned int LEVEL_RANGE;
        
        enum {
            MAP_TYPE_WORKSPACE,
            MAP_TYPE_HEIGHT,
            MAP_TYPE_NORMAL,
        };

        enum {
            FILTER_TYPE_NORMAL,
        };
        
        PlanetMapBuffer(SceneManager* sceneManager, Camera* camera, int size, int border, Real fill);
        ~PlanetMapBuffer();

        void render(int face, int lod, int x, int y, SceneNode* brushes);
        void filter(int face, int lod, int x, int y, int type, PlanetMapBuffer* source);
        TexturePtr saveTexture(bool border, int type);
        Image saveImage(bool border, int type);

        void prepareMaterial();
        std::string getMaterial();
        
        String getTextureName();
        TexturePtr mTexture;

    protected:
        void init();
        void renderTile(int face, int lod, int x, int y, bool transform, unsigned int clearFrame);
        static PixelFormat getPixelFormat(int type);
        
        RenderTexture* mRenderTexture;
    };
    
};

#endif