// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// Licensed under GPL v3 — see LICENSE file for details
// https://github.com/submitaudio/submit-vcv-modules

#include "plugin.hpp"
struct ChainKnob : SvgKnob {
    ChainKnob() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/ImpactKnobMini.svg")));
        shadow->opacity = 0.f;
    }
};



struct Mix2ch : Module {
    enum ParamId {
        // Kanaal 1
        CH1_GAIN_PARAM,
        CH1_VOL_PARAM,
        CH1_PAN_PARAM,
        CH1_MUTE_PARAM,
        CH1_HPF_PARAM,
        CH1_SEND1_PARAM,
        CH1_SEND2_PARAM,
        // Kanaal 2
        CH2_GAIN_PARAM,
        CH2_VOL_PARAM,
        CH2_PAN_PARAM,
        CH2_MUTE_PARAM,
        CH2_HPF_PARAM,
        CH2_SEND1_PARAM,
        CH2_SEND2_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        CH1_L_INPUT,
        CH1_R_INPUT,
        CH2_L_INPUT,
        CH2_R_INPUT,
        // Audio chain in
        CH1_COMP_INPUT,
        CH2_COMP_INPUT,
        CH1_MUTE_CV_INPUT,
        CH2_MUTE_CV_INPUT,
        CHAIN_L_INPUT,
        CHAIN_R_INPUT,
        // Send chain in (van vorige mixer)
        SEND1_CHAIN_INPUT,
        SEND1R_CHAIN_INPUT,
        SEND2_CHAIN_INPUT,
        SEND2R_CHAIN_INPUT,
        // Returns
        RETURN1_L_INPUT,
        RETURN1_R_INPUT,
        RETURN2_L_INPUT,
        RETURN2_R_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        // Audio chain out
        CHAIN_L_OUTPUT,
        CHAIN_R_OUTPUT,
        // Send chain out (naar volgende mixer of effect)
        SEND1_CHAIN_OUTPUT,
        SEND1R_CHAIN_OUTPUT,
        SEND2_CHAIN_OUTPUT,
        SEND2R_CHAIN_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        CH1_PEAK_R_LIGHT,
        CH1_PEAK_G_LIGHT,
        CH1_PEAK_B_LIGHT,
        CH2_PEAK_R_LIGHT,
        CH2_PEAK_G_LIGHT,
        CH2_PEAK_B_LIGHT,
        CH1_CLIP_LIGHT,
        CH2_CLIP_LIGHT,
        LIGHTS_LEN
    };

    float hpf1L = 0.f, hpf1R = 0.f;
    float hpf2L = 0.f, hpf2R = 0.f;
    float peak1 = 0.f, peak2 = 0.f;
    float vuLevel1 = 0.f, vuLevel2 = 0.f;
    float peakLevel1 = 0.f, peakLevel2 = 0.f;
    static constexpr float PEAK_DECAY = 0.9995f;

    Mix2ch() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(CH1_GAIN_PARAM, 0.f, 2.f, 1.f, "Ch1 Pre-gain");
        configParam(CH1_VOL_PARAM,  0.f, 1.f, 0.75f, "Ch1 Volume");
        configParam(CH1_PAN_PARAM, -1.f, 1.f, 0.f, "Ch1 Pan");
        configSwitch(CH1_MUTE_PARAM, 0.f, 1.f, 0.f, "Ch1 Mute", {"On", "Mute"});
        configSwitch(CH1_HPF_PARAM,  0.f, 1.f, 0.f, "Ch1 40Hz HPF", {"Off", "On"});
        configParam(CH1_SEND1_PARAM, 0.f, 1.f, 0.f, "Ch1 Send 1");
        configParam(CH1_SEND2_PARAM, 0.f, 1.f, 0.f, "Ch1 Send 2");

        configParam(CH2_GAIN_PARAM, 0.f, 2.f, 1.f, "Ch2 Pre-gain");
        configParam(CH2_VOL_PARAM,  0.f, 1.f, 0.75f, "Ch2 Volume");
        configParam(CH2_PAN_PARAM, -1.f, 1.f, 0.f, "Ch2 Pan");
        configSwitch(CH2_MUTE_PARAM, 0.f, 1.f, 0.f, "Ch2 Mute", {"On", "Mute"});
        configSwitch(CH2_HPF_PARAM,  0.f, 1.f, 0.f, "Ch2 40Hz HPF", {"Off", "On"});
        configParam(CH2_SEND1_PARAM, 0.f, 1.f, 0.f, "Ch2 Send 1");
        configParam(CH2_SEND2_PARAM, 0.f, 1.f, 0.f, "Ch2 Send 2");

