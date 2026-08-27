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
	p->addModel(modelSumM4);
	p->addModel(modelSumS4);
	p->addModel(modelTag);
	p->addModel(modelSet);
	p->addModel(modelPulse);

	p->addModel(modelSqueeze);
	p->addModel(modelShape);
	p->addModel(modelMaster);
	p->addModel(modelGain);
	p->addModel(modelSweep);
	p->addModel(modelLoop);
	p->addModel(modelClang);
	p->addModel(modelReact);
	p->addModel(modelSync);
	p->addModel(modelFlip);
	p->addModel(modelOrbit);

	p->addModel(modelCircles);
	p->addModel(modelSub);
	pluginInstance = p;
}
