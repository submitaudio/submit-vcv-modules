#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;
    p->addModel(modelDrift13);
	p->addModel(modelImpact);
	p->addModel(modelChrono);
}