        configInput(CH1_L_INPUT, "Ch1 L");
        configInput(CH1_R_INPUT, "Ch1 R");
        configInput(CH2_L_INPUT, "Ch2 L");
        configInput(CH2_R_INPUT, "Ch2 R");
        configInput(CH1_COMP_INPUT, "Ch1 Comp/CV");
        configInput(CH2_COMP_INPUT, "Ch2 Comp/CV");
        configInput(CH1_MUTE_CV_INPUT, "Ch1 Mute CV");
        configInput(CH2_MUTE_CV_INPUT, "Ch2 Mute CV");
        configInput(CHAIN_L_INPUT, "Audio In L");
        configInput(CHAIN_R_INPUT, "Audio In R");
        configInput(SEND1_CHAIN_INPUT,  "Send 1 L Chain In");
        configInput(SEND1R_CHAIN_INPUT, "Send 1 R Chain In");
        configInput(SEND2_CHAIN_INPUT,  "Send 2 L Chain In");
        configInput(SEND2R_CHAIN_INPUT, "Send 2 R Chain In");
        configInput(RETURN1_L_INPUT, "Return 1 L");
        configInput(RETURN1_R_INPUT, "Return 1 R");
        configInput(RETURN2_L_INPUT, "Return 2 L");
        configInput(RETURN2_R_INPUT, "Return 2 R");

