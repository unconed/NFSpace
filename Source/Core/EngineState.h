/*
 *  EngineState.h
 *  NFSpace
 *
 *  Created by Steven Wittens on 23/08/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef EngineState_H
#define EngineState_H

#include <Ogre/Ogre.h>
#include <OIS/OIS.h>

#include <map>
#include "PlanetMovable.h"
#include "ApplicationFrameListener.h"

namespace NFSpace {

using namespace Ogre;
using namespace std;

class EngineState : public Ogre::Singleton<EngineState> {
    
public:
    static EngineState* EngineState::getSingletonPtr(void);
    static EngineState& EngineState::getSingleton(void);

    enum Type {
        VALUE_TYPE_NULL,
        VALUE_TYPE_REAL,
        VALUE_TYPE_INT,
        VALUE_TYPE_STRING,
        VALUE_TYPE_BOOL,
    };
    
    class VariableValue {
        int mType;
        union {
            Real mReal;
            int mInt;
            bool mBool;
        } uValue;
        string mString;
        
    public:
         
        VariableValue()             : mType(VALUE_TYPE_NULL) { };
        VariableValue(Real value)   : mType(VALUE_TYPE_REAL) { uValue.mReal = value; };
        VariableValue(int value)    : mType(VALUE_TYPE_INT)  { uValue.mInt = value;  };
        VariableValue(bool value)   : mType(VALUE_TYPE_BOOL) { uValue.mBool = value; };
        VariableValue(string value) : mType(VALUE_TYPE_STRING), mString(value)    { };

        const bool isNull() const {
            return mType == VALUE_TYPE_NULL;
        };
        
        const Real getRealValue() const {
            switch (mType) {
                default:
                case VALUE_TYPE_NULL:
                    return NAN;
                case VALUE_TYPE_REAL:
                    return uValue.mReal;
                case VALUE_TYPE_INT:
                    return uValue.mInt;
                case VALUE_TYPE_BOOL:
                    return uValue.mBool ? 1.0 : 0.0;
                case VALUE_TYPE_STRING:
                    return atof(mString.c_str());
            }
        }

        const int getIntValue() const {
            switch (mType) {
                default:
                case VALUE_TYPE_NULL:
                    return NAN;
                case VALUE_TYPE_REAL:
                    return uValue.mReal;
                case VALUE_TYPE_INT:
                    return uValue.mInt;
                case VALUE_TYPE_BOOL:
                    return uValue.mBool ? 1 : 0;
                case VALUE_TYPE_STRING:
                    return atoi(mString.c_str());
            }
        }
        
        const bool getBoolValue() const {
            switch (mType) {
                default:
                case VALUE_TYPE_NULL:
                    return false;
                case VALUE_TYPE_REAL:
                    return uValue.mReal != 0;
                case VALUE_TYPE_INT:
                    return uValue.mInt != 0;
                case VALUE_TYPE_BOOL:
                    return uValue.mBool;
                case VALUE_TYPE_STRING:
                    return !mString.compare("True");
            }
        }
        
        const string getStringValue() const {
            std::ostringstream str;
            switch (mType) {
                default:
                case VALUE_TYPE_NULL:
                    return "Null";
                case VALUE_TYPE_REAL:
                    str << uValue.mReal;
                    return str.str();
                case VALUE_TYPE_INT:
                    str << uValue.mInt;
                    return str.str();
                case VALUE_TYPE_BOOL:
                    return uValue.mBool ? "True" : "False";
                case VALUE_TYPE_STRING:
                    return mString;
            };
        };
    };

    void go();
    EngineState();
    ~EngineState();
    
    void dumpValues();
    
    typedef map<string, VariableValue> EngineStore;

    NameValuePairList* getOgreRootOptions();

    const VariableValue& getValue(const string key) const {
        EngineStore::const_iterator it = mKeyValueStore.find(key);
        return (it != mKeyValueStore.end()) ? it->second : mNull;
    };
    
    void _setValue(const string key, VariableValue &value) {
        //std::ostringstream str;
        //str << "Setting value " << key << " to " << value.getStringValue();
        //LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, str.str());
        
        EngineStore::iterator it = mKeyValueStore.find(key);
        
        if (it != mKeyValueStore.end()) {
            mKeyValueStore.erase(it);
        }
        mKeyValueStore.insert(std::pair<string, VariableValue>(key, value));
    }
    
    void setValue(const string key, VariableValue &value) {
        _setValue(key, value);
    };

    void setValue(const string key, VariableValue value) {
        _setValue(key, value);
    };
    
    const bool isNull(const string key) const {
        return getValue(key).isNull();
    };
    
    const Real getRealValue(const string key) const {
        return getValue(key).getRealValue();
    }

    const int getIntValue(const string key) const {
        return getValue(key).getIntValue();
    }
    
    const bool getBoolValue(const string key) const {
        return getValue(key).getBoolValue();
    }
    
    const string getStringValue(const string key) const {
        return getValue(key).getStringValue();
    }

private:
    EngineStore mKeyValueStore; 
    NameValuePairList mOgreRootOptions;
    
    VariableValue mNull;
    
};

inline bool getBool(string key) {
    return EngineState::getSingleton().getBoolValue(key);
};

inline Real getReal(string key) {
    return EngineState::getSingleton().getRealValue(key);
};

inline string getString(string key) {
    return EngineState::getSingleton().getStringValue(key);
};

inline int getInt(string key) {
    return EngineState::getSingleton().getIntValue(key);
};


};

#endif