// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// Licensed under GPL v3 — see LICENSE file for details
// https://github.com/submitaudio/submit-vcv-modules

#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	p->addModel(modelDrift);
	p->addModel(modelChrono);
	p->addModel(modelImpact);
	p->addModel(modelChain);
	p->addModel(modelSqueeze);
	p->addModel(modelShape);
	p->addModel(modelMaster);
	pluginInstance = p;
}
