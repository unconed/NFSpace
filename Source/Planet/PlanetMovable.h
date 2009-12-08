/*
 *  PlanetMovable.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 20/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PlanetMovable_H
#define PlanetMovable_H

#include <Ogre/OgrePrerequisites.h>
#include <Ogre/OgreMovableObject.h>
#include <Ogre/OgreSceneNode.h>
#include <Ogre/OgreMaterialManager.h>

#include "PlanetDescriptor.h"
#include "PlanetCube.h"
#include "PlanetMap.h"

using namespace Ogre;

namespace NFSpace {

/**
 * Movable entity representing a single planet object.
 *
 * This is the top-evel class that manages everything required for representing and rendering planet.
 *
 * Note: messy code ahead. Was a dumping ground for OGRE experimentation.
 */
class PlanetMovable : public MovableObject, public MovableObject::Listener {
    public:
        PlanetMovable(PlanetDescriptor descriptor);
        PlanetMovable(PlanetDescriptor descriptor, const String& name);
        virtual ~PlanetMovable();

        PlanetCube* mCube;
        PlanetMap* mMap;
    protected:
        static String MOVABLE_TYPE_NAME;
    
        PlanetDescriptor mDescriptor;
        AxisAlignedBox mBoundingBox;
        bool mInited;
    
    public:
        /**
         * Initialize the planet object.
         */
        void initObject();
    
        /**
         * Clean up the planet object.
         */
        void deleteObject();
    
        void refresh(PlanetDescriptor descriptor);
    
        /**
         * Callback: MovableObject has been destroyed.
         */
        virtual void objectDestroyed(MovableObject* movableObject);

        /**
         * Callback: MovableObject has been attached to a node.
         */
        virtual void objectAttached(MovableObject* movableObject);

        /**
         * Callback: MovableObject has been detached from a node.
         */
        virtual void objectDetached(MovableObject* movableObject);

        /**
         * Callback: MovableObject has been moved.
         */
        virtual void objectMoved(MovableObject* movableObject);
    
        virtual const String& getMovableType(void) const;
        virtual Real getBoundingRadius(void) const;
        virtual const AxisAlignedBox& getBoundingBox(void) const;
        virtual void _updateRenderQueue(RenderQueue* queue);
        virtual void visitRenderables(Renderable::Visitor* visitor, bool debugRenderables = false);

        virtual bool objectRendering(const MovableObject* movableObject, const Camera* camera);

        virtual void _notifyCurrentCamera(Camera* cam);
};

/**
 * Movable entity factory.
 */
class PlanetMovableFactory : public MovableObjectFactory {
    protected:
        MovableObject* createInstanceImpl(const String& name, const NameValuePairList* params);
    public:
        PlanetMovableFactory() {}
        ~PlanetMovableFactory() {}
        
        static String FACTORY_TYPE_NAME;
    
        static PlanetDescriptor getDefaultDescriptor();
        
        const String& getType(void) const;
        void destroyInstance(MovableObject* obj);
};
    
};

#endif