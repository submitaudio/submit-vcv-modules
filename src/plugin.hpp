#pragma once

#define SUBMIT_URL "https://submitaudio.nl"

#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
// extern Model* modelMyModule;
extern Model* modelDrift;
extern Model* modelChrono;
extern Model* modelImpact;

extern Model* modelChain;

extern Model* modelSqueeze;
extern Model* modelShape;
extern Model* modelMaster;
extern Model* modelGain;
