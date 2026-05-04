// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// Licensed under GPL v3 — see LICENSE file for details
// https://github.com/submitaudio/submit-vcv-modules

#include "plugin.hpp"
#include <cmath>

struct Drift13KnobMedium : SvgKnob {
    Drift13KnobMedium() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/Drift13KnobMedium.svg")));
        shadow->opacity = 0.f;
    }
};

struct Drift13KnobSmall : SvgKnob {
    Drift13KnobSmall() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/Drift13KnobSmall.svg")));
        shadow->opacity = 0.f;
    }
};

struct SweepKnob : SvgKnob {
    SweepKnob() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/ImpactKnobMini.svg")));
        shadow->opacity = 0.f;
    }
};

struct SweepButton : SvgSwitch {
    SweepButton() {
        momentary = true;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/knob-reset-off.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/knob-reset-on.svg")));
    }
};

// Biquad filter — bewezen stabiel
struct BiquadFilter {
    float b0=1,b1=0,b2=0,a1=0,a2=0;
    float x1=0,x2=0,y1=0,y2=0;

    float process(float x) {
        float y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;
        x2=x1; x1=x;
        y2=y1; y1=y;
        return y;
    }

    void setLP(float freq, float q, float sr) {
        float w0 = 2.f * float(M_PI) * freq / sr;
        float alpha = std::sin(w0) / (2.f * q);
        float cosw0 = std::cos(w0);
        float a0 = 1.f + alpha;
        b0 = (1.f - cosw0) / 2.f / a0;
        b1 = (1.f - cosw0) / a0;
        b2 = (1.f - cosw0) / 2.f / a0;
        a1 = -2.f * cosw0 / a0;
        a2 = (1.f - alpha) / a0;
    }

    void setHP(float freq, float q, float sr) {
        float w0 = 2.f * float(M_PI) * freq / sr;
        float alpha = std::sin(w0) / (2.f * q);
        float cosw0 = std::cos(w0);
        float a0 = 1.f + alpha;
        b0 =  (1.f + cosw0) / 2.f / a0;
        b1 = -(1.f + cosw0) / a0;
        b2 =  (1.f + cosw0) / 2.f / a0;
        a1 = -2.f * cosw0 / a0;
        a2 = (1.f - alpha) / a0;
    }

    void reset(float val=0) {
        x1=x2=y1=y2=val;
    }
};

struct Sweep : Module {
    enum ParamId {
        SWEEP_KNOB_PARAM,
        RES_PARAM,
        RESET_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        SWEEP_CV_INPUT,
        RES_CV_INPUT,
        RESET_CV_INPUT,
        CHAIN_L_INPUT,
        CHAIN_R_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        CHAIN_L_OUTPUT,
        CHAIN_R_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        RESET_LIGHT,
        LIGHTS_LEN
    };

    BiquadFilter lpL, lpR, hpL, hpR;
    float smoothSweep = 0.5f;
    float resetTimer = 0.f;
    float smoothRes   = 0.f;
    float lastFreq = -1.f;
    float lastQ = -1.f;
    bool lastIsLP = true;

