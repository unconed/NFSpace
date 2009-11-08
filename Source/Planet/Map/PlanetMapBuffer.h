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
     * Data structure for creating, loading and storing a base level cube-map for a planet surface.
     *
     * Implements drawing operations into itself using the provided scenemanager and camera.
     */
    class PlanetMapBuffer {
        int mSize;
        int mBorder;
        int mFullSize;
        int mType;
        Real mFill;
        
        SceneManager* mSceneManager;
        Camera* mCamera;
        
    public:
        static const unsigned int LEVEL_SHIFT;
        static const unsigned int LEVEL_MASK;
        static const unsigned int LEVEL_MIN;
        static const unsigned int LEVEL_MID;
        static const unsigned int LEVEL_MAX;
        static const unsigned int LEVEL_RANGE;
        
        enum {
            MAP_TYPE_MONO,
            MAP_TYPE_NORMAL,
        };
        
        enum {
            
        };
        
        PlanetMapBuffer(SceneManager* sceneManager, Camera* camera, int type, int size, int border, Real fill);
        ~PlanetMapBuffer();
        
        Image* getFace(int face);
        void render(SceneNode* brushes);
        void edgeFixup();
        void save(bool border, bool file = false);

        void prepareMaterial();
        std::string getMaterial();
        
        String getTextureName();

    protected:
        void init();
        void renderFace(int face, bool transform, unsigned int clearFrame);
        PixelFormat getPixelFormat();
        
        MaterialPtr mMaterialFaces[6];
        MaterialPtr mMaterial;
        bool mMaterialCreated;
        TexturePtr mTexture;
        TexturePtr mTextureFaces[6];
        RenderTexture* mRenderTexture[6];
        Image mImage[6];
    };
    
};

#endif