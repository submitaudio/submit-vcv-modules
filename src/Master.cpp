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

struct Master : Module {
    enum ParamIds {
        OUTPUT_PARAM,
        TRANSIENT_PARAM,
        GLUE_PARAM,
        WIDTH_PARAM,
        LIMIT_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        AUDIO_IN_L_INPUT,
        AUDIO_IN_R_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUT_L_OUTPUT,
        AUDIO_OUT_R_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        CLIP_LIGHT,
        NUM_LIGHTS
    };

    float vuLevel = 0.f;
    bool clipping = false;
    int clipTimer = 0;

    double rms = 0.0;
    double envGain = 1.0;
    double envFast = 0.0, envSlow = 0.0;

    float sampleRate = 44100.f;

    Master() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(OUTPUT_PARAM,    0.f, 1.5f, 1.f,  "Output",    "x", 0, 1);
        configParam(TRANSIENT_PARAM,-1.f, 1.f,  0.f,  "Transient", "%", 0, 100);
        configParam(GLUE_PARAM,     0.f,  1.f,  0.f,  "Glue",      "%", 0, 100);
        configParam(WIDTH_PARAM,    0.f,  2.f,  1.f,  "Width",     "x", 0, 1);
        configParam(LIMIT_PARAM,    0.f,  1.f,  0.f,  "Limit",     "%", 0, 100);
        configInput(AUDIO_IN_L_INPUT,  "Audio In L");
        configInput(AUDIO_IN_R_INPUT,  "Audio In R");
        configOutput(AUDIO_OUT_L_OUTPUT, "Audio Out L");
        configOutput(AUDIO_OUT_R_OUTPUT, "Audio Out R");
    }

    void onSampleRateChange() override {
        sampleRate = APP->engine->getSampleRate();
    }

    void process(const ProcessArgs& args) override {
        double inL = inputs[AUDIO_IN_L_INPUT].getVoltage();
        double inR = inputs[AUDIO_IN_R_INPUT].isConnected() ?
                     inputs[AUDIO_IN_R_INPUT].getVoltage() : inL;

        float glueAmt    = params[GLUE_PARAM].getValue();
        float transAmt   = params[TRANSIENT_PARAM].getValue();
        float widthAmt   = params[WIDTH_PARAM].getValue();
        float limitAmt   = params[LIMIT_PARAM].getValue();
        float outputGain = params[OUTPUT_PARAM].getValue();

        // 1. WARMTE — zachte even-harmonic saturatie (alleen actief boven drempel)
        double warmDrive = 0.15;
        double warmThresh = 0.01;
        if (fabs(inL) > warmThresh)
            inL = inL + warmDrive * inL * inL * (inL > 0 ? 1.0 : -1.0) * 0.1;
        if (fabs(inR) > warmThresh)
            inR = inR + warmDrive * inR * inR * (inR > 0 ? 1.0 : -1.0) * 0.1;

        // 2. TRANSIENT SHAPER
        if (fabs(transAmt) > 0.01f) {
            double inAbs = (fabs(inL) + fabs(inR)) * 0.5;
            double fast = exp(-1.0 / (0.001 * sampleRate));
            double slow = exp(-1.0 / (0.010 * sampleRate));
            envFast = envFast * fast + inAbs * (1.0 - fast);
            envSlow = envSlow * slow + inAbs * (1.0 - slow);
            double transient = envFast - envSlow;
            double gain = 1.0 + transAmt * transient * 2.0;
            if (gain < 0.5) gain = 0.5;
            if (gain > 2.0) gain = 2.0;
            // Smooth de gain om kraken te voorkomen
            static double smoothGain = 1.0;
            smoothGain = smoothGain * 0.99 + gain * 0.01;
            inL *= smoothGain;
            inR *= smoothGain;
        }

        // 3. GLUE COMPRESSOR
        if (glueAmt > 0.01f) {
            double threshold = 1.0 - glueAmt * 0.6;
            double ratio = 1.0 + glueAmt * 6.0;
            double attack  = exp(-1.0 / (0.010 * sampleRate));
            double release = exp(-1.0 / (0.200 * sampleRate));

            double inAbs = (fabs(inL) + fabs(inR)) * 0.5;
            rms = rms * 0.999 + inAbs * inAbs * 0.001;
            double rmsVal = sqrt(rms);

            double targetGain = 1.0;
            if (rmsVal > threshold) {
                targetGain = threshold + (rmsVal - threshold) / ratio;
                targetGain /= rmsVal;
            }
            double env = targetGain < envGain ? attack : release;
            envGain = envGain * env + targetGain * (1.0 - env);
            inL *= envGain;
            inR *= envGain;
        }

        // 4. WIDTH
        if (fabs(widthAmt - 1.0f) > 0.01f) {
            double mid  = (inL + inR) * 0.5;
            double side = (inL - inR) * 0.5;
            side *= widthAmt;
            inL = mid + side;
            inR = mid - side;
        }

        // 5. LIMITER
        if (limitAmt > 0.01f) {
            double ceiling = 5.0 * (1.0 - limitAmt * 0.25);
            double knee = 1.5;
            auto softLimit = [&](double x) -> double {
                double ax = fabs(x);
                if (ax <= ceiling - knee) return x;
                if (ax >= ceiling + knee) {
                    return x > 0 ? ceiling : -ceiling;
                }
                double over = ax - (ceiling - knee);
                double soft = ceiling - knee + (2.0 * knee * over - over * over / (2.0 * knee)) / (2.0 * knee);
                return x > 0 ? soft : -soft;
            };
            // Limiter met lookahead envelope
            static double limGain = 1.0;
            double peakL = fabs(inL), peakR = fabs(inR);
            double peak = peakL > peakR ? peakL : peakR;
            double targetLimGain = peak > ceiling ? ceiling / peak : 1.0;
            double limAttack  = exp(-1.0 / (0.0001 * sampleRate));
            double limRelease = exp(-1.0 / (0.100 * sampleRate));
            double limEnv = targetLimGain < limGain ? limAttack : limRelease;
            limGain = limGain * limEnv + targetLimGain * (1.0 - limEnv);
            inL *= limGain;
            inR *= limGain;
        }

        // 6. OUTPUT GAIN
        inL *= outputGain;
        inR *= outputGain;

        // VU meter
        float sig = std::max(fabs((float)inL), fabs((float)inR)) / 5.f;
        if (sig > 1.f) sig = 1.f;
        vuLevel = sig > vuLevel ? sig : vuLevel * 0.9990f;
        if (vuLevel > 1.f) vuLevel = 1.f;

        // Clip detectie
        if (fabs(inL) > 5.f || fabs(inR) > 5.f) {
            clipping = true;
            clipTimer = (int)sampleRate / 4;
        }
        if (clipTimer > 0) clipTimer--;
        else clipping = false;
        lights[CLIP_LIGHT].setBrightness(clipping ? 1.f : 0.f);

        outputs[AUDIO_OUT_L_OUTPUT].setVoltage((float)inL);
        outputs[AUDIO_OUT_R_OUTPUT].setVoltage((float)inR);
    }
};