    Sweep() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(SWEEP_KNOB_PARAM, 0.f, 1.f, 0.5f, "Sweep");
        configParam(RES_PARAM,        0.f, 1.f, 0.f,  "Resonance");
        configParam(RESET_PARAM,      0.f, 1.f, 0.f,  "Reset");
        configInput(SWEEP_CV_INPUT, "Sweep CV");
        configInput(RES_CV_INPUT,   "Resonance CV");
        configInput(RESET_CV_INPUT, "Reset CV");
        configInput(CHAIN_L_INPUT,  "Chain In L");
        configInput(CHAIN_R_INPUT,  "Chain In R");
        configOutput(CHAIN_L_OUTPUT, "Chain Out L");
        configOutput(CHAIN_R_OUTPUT, "Chain Out R");
    }

    void process(const ProcessArgs& args) override {
        // RESET
        bool resetActive = params[RESET_PARAM].getValue() > 0.5f;
        if (inputs[RESET_CV_INPUT].isConnected() && inputs[RESET_CV_INPUT].getVoltage() > 1.f)
            resetActive = true;

        if (resetActive) resetTimer = 0.30f;
        if (resetTimer > 0.f) {
            resetTimer -= args.sampleTime;
            float cur = params[SWEEP_KNOB_PARAM].getValue();
            float spd = 8.f * args.sampleTime;
            float next = cur + spd * (0.5f - cur);
            if (std::abs(next - 0.5f) < 0.05f) next = 0.5f;
            params[SWEEP_KNOB_PARAM].setValue(next);
            if (next == 0.5f) resetTimer = 0.f;
        }

        // Waarden
        float sweepVal = clamp(params[SWEEP_KNOB_PARAM].getValue() + (inputs[SWEEP_CV_INPUT].isConnected() ? inputs[SWEEP_CV_INPUT].getVoltage() / 10.f : 0.f), 0.f, 1.f);
        float resVal   = clamp(params[RES_PARAM].getValue()        + (inputs[RES_CV_INPUT].isConnected()   ? inputs[RES_CV_INPUT].getVoltage()   / 10.f : 0.f), 0.f, 1.f);

        // Smoothing
        smoothSweep += 0.002f * (sweepVal - smoothSweep);
        smoothRes   += 0.002f * (resVal   - smoothRes);

        // Input
        float inL = inputs[CHAIN_L_INPUT].getVoltage();
        float inR = inputs[CHAIN_R_INPUT].isConnected() ? inputs[CHAIN_R_INPUT].getVoltage() : inL;

        // Midden = bypass
        float deadzone = 0.01f;
        if (std::abs(smoothSweep - 0.5f) < deadzone) {
            outputs[CHAIN_L_OUTPUT].setVoltage(inL);
            outputs[CHAIN_R_OUTPUT].setVoltage(inR);
            lights[RESET_LIGHT].setBrightness(resetActive ? 1.f : 0.f);
            return;
        }

        float q = 0.707f + smoothRes * 4.f; // 0.707=vlak, 4.7=hoge resonantie

        if (smoothSweep < 0.5f) {
            // LP: 0=20Hz, 0.5=20kHz
            float t = smoothSweep * 2.f;
            float freq = clamp(20.f * std::pow(1000.f, t), 20.f, 20000.f);
            lpL.setLP(freq, q, args.sampleRate);
            lpR.setLP(freq, q, args.sampleRate);
            outputs[CHAIN_L_OUTPUT].setVoltage(lpL.process(inL));
            outputs[CHAIN_R_OUTPUT].setVoltage(lpR.process(inR));
        } else {
            // HP: 0.5=20Hz, 1=20kHz
            float t = (smoothSweep - 0.5f) * 2.f;
            float freq = clamp(20.f * std::pow(1000.f, t), 20.f, 20000.f);
            hpL.setHP(freq, q, args.sampleRate);
            hpR.setHP(freq, q, args.sampleRate);
            outputs[CHAIN_L_OUTPUT].setVoltage(hpL.process(inL));
            outputs[CHAIN_R_OUTPUT].setVoltage(hpR.process(inR));
        }

        lights[RESET_LIGHT].setBrightness(resetActive ? 1.f : 0.f);
    }

    json_t* dataToJson() override { return json_object(); }
    void dataFromJson(json_t* rootJ) override { (void)rootJ; }
};

struct SweepWidget : ModuleWidget {
    SweepWidget(Sweep* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Panel-design-sweep.svg")));

        addParam(createParamCentered<Drift13KnobMedium>(mm2px(Vec(26.83f, 35.00f)), module, Sweep::SWEEP_KNOB_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(26.75f, 58.94f)), module, Sweep::RES_PARAM));
        addParam(createParamCentered<SweepButton>(mm2px(Vec(26.91f, 75.59f)), module, Sweep::RESET_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.33f, 35.37f)), module, Sweep::SWEEP_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.33f, 59.03f)), module, Sweep::RES_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.33f, 75.30f)), module, Sweep::RESET_CV_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.33f, 102.78f)), module, Sweep::CHAIN_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.33f, 116.30f)), module, Sweep::CHAIN_R_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(32.63f, 102.78f)), module, Sweep::CHAIN_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(32.63f, 116.30f)), module, Sweep::CHAIN_R_OUTPUT));

    }

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("submitaudio.nl", "", []() {
            system::openBrowser(SUBMIT_URL);
        }));
        menu->addChild(createMenuItem("Report a Bug", "", []() {
            system::openBrowser("https://github.com/submitaudio/submit-vcv-modules/issues");
        }));
    }
};

Model* modelSweep = createModel<Sweep, SweepWidget>("Sweep");
