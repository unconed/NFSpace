/*
 *  EngineState.cpp
 *  NFSpace
 *
 *  Created by Steven Wittens on 23/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "EngineState.h"

namespace NFSpace {
    
template<> EngineState* Singleton<EngineState>::ms_Singleton = 0;

EngineState* EngineState::getSingletonPtr(void) {
    return ms_Singleton;
}

EngineState& EngineState::getSingleton(void) {  
    assert(ms_Singleton);  return (*ms_Singleton);  
}

EngineState::EngineState() {
    // Ogre root settings
    setValue("FSAA", 0);
    setValue("colourDepth", 32);
    setValue("vsync", false);

    // NF engine settings
    setValue("fullscreen", false);
    setValue("renderSystem", string("OpenGL"));
    setValue("screenWidth", 1024);
    setValue("screenHeight", 768);

    // Run-time values
    setValue("planet.lodDetail", 6.f);
    setValue("planet.lodLimit", 8);
    setValue("planet.radius", 60.0f);
    setValue("planet.height", 40.f);

    setValue("planet.gridSize", 17);    
    setValue("planet.textureSize", 257);    
    
    setValue("planet.pagerTimeSlot", 1.f);

    //setValue("planet.seed", 1007);    
    setValue("planet.seed", 32489);
//    setValue("planet.seed", (int)(time(0) & 0xFFFF));
    setValue("planet.brushes", 200);    
    
    //dumpValues();
};

EngineState::~EngineState() {
};

NameValuePairList* EngineState::getOgreRootOptions() {
    string keys[] = {
        "title",
        "colourDepth", // Notes: [W32 specific]
        "left",
        "top",
        "FSAA" // Values: 0,2,4,6,...
        "displayFrequency",
        "vsync",
        "border",  // Values: none, fixed, resize
        "outerDimensions",  // Values: true, false
        "gamma",  // Values: true, false
    };
    int n = sizeof(keys) / sizeof(keys[0]);

    mOgreRootOptions.clear();
    for (int i = 0; i < n; ++i) {
        const VariableValue &value = getValue(keys[i]);
        if (!value.isNull()) {
            mOgreRootOptions.insert(NameValuePairList::value_type(keys[i], value.getStringValue()));
        }
    }
    return &mOgreRootOptions;
};

void EngineState::dumpValues() {
    LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "EngineState");

    for (EngineStore::iterator it = mKeyValueStore.begin(); it != mKeyValueStore.end(); ++it) {
        std::ostringstream str;
        str << "  ";
        str << it->first;
        str << ": ";
        str << it->second.getStringValue();
        LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, str.str());
    }

};

};