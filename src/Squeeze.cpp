// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// SPDX-License-Identifier: GPL-3.0-or-later
// https://github.com/submitaudio/submit-vcv-modules

#include "plugin.hpp"
#include <algorithm>
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

    float gateEnvelope = 0.f;
    float audioEnvelope = 0.f;
    float amountSmooth = 1.f;
    float attackCoeff = 1.f;
    float releaseCoeff = 1.f;
    float amountCoeff = 1.f;
    float cachedAttack = -1.f;
    float cachedRelease = -1.f;
    float cachedSampleRate = 0.f;
    bool gateHigh = false;

    Squeeze() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(ATTACK_PARAM,  0.001f, 1.f, 0.01f, "Attack", " s");
        configParam(RELEASE_PARAM, 0.01f, 2.f, 0.1f,  "Release", " s");
        configParam(AMOUNT_PARAM,  0.f, 1.f, 1.f,     "Amount");
        configSwitch(CONTOUR_PARAM, 0.f, 2.f, 0.f, "Contour", {"Lin", "Exp", "Log"});
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

    static float smoothingCoefficient(float seconds, float sampleRate) {
        return 1.f - std::exp(-1.f / (std::max(seconds, 1.0e-5f) * sampleRate));
    }

    static float sanitizeVoltage(float voltage) {
        return std::isfinite(voltage) ? voltage : 0.f;
    }

    void updateCoefficients(float attack, float release, float sampleRate) {
        if (attack == cachedAttack && release == cachedRelease && sampleRate == cachedSampleRate)
            return;

        cachedAttack = attack;
        cachedRelease = release;
        cachedSampleRate = sampleRate;
        attackCoeff = smoothingCoefficient(attack, sampleRate);
        releaseCoeff = smoothingCoefficient(release, sampleRate);
        amountCoeff = smoothingCoefficient(0.01f, sampleRate);
    }

    static void followEnvelope(float target, float& envelope,
                               float attackCoefficient, float releaseCoefficient) {
        float coefficient = target > envelope ? attackCoefficient : releaseCoefficient;
        envelope += coefficient * (target - envelope);
        if (std::abs(envelope) < 1.0e-8f)
            envelope = 0.f;
    }

    void onReset() override {
        gateEnvelope = 0.f;
        audioEnvelope = 0.f;
        amountSmooth = 1.f;
        cachedAttack = -1.f;
        cachedRelease = -1.f;
        cachedSampleRate = 0.f;
        gateHigh = false;
    }

    void process(const ProcessArgs& args) override {
        bool gateConnected  = inputs[GATE_INPUT].isConnected();
        bool audioConnected = inputs[AUDIO_INPUT].isConnected();

        float attack  = params[ATTACK_PARAM].getValue();
        float release = params[RELEASE_PARAM].getValue();
        float amount  = params[AMOUNT_PARAM].getValue();
        int   contour = (int)params[CONTOUR_PARAM].getValue();

        updateCoefficients(attack, release, args.sampleRate);
        amountSmooth += amountCoeff * (amount - amountSmooth);

        float gateTarget = 0.f;
        if (gateConnected) {
            float gateVoltage = sanitizeVoltage(inputs[GATE_INPUT].getVoltage());
            if (gateHigh) {
                if (gateVoltage < 0.1f)
                    gateHigh = false;
            } else if (gateVoltage > 1.f) {
                gateHigh = true;
            }
            gateTarget = gateHigh ? 1.f : 0.f;
        } else {
            gateHigh = false;
        }

        float audioTarget = 0.f;
        if (audioConnected) {
            float audioVoltage = sanitizeVoltage(inputs[AUDIO_INPUT].getVoltage());
            audioTarget = clamp(std::abs(audioVoltage) / 5.f, 0.f, 1.f);
        }

        followEnvelope(gateTarget, gateEnvelope, attackCoeff, releaseCoeff);
        followEnvelope(audioTarget, audioEnvelope, attackCoeff, releaseCoeff);

        gateEnvelope = clamp(gateEnvelope, 0.f, 1.f);
        audioEnvelope = clamp(audioEnvelope, 0.f, 1.f);
        float envelope = std::max(gateEnvelope, audioEnvelope);

        float shaped = applyContour(envelope, contour);
        outputs[COMP_OUTPUT].setVoltage(shaped * clamp(amountSmooth, 0.f, 1.f) * 10.f);
    }
};

struct SqueezeKnobSmall : SvgKnob {
    SqueezeKnobSmall() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobSmall.svg")));
        shadow->opacity = 0.f;
    }
};

struct SqueezeKnobMedium : SvgKnob {
    SqueezeKnobMedium() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobMedium.svg")));
        shadow->opacity = 0.f;
    }
};

struct SqueezeKnob : SvgKnob {
    SqueezeKnob() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobMini.svg")));
        shadow->opacity = 0.f;
    }
};

struct SqueezeWidget : SubmitModuleWidget {
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

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Manual", "", []() {
            system::openBrowser("https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/squeeze/");
        }));
        menu->addChild(createMenuItem("submitaudio.nl", "", []() {
            system::openBrowser(SUBMIT_URL);
        }));
        menu->addChild(createMenuItem("Report a Bug", "", []() {
            system::openBrowser("https://github.com/submitaudio/submit-vcv-modules/issues");
        }));
		SubmitModuleWidget::appendContextMenu(menu);
    }
};


Model* modelSqueeze = createModel<Squeeze, SqueezeWidget>("Squeeze");
