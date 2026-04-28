#include "plugin.hpp"
#include <cmath>

struct Squeeze : Module {
    enum ParamId {
        ATTACK_PARAM,
        RELEASE_PARAM,
        AMOUNT_PARAM,
        CONTOUR_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        GATE_INPUT,
        AUDIO_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        COMP_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    float envelope = 0.f;

    Squeeze() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(ATTACK_PARAM,  0.001f, 1.f, 0.01f, "Attack", " s");
        configParam(RELEASE_PARAM, 0.01f, 2.f, 0.1f,  "Release", " s");
        configParam(AMOUNT_PARAM,  0.f, 1.f, 1.f,     "Amount");
        configSwitch(CONTOUR_PARAM, 0.f, 2.f, 0.f, "Contour", {"Log", "Exp", "Lin"});
        paramQuantities[CONTOUR_PARAM]->snapEnabled = true;
        configInput(GATE_INPUT,  "Gate In");
        configInput(AUDIO_INPUT, "Audio In");
        configOutput(COMP_OUTPUT, "Comp Out");
    }

    float applyContour(float x, int contour) {
        x = clamp(x, 0.f, 1.f);
        switch (contour) {
            case 0: return x;
            case 1: return x * x;
            case 2: return std::sqrt(x);
            case 3: return x * x * (3.f - 2.f * x);
            default: return x;
        }
    }

    void process(const ProcessArgs& args) override {
        bool gateConnected  = inputs[GATE_INPUT].isConnected();
        bool audioConnected = inputs[AUDIO_INPUT].isConnected();

        float attack  = params[ATTACK_PARAM].getValue();
        float release = params[RELEASE_PARAM].getValue();
        float amount  = params[AMOUNT_PARAM].getValue();
        int   contour = (int)params[CONTOUR_PARAM].getValue();

        float attackCoeff  = 1.f - std::exp(-1.f / (attack  * args.sampleRate));
        float releaseCoeff = 1.f - std::exp(-1.f / (release * args.sampleRate));

        float target = 0.f;

        if (audioConnected) {
            // Peak follower voor audio - instant attack, slow release
            float audio = std::abs(inputs[AUDIO_INPUT].getVoltage()) / 5.f;
            if (audio > 1.f) audio = 1.f;
            // Snelle attack (direct naar piek), langzame release via release knob
            if (audio > envelope) {
                envelope = audio; // instant attack
            } else {
                envelope += releaseCoeff * (0.f - envelope);
            }
        } else if (gateConnected) {
            // Gate mode: attack en release via knoppen
            float gate = inputs[GATE_INPUT].getVoltage();
            target = (gate > 1.f) ? 1.f : 0.f;
            if (target > envelope) {
                envelope += attackCoeff * (target - envelope);
            } else {
                envelope += releaseCoeff * (target - envelope);
            }
        }
        envelope = clamp(envelope, 0.f, 1.f);

        float shaped = applyContour(envelope, contour);
        outputs[COMP_OUTPUT].setVoltage(shaped * amount * 10.f);
    }
};

struct SqueezeKnobSmall : SvgKnob {
    SqueezeKnobSmall() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/DriftKnobSmall.svg")));
        shadow->opacity = 0.f;
    }
};

struct SqueezeKnobMedium : SvgKnob {
    SqueezeKnobMedium() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/Drift13KnobMedium.svg")));
        shadow->opacity = 0.f;
    }
};

struct SqueezeKnob : SvgKnob {
    SqueezeKnob() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/ImpactKnobMini.svg")));
        shadow->opacity = 0.f;
    }
};

struct SqueezeWidget : ModuleWidget {
    SqueezeWidget(Squeeze* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Squeeze.svg")));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.94f, 31.41f)), module, Squeeze::GATE_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.94f, 45.00f)), module, Squeeze::AUDIO_INPUT));

        addParam(createParamCentered<CKSSThree>(mm2px(Vec(23.44f, 32.88f)), module, Squeeze::CONTOUR_PARAM));

        addParam(createParamCentered<SqueezeKnobSmall>(mm2px(Vec(17.55f, 64.79f)), module, Squeeze::ATTACK_PARAM));
        addParam(createParamCentered<SqueezeKnobSmall>(mm2px(Vec(17.55f, 84.00f)), module, Squeeze::RELEASE_PARAM));
        addParam(createParamCentered<SqueezeKnob>(mm2px(Vec(17.52f, 100.32f)), module, Squeeze::AMOUNT_PARAM));

        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(17.47f, 116.16f)), module, Squeeze::COMP_OUTPUT));
    }
};

Model* modelSqueeze = createModel<Squeeze, SqueezeWidget>("Squeeze");
