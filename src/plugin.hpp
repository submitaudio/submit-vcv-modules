#pragma once

#define SUBMIT_URL "https://submitaudio.nl"
#define SUBMIT_SUPPORT_URL "https://ko-fi.com/submitaudio"
#define SUBMIT_CHANGELOG_URL "https://github.com/submitaudio/submit-vcv-modules/blob/master/CHANGELOG.md"
#define SUBMIT_CHANGELOG_BASE_URL "https://github.com/submitaudio/submit-vcv-modules/blob/master/docs/changelogs/"

#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

struct SubmitModuleWidget : ModuleWidget {
	void appendContextMenu(Menu* menu) override {
		std::string changelogUrl = SUBMIT_CHANGELOG_URL;
		if (model) {
			const std::string slug = model->slug;
			if (slug == "Drift" || slug == "Chrono" || slug == "Impact" ||
				slug == "Chain" || slug == "Squeeze" || slug == "Shape" ||
				slug == "Master" || slug == "Gain" || slug == "Sweep" ||
				slug == "Loop" || slug == "Clang" || slug == "React" ||
				slug == "Sync" || slug == "Flip" || slug == "Orbit") {
				changelogUrl = std::string(SUBMIT_CHANGELOG_BASE_URL) + slug + ".md";
			}
		}
		menu->addChild(createMenuItem("Changelog", "", [changelogUrl]() {
			system::openBrowser(changelogUrl);
		}));
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