        configOutput(CHAIN_L_OUTPUT, "Audio Out L");
        configOutput(CHAIN_R_OUTPUT, "Audio Out R");
        configOutput(SEND1_CHAIN_OUTPUT,  "Send 1 L Chain Out");
        configOutput(SEND1R_CHAIN_OUTPUT, "Send 1 R Chain Out");
        configOutput(SEND2_CHAIN_OUTPUT,  "Send 2 L Chain Out");
        configOutput(SEND2R_CHAIN_OUTPUT, "Send 2 R Chain Out");
    }

    float applyHPF(float x, float& state, float alpha) {
        state += alpha * (x - state);
        return x - state;
    }

    void applyPan(float pan, float& gainL, float& gainR) {
        float p = (pan + 1.f) * 0.5f;
        gainL = std::cos(p * float(M_PI) * 0.5f);
        gainR = std::sin(p * float(M_PI) * 0.5f);
    }

    void setPeakLight(int ch, float peak) {
        int r = (ch == 1) ? CH1_PEAK_R_LIGHT : CH2_PEAK_R_LIGHT;
        int g = (ch == 1) ? CH1_PEAK_G_LIGHT : CH2_PEAK_G_LIGHT;
        int b = (ch == 1) ? CH1_PEAK_B_LIGHT : CH2_PEAK_B_LIGHT;
        if (peak > 0.9f) {
            lights[r].setBrightness(1.f);
            lights[g].setBrightness(0.f);
            lights[b].setBrightness(0.f);
        } else if (peak > 0.6f) {
            lights[r].setBrightness(1.f);
            lights[g].setBrightness(0.8f);
            lights[b].setBrightness(0.f);
        } else {
            lights[r].setBrightness(0.f);
            lights[g].setBrightness(peak * 1.5f);
            lights[b].setBrightness(0.f);
        }
    }

    void process(const ProcessArgs& args) override {
        float hpfAlpha = 1.f - std::exp(-2.f * float(M_PI) * 40.f / args.sampleRate);

        // ── KANAAL 1 ──────────────────────────────
        float ch1L = inputs[CH1_L_INPUT].getVoltage();
        float ch1R = inputs[CH1_R_INPUT].isConnected() ? inputs[CH1_R_INPUT].getVoltage() : ch1L;

        float gain1 = params[CH1_GAIN_PARAM].getValue();
        ch1L *= gain1; ch1R *= gain1;

        if (params[CH1_HPF_PARAM].getValue() > 0.5f) {
            ch1L = applyHPF(ch1L, hpf1L, hpfAlpha);
            ch1R = applyHPF(ch1R, hpf1R, hpfAlpha);
        }

        float rawPeak1 = std::max(std::abs(ch1L), std::abs(ch1R)) / 5.f;
        peak1 = std::max(rawPeak1, peak1 * PEAK_DECAY);

        float vol1 = params[CH1_VOL_PARAM].getValue();
        if (inputs[CH1_COMP_INPUT].isConnected()) {
            float compCV1 = clamp(inputs[CH1_COMP_INPUT].getVoltage(), 0.f, 10.f);
            vol1 *= (1.f - compCV1 / 10.f);
        }
        if (params[CH1_MUTE_PARAM].getValue() > 0.5f) vol1 = 0.f;
        if (inputs[CH1_MUTE_CV_INPUT].isConnected() && inputs[CH1_MUTE_CV_INPUT].getVoltage() > 1.f) vol1 = 0.f;
        if (inputs[CH1_MUTE_CV_INPUT].isConnected() && inputs[CH1_MUTE_CV_INPUT].getVoltage() > 1.f) vol1 = 0.f;
        ch1L *= vol1; ch1R *= vol1;

        float panL1, panR1;
        applyPan(params[CH1_PAN_PARAM].getValue(), panL1, panR1);

        float s1_1L = ch1L * params[CH1_SEND1_PARAM].getValue();
        float s1_1R = ch1R * params[CH1_SEND1_PARAM].getValue();
        float s2_1L = ch1L * params[CH1_SEND2_PARAM].getValue();
        float s2_1R = ch1R * params[CH1_SEND2_PARAM].getValue();

        float mix1L = ch1L * panL1;
        float mix1R = ch1R * panR1;

        // ── KANAAL 2 ──────────────────────────────
        float ch2L = inputs[CH2_L_INPUT].getVoltage();
        float ch2R = inputs[CH2_R_INPUT].isConnected() ? inputs[CH2_R_INPUT].getVoltage() : ch2L;

        float gain2 = params[CH2_GAIN_PARAM].getValue();
        ch2L *= gain2; ch2R *= gain2;

        if (params[CH2_HPF_PARAM].getValue() > 0.5f) {
            ch2L = applyHPF(ch2L, hpf2L, hpfAlpha);
            ch2R = applyHPF(ch2R, hpf2R, hpfAlpha);
        }

        float rawPeak2 = std::max(std::abs(ch2L), std::abs(ch2R)) / 5.f;
        peak2 = std::max(rawPeak2, peak2 * PEAK_DECAY);

        float vol2 = params[CH2_VOL_PARAM].getValue();
        if (inputs[CH2_COMP_INPUT].isConnected()) {
            float compCV2 = clamp(inputs[CH2_COMP_INPUT].getVoltage(), 0.f, 10.f);
            vol2 *= (1.f - compCV2 / 10.f);
        }
        if (params[CH2_MUTE_PARAM].getValue() > 0.5f) vol2 = 0.f;
        if (inputs[CH2_MUTE_CV_INPUT].isConnected() && inputs[CH2_MUTE_CV_INPUT].getVoltage() > 1.f) vol2 = 0.f;
        if (inputs[CH2_MUTE_CV_INPUT].isConnected() && inputs[CH2_MUTE_CV_INPUT].getVoltage() > 1.f) vol2 = 0.f;
        ch2L *= vol2; ch2R *= vol2;

        float panL2, panR2;
        applyPan(params[CH2_PAN_PARAM].getValue(), panL2, panR2);

        float s1_2L = ch2L * params[CH2_SEND1_PARAM].getValue();
        float s1_2R = ch2R * params[CH2_SEND1_PARAM].getValue();
        float s2_2L = ch2L * params[CH2_SEND2_PARAM].getValue();
        float s2_2R = ch2R * params[CH2_SEND2_PARAM].getValue();

        float mix2L = ch2L * panL2;
        float mix2R = ch2R * panR2;

        // ── AUDIO MIX + CHAIN ─────────────────────
        float chainInL = inputs[CHAIN_L_INPUT].getVoltage();
        float chainInR = inputs[CHAIN_R_INPUT].getVoltage();

        float ret1L = inputs[RETURN1_L_INPUT].getVoltage();
        float ret1R = inputs[RETURN1_R_INPUT].isConnected() ? inputs[RETURN1_R_INPUT].getVoltage() : ret1L;
        float ret2L = inputs[RETURN2_L_INPUT].getVoltage();
        float ret2R = inputs[RETURN2_R_INPUT].isConnected() ? inputs[RETURN2_R_INPUT].getVoltage() : ret2L;

        bool mute1 = params[CH1_MUTE_PARAM].getValue() > 0.5f;
        bool mute2 = params[CH2_MUTE_PARAM].getValue() > 0.5f;
        // FX return schalen op basis van send niveau per kanaal
        float ch1send1 = mute1 ? 0.f : params[CH1_SEND1_PARAM].getValue();
        float ch2send1 = mute2 ? 0.f : params[CH2_SEND1_PARAM].getValue();
        float fx1scale = std::max(ch1send1, ch2send1);
        ret1L *= fx1scale; ret1R *= fx1scale;

        float ch1send2 = mute1 ? 0.f : params[CH1_SEND2_PARAM].getValue();
        float ch2send2 = mute2 ? 0.f : params[CH2_SEND2_PARAM].getValue();
        float fx2scale = std::max(ch1send2, ch2send2);
        ret2L *= fx2scale; ret2R *= fx2scale;

        outputs[CHAIN_L_OUTPUT].setVoltage(mix1L + mix2L + chainInL + ret1L + ret2L);
        outputs[CHAIN_R_OUTPUT].setVoltage(mix1R + mix2R + chainInR + ret1R + ret2R);

        // ── SEND CHAIN (eigen send + wat binnenkomt) ──
        outputs[SEND1_CHAIN_OUTPUT].setVoltage(s1_1L + s1_2L + inputs[SEND1_CHAIN_INPUT].getVoltage());
        outputs[SEND1R_CHAIN_OUTPUT].setVoltage(s1_1R + s1_2R + inputs[SEND1R_CHAIN_INPUT].getVoltage());
        outputs[SEND2_CHAIN_OUTPUT].setVoltage(s2_1L + s2_2L + inputs[SEND2_CHAIN_INPUT].getVoltage());
        outputs[SEND2R_CHAIN_OUTPUT].setVoltage(s2_1R + s2_2R + inputs[SEND2R_CHAIN_INPUT].getVoltage());

        // Peak LEDs
        setPeakLight(1, peak1);
        setPeakLight(2, peak2);

        // VU meter
        float sig1 = std::max(std::abs(mix1L), std::abs(mix1R)) / 5.f;
        if (sig1 > 1.f) sig1 = 1.f;
        if (sig1 < 0.f) sig1 = 0.f;
        vuLevel1 = sig1 > vuLevel1 ? sig1 : vuLevel1 * 0.9990f;
        if (vuLevel1 > 1.f) vuLevel1 = 1.f;

        float sig2 = std::max(std::abs(mix2L), std::abs(mix2R)) / 5.f;
        if (sig2 > 1.f) sig2 = 1.f;
        if (sig2 < 0.f) sig2 = 0.f;
        vuLevel2 = sig2 > vuLevel2 ? sig2 : vuLevel2 * 0.9990f;
        if (vuLevel2 > 1.f) vuLevel2 = 1.f;
    }

    json_t* dataToJson() override { return json_object(); }
    void dataFromJson(json_t* rootJ) override { (void)rootJ; }
};

