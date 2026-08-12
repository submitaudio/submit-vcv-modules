// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// Licensed under GPL v3 — see LICENSE file for details
// https://github.com/submitaudio/submit-vcv-modules

#include "plugin.hpp"
#include <algorithm>
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

    struct HighPassQuantity : ParamQuantity {
        std::string getDisplayValueString() override {
            float value = getValue();
            if (value <= 0.001f)
                return "Off";
            float frequency = 16.f * std::pow(350.f / 16.f, value);
            return string::f("%.1f Hz", frequency);
        }
    };

    struct LowPassQuantity : ParamQuantity {
        std::string getDisplayValueString() override {
            float value = getValue();
            if (value <= 0.001f)
                return "Off";
            float frequency = 22000.f * std::pow(3000.f / 22000.f, value);
            return string::f("%.0f Hz", frequency);
        }
    };

    struct StereoBiquad {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double z1L = 0.0, z2L = 0.0, z1R = 0.0, z2R = 0.0;

        static double safeFrequency(double frequency, double sampleRate) {
            return std::max(5.0, std::min(frequency, sampleRate * 0.45));
        }

        void setCoefficients(double newB0, double newB1, double newB2,
                             double newA1, double newA2) {
            b0 = newB0;
            b1 = newB1;
            b2 = newB2;
            a1 = newA1;
            a2 = newA2;
        }

        void process(double& left, double& right) {
            double outL = b0 * left + z1L;
            z1L = b1 * left - a1 * outL + z2L;
            z2L = b2 * left - a2 * outL;
            left = outL;

            double outR = b0 * right + z1R;
            z1R = b1 * right - a1 * outR + z2R;
            z2R = b2 * right - a2 * outR;
            right = outR;
        }

        void setHighPass(double frequency, double q, double sampleRate) {
            frequency = safeFrequency(frequency, sampleRate);
            double w0 = 2.0 * M_PI * frequency / sampleRate;
            double cosW = std::cos(w0);
            double alpha = std::sin(w0) / (2.0 * q);
            double a0 = 1.0 + alpha;
            setCoefficients(
                ((1.0 + cosW) * 0.5) / a0,
                -(1.0 + cosW) / a0,
                ((1.0 + cosW) * 0.5) / a0,
                (-2.0 * cosW) / a0,
                (1.0 - alpha) / a0);
        }

        void setLowPass(double frequency, double q, double sampleRate) {
            frequency = safeFrequency(frequency, sampleRate);
            double w0 = 2.0 * M_PI * frequency / sampleRate;
            double cosW = std::cos(w0);
            double alpha = std::sin(w0) / (2.0 * q);
            double a0 = 1.0 + alpha;
            setCoefficients(
                ((1.0 - cosW) * 0.5) / a0,
                (1.0 - cosW) / a0,
                ((1.0 - cosW) * 0.5) / a0,
                (-2.0 * cosW) / a0,
                (1.0 - alpha) / a0);
        }

        void setLowShelf(double frequency, double gainDb, double slope, double sampleRate) {
            frequency = safeFrequency(frequency, sampleRate);
            double amplitude = std::pow(10.0, gainDb / 40.0);
            double w0 = 2.0 * M_PI * frequency / sampleRate;
            double cosW = std::cos(w0);
            double sinW = std::sin(w0);
            double alpha = sinW * 0.5 * std::sqrt((amplitude + 1.0 / amplitude)
                * (1.0 / slope - 1.0) + 2.0);
            double twoRootAAlpha = 2.0 * std::sqrt(amplitude) * alpha;
            double a0 = (amplitude + 1.0) + (amplitude - 1.0) * cosW + twoRootAAlpha;
            setCoefficients(
                amplitude * ((amplitude + 1.0) - (amplitude - 1.0) * cosW + twoRootAAlpha) / a0,
                2.0 * amplitude * ((amplitude - 1.0) - (amplitude + 1.0) * cosW) / a0,
                amplitude * ((amplitude + 1.0) - (amplitude - 1.0) * cosW - twoRootAAlpha) / a0,
                -2.0 * ((amplitude - 1.0) + (amplitude + 1.0) * cosW) / a0,
                ((amplitude + 1.0) + (amplitude - 1.0) * cosW - twoRootAAlpha) / a0);
        }

        void setHighShelf(double frequency, double gainDb, double slope, double sampleRate) {
            frequency = safeFrequency(frequency, sampleRate);
            double amplitude = std::pow(10.0, gainDb / 40.0);
            double w0 = 2.0 * M_PI * frequency / sampleRate;
            double cosW = std::cos(w0);
            double sinW = std::sin(w0);
            double alpha = sinW * 0.5 * std::sqrt((amplitude + 1.0 / amplitude)
                * (1.0 / slope - 1.0) + 2.0);
            double twoRootAAlpha = 2.0 * std::sqrt(amplitude) * alpha;
            double a0 = (amplitude + 1.0) - (amplitude - 1.0) * cosW + twoRootAAlpha;
            setCoefficients(
                amplitude * ((amplitude + 1.0) + (amplitude - 1.0) * cosW + twoRootAAlpha) / a0,
                -2.0 * amplitude * ((amplitude - 1.0) + (amplitude + 1.0) * cosW) / a0,
                amplitude * ((amplitude + 1.0) + (amplitude - 1.0) * cosW - twoRootAAlpha) / a0,
                2.0 * ((amplitude - 1.0) - (amplitude + 1.0) * cosW) / a0,
                ((amplitude + 1.0) - (amplitude - 1.0) * cosW - twoRootAAlpha) / a0);
        }

        void setPeak(double frequency, double gainDb, double q, double sampleRate) {
            frequency = safeFrequency(frequency, sampleRate);
            double amplitude = std::pow(10.0, gainDb / 40.0);
            double w0 = 2.0 * M_PI * frequency / sampleRate;
            double cosW = std::cos(w0);
            double alpha = std::sin(w0) / (2.0 * q);
            double a0 = 1.0 + alpha / amplitude;
            setCoefficients(
                (1.0 + alpha * amplitude) / a0,
                (-2.0 * cosW) / a0,
                (1.0 - alpha * amplitude) / a0,
                (-2.0 * cosW) / a0,
                (1.0 - alpha / amplitude) / a0);
        }

        void reset() {
            z1L = z2L = z1R = z2R = 0.0;
        }
    };

    struct StereoOnePoleHighPass {
        double b0 = 1.0, b1 = -1.0, a1 = 0.0;
        double z1L = 0.0, z1R = 0.0;

        void setCutoff(double frequency, double sampleRate) {
            frequency = std::max(5.0, std::min(frequency, sampleRate * 0.45));
            double k = std::tan(M_PI * frequency / sampleRate);
            double normal = 1.0 / (1.0 + k);
            b0 = normal;
            b1 = -normal;
            a1 = (k - 1.0) * normal;
        }

        void process(double& left, double& right) {
            double outL = b0 * left + z1L;
            z1L = b1 * left - a1 * outL;
            left = outL;
            double outR = b0 * right + z1R;
            z1R = b1 * right - a1 * outR;
            right = outR;
        }

        void reset() {
            z1L = z1R = 0.0;
        }
    };

    StereoOnePoleHighPass hpfFirstOrder;
    StereoBiquad hpfSecondOrder;
    StereoBiquad lowShelf;
    StereoBiquad lowMid;
    StereoBiquad highMid;
    StereoBiquad highShelf;
    StereoBiquad lowPass;

    float sampleRate = 44100.f;
    float smoothedParams[6] = {};
    float smoothingCoefficient = 1.f;
    float highPassMix = 0.f;
    float lowPassMix = 0.f;
    int coefficientDivider = 0;
    bool smoothingInitialized = false;

    Shape() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam<HighPassQuantity>(HIGH_PASS_PARAM, 0.f, 1.f, 0.f, "High Pass");
        configParam(LOW_SHELF_PARAM,  -1.f, 1.f, 0.f, "Low Shelf",  "dB", 0, 15);
        configParam(LOW_MID_PARAM,    -1.f, 1.f, 0.f, "Low Mid",    "dB", 0, 15);
        configParam(HIGH_MID_PARAM,   -1.f, 1.f, 0.f, "High Mid",   "dB", 0, 15);
        configParam(HIGH_SHELF_PARAM, -1.f, 1.f, 0.f, "High Shelf", "dB", 0, 15);
        configParam<LowPassQuantity>(LOW_PASS_PARAM, 0.f, 1.f, 0.f, "Low Pass");
        configInput(AUDIO_IN_L_INPUT,  "Audio In L");
        configInput(AUDIO_IN_R_INPUT,  "Audio In R");
        configOutput(AUDIO_OUT_L_OUTPUT, "Audio Out L");
        configOutput(AUDIO_OUT_R_OUTPUT, "Audio Out R");
    }

    void onSampleRateChange() override {
        sampleRate = APP->engine->getSampleRate();
        smoothingCoefficient = 1.f - std::exp(-1.f / (0.015f * sampleRate));
        coefficientDivider = 0;
        updateFilters();
    }

    void onReset() override {
        hpfFirstOrder.reset();
        hpfSecondOrder.reset();
        lowShelf.reset();
        lowMid.reset();
        highMid.reset();
        highShelf.reset();
        lowPass.reset();
        smoothingInitialized = false;
        highPassMix = 0.f;
        lowPassMix = 0.f;
        coefficientDivider = 0;
    }

    static float exponentialMap(float value, float minimum, float maximum) {
        return minimum * std::pow(maximum / minimum, clamp(value, 0.f, 1.f));
    }

    static double proportionalQ(double gainDb, double minimumQ, double maximumQ) {
        double amount = std::max(0.0, std::min(std::abs(gainDb) / 15.0, 1.0));
        return minimumQ + (maximumQ - minimumQ) * amount;
    }

    void updateFilters() {
        double hpFrequency = exponentialMap(smoothedParams[HIGH_PASS_PARAM], 16.f, 350.f);
        double lpFrequency = exponentialMap(smoothedParams[LOW_PASS_PARAM], 22000.f, 3000.f);
        double lowShelfGain = smoothedParams[LOW_SHELF_PARAM] * 15.0;
        double lowMidGain = smoothedParams[LOW_MID_PARAM] * 15.0;
        double highMidGain = smoothedParams[HIGH_MID_PARAM] * 15.0;
        double highShelfGain = smoothedParams[HIGH_SHELF_PARAM] * 15.0;

        // SSL 4K-style 18 dB/oct HPF: one real pole plus a Q=1 complex pair.
        hpfFirstOrder.setCutoff(hpFrequency, sampleRate);
        hpfSecondOrder.setHighPass(hpFrequency, 1.0, sampleRate);

        // Fixed musical centres retain the six-knob workflow while following
        // the classic LF / LMF / HMF / HF channel-EQ topology.
        lowShelf.setLowShelf(80.0, lowShelfGain, 0.82, sampleRate);
        lowMid.setPeak(400.0, lowMidGain,
            proportionalQ(lowMidGain, 0.65, 1.35), sampleRate);
        highMid.setPeak(2500.0, highMidGain,
            proportionalQ(highMidGain, 0.70, 1.55), sampleRate);
        highShelf.setHighShelf(10000.0, highShelfGain, 0.82, sampleRate);
        lowPass.setLowPass(lpFrequency, M_SQRT1_2, sampleRate);
    }

    void process(const ProcessArgs& args) override {
        if (!smoothingInitialized) {
            for (int i = 0; i < 6; ++i)
                smoothedParams[i] = params[i].getValue();
            highPassMix = smoothedParams[HIGH_PASS_PARAM] > 0.001f ? 1.f : 0.f;
            lowPassMix = smoothedParams[LOW_PASS_PARAM] > 0.001f ? 1.f : 0.f;
            smoothingInitialized = true;
            updateFilters();
        }

        for (int i = 0; i < 6; ++i) {
            float target = params[i].getValue();
            smoothedParams[i] += smoothingCoefficient * (target - smoothedParams[i]);
        }

        float targetHighPassMix = params[HIGH_PASS_PARAM].getValue() > 0.001f ? 1.f : 0.f;
        float targetLowPassMix = params[LOW_PASS_PARAM].getValue() > 0.001f ? 1.f : 0.f;
        highPassMix += smoothingCoefficient * (targetHighPassMix - highPassMix);
        lowPassMix += smoothingCoefficient * (targetLowPassMix - lowPassMix);

        if (++coefficientDivider >= 16) {
            coefficientDivider = 0;
            updateFilters();
        }

        double inputL = inputs[AUDIO_IN_L_INPUT].getVoltage();
        double inputR = inputs[AUDIO_IN_R_INPUT].isConnected()
            ? inputs[AUDIO_IN_R_INPUT].getVoltage() : inputL;
        if (!std::isfinite(inputL))
            inputL = 0.0;
        if (!std::isfinite(inputR))
            inputR = 0.0;

        double left = inputL;
        double right = inputR;

        double filteredL = left;
        double filteredR = right;
        hpfFirstOrder.process(filteredL, filteredR);
        hpfSecondOrder.process(filteredL, filteredR);
        left += (filteredL - left) * highPassMix;
        right += (filteredR - right) * highPassMix;

        lowShelf.process(left, right);
        lowMid.process(left, right);
        highMid.process(left, right);
        highShelf.process(left, right);

        filteredL = left;
        filteredR = right;
        lowPass.process(filteredL, filteredR);
        left += (filteredL - left) * lowPassMix;
        right += (filteredR - right) * lowPassMix;

        outputs[AUDIO_OUT_L_OUTPUT].setVoltage(std::isfinite(left) ? (float)left : 0.f);
        outputs[AUDIO_OUT_R_OUTPUT].setVoltage(std::isfinite(right) ? (float)right : 0.f);
    }
};

struct ShapeWidget : SubmitModuleWidget {
    ShapeWidget(Shape* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Shape.svg")));




        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(11.76f, 33.62f)), module, Shape::HIGH_PASS_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(29.14f, 33.62f)), module, Shape::LOW_SHELF_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(11.76f, 55.68f)), module, Shape::LOW_MID_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(29.14f, 55.68f)), module, Shape::HIGH_MID_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(11.76f, 77.91f)), module, Shape::HIGH_SHELF_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(29.14f, 77.91f)), module, Shape::LOW_PASS_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.79f, 102.78f)), module, Shape::AUDIO_IN_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.79f, 116.30f)), module, Shape::AUDIO_IN_R_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.26f, 102.78f)), module, Shape::AUDIO_OUT_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.26f, 116.30f)), module, Shape::AUDIO_OUT_R_OUTPUT));
    }

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Manual", "", []() {
            system::openBrowser("https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/shape/");
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

Model* modelShape = createModel<Shape, ShapeWidget>("Shape");
