#pragma once

#define SUBMIT_URL "https://submitaudio.nl"
#define SUBMIT_SUPPORT_URL "https://ko-fi.com/submitaudio"

#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

struct SubmitModuleWidget : ModuleWidget {
	void appendContextMenu(Menu* menu) override {
		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuItem("Support Submit Audio", "", []() {
			system::openBrowser(SUBMIT_SUPPORT_URL);
		}));
	}
};

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
extern Model* modelSweep;
extern Model* modelClang;
extern Model* modelReact;
extern Model* modelLoop;
extern Model* modelSync;
extern Model* modelFlip;
extern Model* modelOrbit;