struct ChainSlider : SvgSlider {
    ChainSlider() {
        setHandleSvg(Svg::load(asset::plugin(pluginInstance, "res/ChainSliderHandle.svg")));
        setHandlePosCentered(Vec(0.f, mm2px(45.2f)), Vec(0.f, 0.f));
        box.size = mm2px(Vec(9.66f, 49.13f));
        horizontal = false;
    }
};

struct ChainMuteButton : SvgSwitch {
    ChainMuteButton() {
        momentary = false;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChainMuteButton_1.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChainMuteButton_0.svg")));
    }
};

struct ChainVUMeter : Widget {
    Mix2ch* module = nullptr;
    int channel = 0;

    void draw(const DrawArgs& args) override {
        if (module == nullptr) return;
        float h = box.size.y;
        if (h < 1.f) return;
        float level = (channel == 0) ? module->vuLevel1 : module->vuLevel2;
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

struct Mix2chWidget : ModuleWidget {
    Mix2chWidget(Mix2ch* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Chain.svg")));

        // ── KANAAL 1-2 ────────────────────────────
        addParam(createParamCentered<ChainKnob>(mm2px(Vec(11.28f, 31.61f)), module, Mix2ch::CH1_GAIN_PARAM));
        addParam(createParamCentered<ChainKnob>(mm2px(Vec(11.28f, 43.93f)), module, Mix2ch::CH1_PAN_PARAM));
        addParam(createParamCentered<ChainKnob>(mm2px(Vec(37.43f, 31.61f)), module, Mix2ch::CH1_SEND1_PARAM));
        addParam(createParamCentered<ChainKnob>(mm2px(Vec(37.43f, 43.93f)), module, Mix2ch::CH1_SEND2_PARAM));
        addParam(createParam<ChainSlider>(mm2px(Vec(24.5f, 26.61f)), module, Mix2ch::CH1_VOL_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(37.44f, 55.92f)), module, Mix2ch::CH1_HPF_PARAM));
        addParam(createParamCentered<ChainMuteButton>(mm2px(Vec(37.48f, 78.59f)), module, Mix2ch::CH1_MUTE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(37.34f, 69.67f)), module, Mix2ch::CH1_MUTE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(84.21f, 69.67f)), module, Mix2ch::CH2_MUTE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.46f, 56.86f)), module, Mix2ch::CH1_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.46f, 69.67f)), module, Mix2ch::CH1_R_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.58f, 85.10f)), module, Mix2ch::CH1_COMP_INPUT));

        // ── KANAAL 3-4 ────────────────────────────
        addParam(createParamCentered<ChainKnob>(mm2px(Vec(58.16f, 31.61f)), module, Mix2ch::CH2_GAIN_PARAM));
        addParam(createParamCentered<ChainKnob>(mm2px(Vec(58.16f, 43.93f)), module, Mix2ch::CH2_PAN_PARAM));
        addParam(createParamCentered<ChainKnob>(mm2px(Vec(84.31f, 31.61f)), module, Mix2ch::CH2_SEND1_PARAM));
        addParam(createParamCentered<ChainKnob>(mm2px(Vec(84.31f, 43.93f)), module, Mix2ch::CH2_SEND2_PARAM));
        addParam(createParam<ChainSlider>(mm2px(Vec(71.21f, 26.61f)), module, Mix2ch::CH2_VOL_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(84.14f, 55.92f)), module, Mix2ch::CH2_HPF_PARAM));
        addParam(createParamCentered<ChainMuteButton>(mm2px(Vec(84.23f, 78.59f)), module, Mix2ch::CH2_MUTE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(58.17f, 56.86f)), module, Mix2ch::CH2_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(58.17f, 69.67f)), module, Mix2ch::CH2_R_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(71.39f, 85.10f)), module, Mix2ch::CH2_COMP_INPUT));

        // ── CHAIN ─────────────────────────────────
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.03f, 102.73f)), module, Mix2ch::CHAIN_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.03f, 116.19f)), module, Mix2ch::CHAIN_R_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(84.94f, 102.73f)), module, Mix2ch::CHAIN_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(84.94f, 116.19f)), module, Mix2ch::CHAIN_R_OUTPUT));

        // ── FX1 ───────────────────────────────────
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(28.25f, 102.73f)), module, Mix2ch::SEND1_CHAIN_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(28.25f, 116.19f)), module, Mix2ch::SEND1R_CHAIN_OUTPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.79f, 102.73f)), module, Mix2ch::RETURN1_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.79f, 116.19f)), module, Mix2ch::RETURN1_R_INPUT));

        // ── FX2 ───────────────────────────────────
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(56.46f, 102.73f)), module, Mix2ch::SEND2_CHAIN_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(56.46f, 116.19f)), module, Mix2ch::SEND2R_CHAIN_OUTPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(67.13f, 102.73f)), module, Mix2ch::RETURN2_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(67.13f, 116.19f)), module, Mix2ch::RETURN2_R_INPUT));

        // ── VU METER CH1-2 (als laatste zodat hij bovenop ligt) ──
        {
            auto vu = createWidget<ChainVUMeter>(mm2px(Vec(23.70f, 26.51f)));
            vu->box.size = mm2px(Vec(2.0f, 48.32f));
            vu->module = module;
            vu->channel = 0;
            addChild(vu);
        }

        // ── VU METER CH3-4 (als laatste zodat hij bovenop ligt) ──
        {
            auto vu = createWidget<ChainVUMeter>(mm2px(Vec(70.63f, 26.51f)));
            vu->box.size = mm2px(Vec(2.0f, 48.32f));
            vu->module = module;
            vu->channel = 1;
            addChild(vu);
        }

        // Clip LEDs
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(30.28f, 20.91f)), module, Mix2ch::CH1_CLIP_LIGHT));
        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(77.70f, 20.91f)), module, Mix2ch::CH2_CLIP_LIGHT));
    }

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Manual", "", []() {
            system::openBrowser("https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/chain/");
        }));
        menu->addChild(createMenuItem("submitaudio.nl", "", []() {
            system::openBrowser(SUBMIT_URL);
        }));
        menu->addChild(createMenuItem("Report a Bug", "", []() {
            system::openBrowser("https://github.com/submitaudio/submit-vcv-modules/issues");
        }));
    }
};


Model* modelChain = createModel<Mix2ch, Mix2chWidget>("Chain");
