/*
 *  PlanetMovable.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 20/07/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "PlanetMovable.h"

namespace NFSpace {
    
String PlanetMovable::MOVABLE_TYPE_NAME = "PlanetMovable";

PlanetMovable::PlanetMovable() 
: MovableObject() {
    initObject();
}

PlanetMovable::PlanetMovable(const String& name)
: MovableObject(name) {
    initObject();
}

PlanetMovable::~PlanetMovable() {
    delete mCube;
    delete mMap;
}
    
void PlanetMovable::refresh() {
    delete mCube;
    delete mMap;
    initObject();
}

void PlanetMovable::initObject() {
    setListener(this);
    mMap = new PlanetMap();
    mCube = new PlanetCube(this, mMap);
    mBoundingBox = AxisAlignedBox(Vector3(-mCube->getScale()), Vector3(mCube->getScale()));
}

void PlanetMovable::_notifyCurrentCamera(Camera* camera) {
    mCube->setCamera(camera);
}

/**
 * Callback: MovableObject has been destroyed.
 */
void PlanetMovable::objectDestroyed(MovableObject* movableObject) {
}

/**
 * Callback: MovableObject has been attached to a node.
 */
void PlanetMovable::objectAttached(MovableObject* movableObject) {
    //initGraph();
}

/**
 * Callback: MovableObject has been detached from a node.
 */
void PlanetMovable::objectDetached(MovableObject* movableObject) {
    //destroyGraph();
}

/**
 * Callback: MovableObject has been moved.
 */
void PlanetMovable::objectMoved(MovableObject* movableObject) {
}

const String& PlanetMovable::getMovableType(void) const {
    return MOVABLE_TYPE_NAME;
}

Real PlanetMovable::getBoundingRadius(void) const {
    return 1;
}

const AxisAlignedBox& PlanetMovable::getBoundingBox(void) const 
{
    return mBoundingBox; 
}

void PlanetMovable::_updateRenderQueue(RenderQueue* queue) {
    if (mCube) {
        mCube->updateRenderQueue(queue, getParentSceneNode()->_getFullTransform());
    }
}

void PlanetMovable::visitRenderables(Renderable::Visitor* visitor, bool debugRenderables) {
}

bool PlanetMovable::objectRendering(const MovableObject* movableObject, const Camera* camera) {
    return true;
}

//////////////////////////////////////////////////////////////////////////////////

String PlanetMovableFactory::FACTORY_TYPE_NAME = "PlanetMovable";

const String& PlanetMovableFactory::getType(void) const {
    return FACTORY_TYPE_NAME;
}

MovableObject* PlanetMovableFactory::createInstanceImpl(const String& name, const NameValuePairList* params) {
    return new PlanetMovable(name);
}

void PlanetMovableFactory::destroyInstance(MovableObject* obj) {
    delete obj;
}

};
