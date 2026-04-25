#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
    pluginInstance = p;
    p->addModel(modelDrift);
	p->addModel(modelImpact);
	p->addModel(modelChrono);
}
