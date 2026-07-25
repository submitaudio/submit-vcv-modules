// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// Licensed under GPL v3 — see LICENSE file for details
// https://github.com/submitaudio/submit-vcv-modules

#include "plugin.hpp"
#include <cmath>

// ── KNOPPEN ───────────────────────────────────────────────────────────────────

struct GainKnobLarge : SvgKnob {
    GainKnobLarge() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/Drift13KnobMedium.svg")));
        shadow->opacity = 0.f;
    }
};

struct GainKnobSmall : SvgKnob {
    GainKnobSmall() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/ImpactKnobMini.svg")));
        shadow->opacity = 0.f;
    }
};

// ── MODULE ────────────────────────────────────────────────────────────────────

struct GainModule : Module {
    enum ParamId {
        GAIN_PARAM,
        SEND1_PARAM,
        VOL_PARAM,
        MUTE_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        IN_L_INPUT,
        IN_R_INPUT,
        COMP_INPUT,
        MUTE_CV_INPUT,
        CHAIN_L_INPUT,
        CHAIN_R_INPUT,
        SEND1_CHAIN_INPUT,
        SEND1R_CHAIN_INPUT,
        RETURN1_L_INPUT,
        RETURN1_R_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        CHAIN_L_OUTPUT,
        CHAIN_R_OUTPUT,
        SEND1_CHAIN_OUTPUT,
        SEND1R_CHAIN_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        PEAK_R_LIGHT,
        PEAK_G_LIGHT,
        PEAK_B_LIGHT,
        LIGHTS_LEN
    };

    float peak = 0.f;
    float vuLevel = 0.f;
    static constexpr float PEAK_DECAY = 0.9995f;

    GainModule() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(GAIN_PARAM,  1.f, 10.f, 1.f, "Gain", "x");
        configParam(SEND1_PARAM, 0.f, 1.f,  0.f, "FX Send");
        configParam(VOL_PARAM,   0.f, 1.f,  0.75f, "Volume");
        configSwitch(MUTE_PARAM, 0.f, 1.f,  0.f, "Mute", {"On", "Mute"});

        configInput(IN_L_INPUT,          "Line Level L");
        configInput(IN_R_INPUT,          "Line Level R");
        configInput(COMP_INPUT,          "Comp/CV");
        configInput(MUTE_CV_INPUT,       "Mute CV");
        configInput(CHAIN_L_INPUT,       "Chain In L");
        configInput(CHAIN_R_INPUT,       "Chain In R");
        configInput(SEND1_CHAIN_INPUT,   "FX Chain In L");
        configInput(SEND1R_CHAIN_INPUT,  "FX Chain In R");
        configInput(RETURN1_L_INPUT,     "FX Return L");
        configInput(RETURN1_R_INPUT,     "FX Return R");

