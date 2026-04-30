// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// Licensed under GPL v3 — see LICENSE file for details
// https://github.com/submitaudio/submit-vcv-modules

#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	p->addModel(modelDrift);
	p->addModel(modelChrono);
	p->addModel(modelChain);
	p->addModel(modelSqueeze);
	p->addModel(modelShape);
	p->addModel(modelMaster);
	pluginInstance = p;

	// Add modules here
	// p->addModel(modelMyModule);
	p->addModel(modelImpact);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
