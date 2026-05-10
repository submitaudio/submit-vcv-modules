// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// Licensed under GPL v3 — see LICENSE file for details
// https://github.com/submitaudio/submit-vcv-modules

#include "plugin.hpp"
#include <cmath>

struct Drift13KnobSmall : SvgKnob {
    Drift13KnobSmall() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/Drift13KnobSmall.svg")));
        shadow->opacity = 0.f;
    }
};

struct Shape : Module {
    enum ParamIds {
        HIGH_PASS_PARAM,
        LOW_SHELF_PARAM,
        LOW_MID_PARAM,
        HIGH_MID_PARAM,
        HIGH_SHELF_PARAM,
        LOW_PASS_PARAM,
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
        NUM_LIGHTS
    };

    struct Biquad {
        double b0=1,b1=0,b2=0,a1=0,a2=0;
        double z1l=0,z2l=0,z1r=0,z2r=0;

        void process(double& l, double& r) {
            double outL = b0*l + z1l;
            z1l = b1*l - a1*outL + z2l;
            z2l = b2*l - a2*outL;
            l = outL;
            double outR = b0*r + z1r;
            z1r = b1*r - a1*outR + z2r;
            z2r = b2*r - a2*outR;
            r = outR;
        }

        void setHighPass(double freq, double sr) {
            double w0 = 2*M_PI*freq/sr;
            double cosw = cos(w0), sinw = sin(w0);
            double alpha = sinw / (2.0 * 0.707);
            double a0 = 1 + alpha;
            b0 = (1+cosw)/2/a0; b1 = -(1+cosw)/a0; b2 = b0;
            a1 = -2*cosw/a0; a2 = (1-alpha)/a0;
        }

        void setLowPass(double freq, double sr) {
            double w0 = 2*M_PI*freq/sr;
            double cosw = cos(w0), sinw = sin(w0);
            double alpha = sinw / (2.0 * 0.707);
            double a0 = 1 + alpha;
            b0 = (1-cosw)/2/a0; b1 = (1-cosw)/a0; b2 = b0;
            a1 = -2*cosw/a0; a2 = (1-alpha)/a0;
        }

        void setLowShelf(double freq, double gain_dB, double sr) {
            double A = pow(10, gain_dB/40.0);
            double w0 = 2*M_PI*freq/sr;
            double cosw = cos(w0), sinw = sin(w0);
            double alpha = sinw/2 * sqrt((A+1/A)*(1/0.707-1)+2);
            double a0 = (A+1)+(A-1)*cosw+2*sqrt(A)*alpha;
            b0 = A*((A+1)-(A-1)*cosw+2*sqrt(A)*alpha)/a0;
            b1 = 2*A*((A-1)-(A+1)*cosw)/a0;
            b2 = A*((A+1)-(A-1)*cosw-2*sqrt(A)*alpha)/a0;
            a1 = -2*((A-1)+(A+1)*cosw)/a0;
            a2 = ((A+1)+(A-1)*cosw-2*sqrt(A)*alpha)/a0;
        }

        void setHighShelf(double freq, double gain_dB, double sr) {
            double A = pow(10, gain_dB/40.0);
            double w0 = 2*M_PI*freq/sr;
            double cosw = cos(w0), sinw = sin(w0);
            double alpha = sinw/2 * sqrt((A+1/A)*(1/0.707-1)+2);
            double a0 = (A+1)-(A-1)*cosw+2*sqrt(A)*alpha;
            b0 = A*((A+1)+(A-1)*cosw+2*sqrt(A)*alpha)/a0;
            b1 = -2*A*((A-1)+(A+1)*cosw)/a0;
            b2 = A*((A+1)+(A-1)*cosw-2*sqrt(A)*alpha)/a0;
            a1 = 2*((A-1)-(A+1)*cosw)/a0;
            a2 = ((A+1)-(A-1)*cosw-2*sqrt(A)*alpha)/a0;
        }

        void setPeak(double freq, double gain_dB, double Q, double sr) {
            double A = pow(10, gain_dB/40.0);
            double w0 = 2*M_PI*freq/sr;
            double cosw = cos(w0), sinw = sin(w0);
            double alpha = sinw/(2*Q);
            double a0 = 1 + alpha/A;
            b0 = (1 + alpha*A)/a0;
            b1 = -2*cosw/a0;
            b2 = (1 - alpha*A)/a0;
            a1 = -2*cosw/a0;
            a2 = (1 - alpha/A)/a0;
        }

        void reset() { z1l=z2l=z1r=z2r=0; }
    };

    Biquad hpf, lowShelf, lowMid, highMid, highShelf, lpf;
    float sampleRate = 44100.0f;
    float lastParams[6] = {-99,-99,-99,-99,-99,-99};