        configOutput(CHAIN_L_OUTPUT,     "Chain Out L");
        configOutput(CHAIN_R_OUTPUT,     "Chain Out R");
        configOutput(SEND1_CHAIN_OUTPUT, "FX Chain Out L");
        configOutput(SEND1R_CHAIN_OUTPUT,"FX Chain Out R");
    }

    // Zachte clip — transparant, geen harde vervorming
    float softClip(float x) {
        const float threshold = 5.f;
        if (x >  threshold) return  threshold + (x -  threshold) / (1.f + ((x - threshold) / threshold));
        if (x < -threshold) return -threshold + (x + threshold) / (1.f + ((-x - threshold) / threshold));
        return x;
    }

    void setPeakLight(float p) {
        if (p > 0.9f) {
            lights[PEAK_R_LIGHT].setBrightness(1.f);
            lights[PEAK_G_LIGHT].setBrightness(0.f);
            lights[PEAK_B_LIGHT].setBrightness(0.f);
        } else if (p > 0.6f) {
            lights[PEAK_R_LIGHT].setBrightness(1.f);
            lights[PEAK_G_LIGHT].setBrightness(0.8f);
            lights[PEAK_B_LIGHT].setBrightness(0.f);
        } else {
            lights[PEAK_R_LIGHT].setBrightness(0.f);
            lights[PEAK_G_LIGHT].setBrightness(p * 1.5f);
            lights[PEAK_B_LIGHT].setBrightness(0.f);
        }
    }

    void process(const ProcessArgs& args) override {
        // Input — mono normalled naar rechts
        float inL = inputs[IN_L_INPUT].getVoltage();
        float inR = inputs[IN_R_INPUT].isConnected() ? inputs[IN_R_INPUT].getVoltage() : inL;

        // Gain — 1x tot 10x (0dB tot +20dB)
        float gain = params[GAIN_PARAM].getValue();
        inL *= gain;
        inR *= gain;

        // Soft clip — beschermt tegen harde vervorming
        inL = softClip(inL);
        inR = softClip(inR);

        // Peak meten na gain, voor volume
        float rawPeak = std::max(std::abs(inL), std::abs(inR)) / 5.f;
        peak = std::max(rawPeak, peak * PEAK_DECAY);

        // Volume fader
        float vol = params[VOL_PARAM].getValue();

        // Comp/CV sidechain
        if (inputs[COMP_INPUT].isConnected()) {
            float compCV = clamp(inputs[COMP_INPUT].getVoltage(), 0.f, 10.f);
            vol *= (1.f - compCV / 10.f);
        }

        // Mute — knop of CV
        if (params[MUTE_PARAM].getValue() > 0.5f) vol = 0.f;
        if (inputs[MUTE_CV_INPUT].isConnected() && inputs[MUTE_CV_INPUT].getVoltage() > 1.f) vol = 0.f;

        inL *= vol;
        inR *= vol;

        // FX send
        float sendL = inL * params[SEND1_PARAM].getValue();
        float sendR = inR * params[SEND1_PARAM].getValue();

        // FX return schalen op basis van send niveau
        float sendAmt = params[MUTE_PARAM].getValue() > 0.5f ? 0.f : params[SEND1_PARAM].getValue();
        float ret1L = inputs[RETURN1_L_INPUT].getVoltage() * sendAmt;
        float ret1R = (inputs[RETURN1_R_INPUT].isConnected() ?
                       inputs[RETURN1_R_INPUT].getVoltage() : ret1L) * sendAmt;

        // Chain in
        float chainInL = inputs[CHAIN_L_INPUT].getVoltage();
        float chainInR = inputs[CHAIN_R_INPUT].getVoltage();

        // Chain out
        outputs[CHAIN_L_OUTPUT].setVoltage(inL + chainInL + ret1L);
        outputs[CHAIN_R_OUTPUT].setVoltage(inR + chainInR + ret1R);

        // FX chain out
        outputs[SEND1_CHAIN_OUTPUT].setVoltage(sendL + inputs[SEND1_CHAIN_INPUT].getVoltage());
        outputs[SEND1R_CHAIN_OUTPUT].setVoltage(sendR + inputs[SEND1R_CHAIN_INPUT].getVoltage());

        // Peak LED
        setPeakLight(peak);

        // VU meter
        float sig = std::max(std::abs(inL), std::abs(inR)) / 5.f;
        if (sig > 1.f) sig = 1.f;
        vuLevel = sig > vuLevel ? sig : vuLevel * 0.9990f;
        if (vuLevel > 1.f) vuLevel = 1.f;
    }

    json_t* dataToJson() override { return json_object(); }
    void dataFromJson(json_t* rootJ) override { (void)rootJ; }
};

// ── SLIDER ────────────────────────────────────────────────────────────────────

struct GainSlider : SvgSlider {
    GainSlider() {
        setHandleSvg(Svg::load(asset::plugin(pluginInstance, "res/ChainSliderHandle.svg")));
        setHandlePosCentered(Vec(0.f, mm2px(45.2f)), Vec(0.f, 0.f));
        box.size = mm2px(Vec(9.66f, 49.13f));
        horizontal = false;
    }
};

// ── MUTE BUTTON ───────────────────────────────────────────────────────────────

struct GainMuteButton : SvgSwitch {
    GainMuteButton() {
        momentary = false;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChainMuteButton_1.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChainMuteButton_0.svg")));
    }
};