struct MasterVUMeter : Widget {
    Master* module = nullptr;

    void draw(const DrawArgs& args) override {
        float h = box.size.y;
        if (h < 1.f) return;
        float level = module == nullptr ? 0.f : module->vuLevel;
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

struct MasterWidget : SubmitModuleWidget {
    MasterWidget(Master* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Master.svg")));

        addParam(createParamCentered<Drift13KnobMedium>(mm2px(Vec(20.68f, 34.75f)), module, Master::OUTPUT_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(11.64f, 58.65f)), module, Master::TRANSIENT_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(29.10f, 58.65f)), module, Master::GLUE_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(11.64f, 77.74f)), module, Master::WIDTH_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(29.14f, 77.74f)), module, Master::LIMIT_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.75f, 102.69f)), module, Master::AUDIO_IN_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.75f, 116.21f)), module, Master::AUDIO_IN_R_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.37f, 102.69f)), module, Master::AUDIO_OUT_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.37f, 116.21f)), module, Master::AUDIO_OUT_R_OUTPUT));

        addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(27.08f, 20.91f)), module, Master::CLIP_LIGHT));

        // VU meter als laatste!
        {
            auto vu = createWidget<MasterVUMeter>(mm2px(Vec(41.30f, 26.75f)));
            vu->box.size = mm2px(Vec(1.5f, 47.80f));
            vu->module = module;
            addChild(vu);
        }
    }

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Manual", "", []() {
            system::openBrowser("https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/master/");
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

Model* modelMaster = createModel<Master, MasterWidget>("Master");