    Shape() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(HIGH_PASS_PARAM,  0.f, 1.f, 0.f, "High Pass",  "Hz", 0, 1);
        configParam(LOW_SHELF_PARAM,  -1.f, 1.f, 0.f, "Low Shelf",  "dB", 0, 15);
        configParam(LOW_MID_PARAM,    -1.f, 1.f, 0.f, "Low Mid",    "dB", 0, 15);
        configParam(HIGH_MID_PARAM,   -1.f, 1.f, 0.f, "High Mid",   "dB", 0, 15);
        configParam(HIGH_SHELF_PARAM, -1.f, 1.f, 0.f, "High Shelf", "dB", 0, 15);
        configParam(LOW_PASS_PARAM,   0.f, 1.f, 0.f, "Low Pass",   "Hz", 0, 1);
        configInput(AUDIO_IN_L_INPUT,  "Audio In L");
        configInput(AUDIO_IN_R_INPUT,  "Audio In R");
        configOutput(AUDIO_OUT_L_OUTPUT, "Audio Out L");
        configOutput(AUDIO_OUT_R_OUTPUT, "Audio Out R");
    }

    void onSampleRateChange() override {
        sampleRate = APP->engine->getSampleRate();
        updateFilters(true);
    }

    void updateFilters(bool force = false) {
        float p[6];
        for (int i = 0; i < 6; i++) p[i] = params[i].getValue();
        bool changed = force;
        for (int i = 0; i < 6; i++) {
            if (p[i] != lastParams[i]) { changed = true; lastParams[i] = p[i]; }
        }
        if (!changed) return;

        if (p[HIGH_PASS_PARAM] > 0.01f)
            hpf.setHighPass(20.0 + p[HIGH_PASS_PARAM] * 380.0, sampleRate);

        lowShelf.setLowShelf(200.0, p[LOW_SHELF_PARAM] * 15.0, sampleRate);
        lowMid.setPeak(360.0, p[LOW_MID_PARAM] * 15.0, 0.7, sampleRate);
        highMid.setPeak(3200.0, p[HIGH_MID_PARAM] * 15.0, 0.7, sampleRate);
        highShelf.setHighShelf(8000.0, p[HIGH_SHELF_PARAM] * 15.0, sampleRate);

        if (p[LOW_PASS_PARAM] > 0.01f)
            lpf.setLowPass(20000.0 - p[LOW_PASS_PARAM] * 19000.0, sampleRate);
    }

    void process(const ProcessArgs& args) override {
        updateFilters();

        double inL = inputs[AUDIO_IN_L_INPUT].getVoltage();
        double inR = inputs[AUDIO_IN_R_INPUT].isConnected() ?
                     inputs[AUDIO_IN_R_INPUT].getVoltage() : inL;

        if (params[HIGH_PASS_PARAM].getValue() > 0.01f) hpf.process(inL, inR);
        lowShelf.process(inL, inR);
        lowMid.process(inL, inR);
        highMid.process(inL, inR);
        highShelf.process(inL, inR);
        if (params[LOW_PASS_PARAM].getValue() > 0.01f) lpf.process(inL, inR);

        outputs[AUDIO_OUT_L_OUTPUT].setVoltage((float)inL);
        outputs[AUDIO_OUT_R_OUTPUT].setVoltage((float)inR);
    }
};

struct ShapeWidget : ModuleWidget {
    ShapeWidget(Shape* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Shape.svg")));




        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(11.76f, 33.62f)), module, Shape::HIGH_PASS_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(29.33f, 33.70f)), module, Shape::LOW_SHELF_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(11.76f, 55.68f)), module, Shape::LOW_MID_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(29.33f, 55.76f)), module, Shape::HIGH_MID_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(11.76f, 77.91f)), module, Shape::HIGH_SHELF_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(29.33f, 77.99f)), module, Shape::LOW_PASS_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.79f, 102.78f)), module, Shape::AUDIO_IN_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.79f, 116.30f)), module, Shape::AUDIO_IN_R_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.26f, 102.78f)), module, Shape::AUDIO_OUT_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.26f, 116.30f)), module, Shape::AUDIO_OUT_R_OUTPUT));
    }

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Manual", "", []() {
            system::openBrowser("https://github.com/submitaudio/submit-vcv-modules/blob/master/MANUAL.md#shape");
        }));
        menu->addChild(createMenuItem("submitaudio.nl", "", []() {
            system::openBrowser(SUBMIT_URL);
        }));
        menu->addChild(createMenuItem("Report a Bug", "", []() {
            system::openBrowser("https://github.com/submitaudio/submit-vcv-modules/issues");
        }));
    }
};

Model* modelShape = createModel<Shape, ShapeWidget>("Shape");