// ── VU METER ──────────────────────────────────────────────────────────────────

struct GainVUMeter : Widget {
    GainModule* module = nullptr;

    void draw(const DrawArgs& args) override {
        if (module == nullptr) return;
        float h = box.size.y;
        if (h < 1.f) return;
        float level = module->vuLevel;
        if (level <= 0.f) return;
        if (level > 1.f) level = 1.f;
        float vuH = h * level;
        float lw = 1.5f;
        float lx = 0.f;
        if (level > 0.85f) {
            float clipH = h * 0.85f;
            float redH = vuH - clipH;
            nvgBeginPath(args.vg);
            nvgRect(args.vg, lx, h - clipH, lw, clipH);
            nvgFillColor(args.vg, nvgRGBA(255, 255, 0, 255));
            nvgFill(args.vg);
            if (redH > 0.f) {
                nvgBeginPath(args.vg);
                nvgRect(args.vg, lx, h - vuH, lw, redH);
                nvgFillColor(args.vg, nvgRGBA(255, 0, 0, 255));
                nvgFill(args.vg);
            }
        } else {
            nvgBeginPath(args.vg);
            nvgRect(args.vg, lx, h - vuH, lw, vuH);
            nvgFillColor(args.vg, nvgRGBA(255, 255, 0, 255));
            nvgFill(args.vg);
        }
    }
};

// ── WIDGET ────────────────────────────────────────────────────────────────────

struct GainWidget : ModuleWidget {
    GainWidget(GainModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Gain.svg")));

        // GAIN knop groot
        addParam(createParamCentered<GainKnobLarge>(mm2px(Vec(44.90f, 39.24f)), module, GainModule::GAIN_PARAM));

        // FX1 knop klein
        addParam(createParamCentered<GainKnobSmall>(mm2px(Vec(11.31f, 31.61f)), module, GainModule::SEND1_PARAM));

        // Fader
        addParam(createParam<GainSlider>(mm2px(Vec(25.76f, 26.61f)), module, GainModule::VOL_PARAM));

        // Mute knop + CV
        addParam(createParamCentered<GainMuteButton>(mm2px(Vec(37.48f, 78.59f)), module, GainModule::MUTE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(37.48f, 69.67f)), module, GainModule::MUTE_CV_INPUT));

        // Line Level inputs
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.36f, 56.86f)), module, GainModule::IN_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.36f, 69.67f)), module, GainModule::IN_R_INPUT));

        // Comp/CV
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.52f, 85.10f)), module, GainModule::COMP_INPUT));

        // Chain in
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.04f, 102.73f)), module, GainModule::CHAIN_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.04f, 116.19f)), module, GainModule::CHAIN_R_INPUT));

        // Chain out
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(52.90f, 102.73f)), module, GainModule::CHAIN_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(52.90f, 116.19f)), module, GainModule::CHAIN_R_OUTPUT));

        // FX send/return
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.68f, 102.73f)), module, GainModule::SEND1_CHAIN_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(24.68f, 116.19f)), module, GainModule::SEND1R_CHAIN_OUTPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35.21f, 102.73f)), module, GainModule::RETURN1_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35.21f, 116.19f)), module, GainModule::RETURN1_R_INPUT));

        // Peak LED
        addChild(createLightCentered<SmallLight<GreenRedLight>>(mm2px(Vec(30.29f, 20.92f)), module, GainModule::PEAK_R_LIGHT));

        // VU meter
        {
            auto vu = createWidget<GainVUMeter>(mm2px(Vec(23.70f, 20.00f)));
            vu->box.size = mm2px(Vec(2.0f, 48.32f));
            vu->module = module;
            addChild(vu);
        }
    }

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Manual", "", []() {
            system::openBrowser("https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/gain/");
        }));
        menu->addChild(createMenuItem("submitaudio.nl", "", []() {
            system::openBrowser(SUBMIT_URL);
        }));
        menu->addChild(createMenuItem("Report a Bug", "", []() {
            system::openBrowser("https://github.com/submitaudio/submit-vcv-modules/issues");
        }));
    }
};

Model* modelGain = createModel<GainModule, GainWidget>("Gain");
