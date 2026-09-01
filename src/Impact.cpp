// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// SPDX-License-Identifier: GPL-3.0-or-later
// https://github.com/submitaudio/submit-vcv-modules

#include "plugin.hpp"

// ── Gedeelde knob structs ─────────────────────
struct SubmitImpactKnobMini : SvgKnob {
    SubmitImpactKnobMini() {
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobMini.svg")));
        shadow->opacity = 0.f;
    }
};

struct SubmitImpactKnobMedium : SvgKnob {
    SubmitImpactKnobMedium() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobMedium.svg")));
        shadow->opacity = 0.f;
    }
};

struct SubmitImpactKnobSmall : SvgKnob {
    SubmitImpactKnobSmall() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobSmall.svg")));
        shadow->opacity = 0.f;
    }
};

struct TryMeButton : SvgSwitch {
    TryMeButton() {
        momentary = true;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/TryMe_0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/TryMe_1.svg")));
    }
};

struct Impact : Module {
    enum ParamId {
        PITCH_PARAM,
        DECAY_PARAM,
        PUNCH_PARAM,
        MORPH_PARAM,
        HARM_PARAM,
        FOLD_PARAM,
        NOISE_PARAM,
        SNAP_PARAM,
        NLEN_PARAM,
        NTYPE_PARAM,
        MODE_PARAM,
        ATT_DECAY_PARAM,
        ATT_PUNCH_PARAM,
        ATT_MORPH_PARAM,
        ATT_NOISE_PARAM,
        ATT_FOLD_PARAM,
        TRYME_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        TRIG_INPUT,
        VOCT_INPUT,
        PUNCH_CV,
        MORPH_CV,
        DECAY_CV,
        NLEN_CV,
        FOLD_CV,
        ACC_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUT_L_OUTPUT,
        OUT_R_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        TRIG_LIGHT,
        ACC_LIGHT,
        LIGHTS_LEN
    };

    static const int NUM_OSC = 6;
    float phase[NUM_OSC] = {};
    float t        = 0.f;
    bool  active   = false;
    bool  waitZero  = false;
    float accLevel  = 1.f;  // opgeslagen bij trigger
    float accSmooth = 1.f;  // gesmoothe versie
    float attackT  = 0.f;   // korte anti-click fade-in
    static constexpr float ATTACK_TIME = 0.00075f; // 0.75ms

    float fmPhase[3]  = {};
    float modPhase[3] = {};


    float pinkB0     = 0.f;
    float pinkB1     = 0.f;
    float pinkB2     = 0.f;
    float noisePrev  = 0.f;
    float prevOsc0   = 0.f;

    float pinkB0R    = 0.f;
    float pinkB1R    = 0.f;
    float pinkB2R    = 0.f;
    float noisePrevR = 0.f;

    float foldNoisePrev  = 0.f;
    float foldNoisePrevR = 0.f;

    float staticLfo   = 0.f;
    float staticLfoR  = 0.f;
    float staticPink  = 0.f;
    float staticPinkR = 0.f;

    float dustAmp  = 0.f;
    float dustLen  = 0.f;
    float dustGap  = 0.f;
    float dustAmpR = 0.f;
    float dustLenR = 0.f;
    float dustGapR = 0.f;

    dsp::SchmittTrigger trigIn;
    dsp::PulseGenerator ledPulse;

    Impact() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PITCH_PARAM,  30.f, 100.f, 55.f,  "Pitch", " Hz");
        configParam(DECAY_PARAM,   0.f,   1.f,  0.45f, "Decay");
        configParam(PUNCH_PARAM,   0.f,   1.f,  0.66868f, "Punch");
        configParam(MORPH_PARAM,   0.f,   1.f,  0.05f, "Morph");
        configParam(HARM_PARAM,    0.f,   1.f,  0.1f, "Harm");
        configParam(FOLD_PARAM,    0.f,   1.f,  0.f,  "Fold");
        configParam(NOISE_PARAM,   0.f,   1.f,  0.15783f, "Noise");
        configParam(SNAP_PARAM,    0.f,   1.f,  0.25f, "Snap");
        configParam(NLEN_PARAM,    0.f,   1.f,  0.29036f, "Noise Length");
        configSwitch(NTYPE_PARAM,  0.f,   2.f,  0.f,  "Noise Type", {"Rumble", "Crunch", "Dust"});
        configSwitch(MODE_PARAM,   0.f,   1.f,  1.f,  "Mode", {"Harsh", "Pure"});
        configParam(ATT_DECAY_PARAM, -1.f, 1.f, 0.f, "Decay Attenuverter");
        configParam(ATT_PUNCH_PARAM, -1.f, 1.f, 0.f, "Punch Attenuverter");
        configParam(ATT_MORPH_PARAM, -1.f, 1.f, 0.f, "Morph Attenuverter");
        configParam(ATT_NOISE_PARAM, -1.f, 1.f, 0.f, "Noise Attenuverter");
        configParam(ATT_FOLD_PARAM,  -1.f, 1.f, 0.f, "Fold Attenuverter");
        configButton(TRYME_PARAM, "Try Me");
        configInput(TRIG_INPUT,  "Trigger");
        configInput(VOCT_INPUT,  "Pitch CV (1V/oct)");
        configInput(PUNCH_CV,    "Punch CV");
        configInput(MORPH_CV,    "Morph CV");
        configInput(DECAY_CV,    "Decay CV");
        configInput(NLEN_CV,     "Noise Length CV");
        configInput(FOLD_CV,     "Fold CV");
        configInput(ACC_INPUT,   "Accent");
        configOutput(OUT_L_OUTPUT, "Audio Out L");
        configOutput(OUT_R_OUTPUT, "Audio Out R");
    }

    uint32_t rng = 22317u;
    float nextWhite() {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return (float)(int32_t)rng / 2147483648.f;
    }

    float nextRadioNoise(float sampleTime) {
        if (dustLen > 0.f) {
            dustLen -= sampleTime;
            float white = nextWhite();
            pinkB0 = 0.85f * pinkB0 + white * 0.15f;
            return std::tanh(pinkB0 * dustAmp * 3.f);
        } else if (dustGap > 0.f) {
            dustGap -= sampleTime;
            return 0.f;
        } else {
            float r1 = (nextWhite() + 1.f) * 0.5f;
            float r2 = (nextWhite() + 1.f) * 0.5f;
            float r3 = (nextWhite() + 1.f) * 0.5f;
            dustAmp = 0.3f + r1 * 0.7f;
            dustLen = 0.0002f + r2 * 0.003f;
            dustGap = 0.001f + r3 * 0.015f;
            return 0.f;
        }
    }

    float nextRadioNoiseR(float sampleTime) {
        if (dustLenR > 0.f) {
            dustLenR -= sampleTime;
            float white = nextWhite();
            pinkB0R = 0.85f * pinkB0R + white * 0.15f;
            return std::tanh(pinkB0R * dustAmpR * 3.f);
        } else if (dustGapR > 0.f) {
            dustGapR -= sampleTime;
            return 0.f;
        } else {
            float r1 = (nextWhite() + 1.f) * 0.5f;
            float r2 = (nextWhite() + 1.f) * 0.5f;
            float r3 = (nextWhite() + 1.f) * 0.5f;
            dustAmpR = 0.3f + r1 * 0.7f;
            dustLenR = 0.0002f + r2 * 0.003f;
            dustGapR = 0.001f + r3 * 0.018f;
            return 0.f;
        }
    }

    float morphWave(float ph, float morph) {
        float sine = std::sin(2.f * float(M_PI) * ph);
        float tri  = (ph < 0.5f) ? (4.f * ph - 1.f) : (3.f - 4.f * ph);
        float saw  = 2.f * ph - 1.f;
        float sqr  = (ph < 0.5f) ? 1.f : -1.f;
        float m = morph * 3.f;
        if (m < 1.f) return sine + m * (tri - sine);
        else if (m < 2.f) return tri + (m - 1.f) * (saw - tri);
        else return saw + (m - 2.f) * (sqr - saw);
    }

    float fold(float x, float amount) {
        if (amount < 0.001f) return x;
        float y = x * (1.f + amount * 4.f);
        int passes = 1 + (int)(amount * 3.f);
        for (int i = 0; i < passes; i++) {
            y = std::fmod(y + 1.f, 4.f);
            if (y < 0.f) y += 4.f;
            if (y > 2.f) y = 4.f - y;
            y -= 1.f;
        }
        return y / (1.f + amount * 1.5f);
    }

    void process(const ProcessArgs& args) override {

        // TRY ME knop — stuurt interne trigger
        bool tryMePressed = params[TRYME_PARAM].getValue() > 0.5f;
        float trigVoltage = inputs[TRIG_INPUT].getVoltage();
        if (tryMePressed) trigVoltage = 5.f;

        if (trigIn.process(trigVoltage, 0.1f, 2.f)) {
            // Sla ACC niveau op bij trigger moment
            accLevel = (inputs[ACC_INPUT].isConnected() && inputs[ACC_INPUT].getVoltage() > 1.f) ? 1.5f : 1.f;
            waitZero = false;
            active = true;
            t = 0.f;
            attackT = 0.f;
            for (int i = 0; i < 6; i++) phase[i] = 0.f;
            for (int i = 0; i < 3; i++) { fmPhase[i] = 0.f; modPhase[i] = 0.f; }
            dustLen = dustGap = dustAmp = 0.f;
            dustLenR = dustGapR = dustAmpR = 0.f;
            foldNoisePrev = foldNoisePrevR = 0.f;
            staticLfo = staticLfoR = staticPink = staticPinkR = 0.f;
            pinkB0 = pinkB1 = pinkB2 = 0.f;
            noisePrev = 0.f;
            pinkB0R = pinkB1R = pinkB2R = noisePrevR = 0.f;
            ledPulse.trigger(0.05f);
        }

        float baseFreq = params[PITCH_PARAM].getValue();
        if (inputs[VOCT_INPUT].isConnected()) {
            float cv = inputs[VOCT_INPUT].getVoltage();
            baseFreq = clamp(baseFreq * std::pow(2.f, cv), 10.f, 2000.f);
        }

        float punchKnob = params[PUNCH_PARAM].getValue();
        if (inputs[PUNCH_CV].isConnected())
            punchKnob = clamp(punchKnob + inputs[PUNCH_CV].getVoltage() / 5.f * params[ATT_PUNCH_PARAM].getValue(), 0.f, 1.f);

        float morphKnob = params[MORPH_PARAM].getValue();
        if (inputs[MORPH_CV].isConnected())
            morphKnob = clamp(morphKnob + inputs[MORPH_CV].getVoltage() / 5.f * params[ATT_MORPH_PARAM].getValue(), 0.f, 1.f);

        int mode = 1 - (int)params[MODE_PARAM].getValue();

        float punchCurved = std::pow(punchKnob, 1.8f);
        float pitchMult   = 1.f;
        if (active) {
            float pitchRate = 15.f + punchCurved * 200.f;
            float pitchEnv  = std::exp(-t * pitchRate);
            pitchMult = 1.f + punchCurved * 8.f * pitchEnv;
        }

        float freq0 = clamp(baseFreq * pitchMult, 10.f, 20000.f);
        phase[0] += freq0 * args.sampleTime;
        if (phase[0] >= 1.f) phase[0] -= 1.f;
        float osc0now = morphWave(phase[0], morphKnob);

        prevOsc0 = osc0now;

        if (active) {
            float decayKnob = params[DECAY_PARAM].getValue();
            if (inputs[DECAY_CV].isConnected())
                decayKnob = clamp(decayKnob + inputs[DECAY_CV].getVoltage() / 5.f * params[ATT_DECAY_PARAM].getValue(), 0.f, 1.f);

            float harmKnob   = params[HARM_PARAM].getValue();
            float foldKnob   = params[FOLD_PARAM].getValue();
            if (inputs[FOLD_CV].isConnected())
                foldKnob = clamp(foldKnob + inputs[FOLD_CV].getVoltage() / 5.f * params[ATT_FOLD_PARAM].getValue(), 0.f, 1.f);

            float noiseKnob  = params[NOISE_PARAM].getValue();
            float snapKnob   = params[SNAP_PARAM].getValue();
            float nlenKnob   = params[NLEN_PARAM].getValue();
            if (inputs[NLEN_CV].isConnected())
                nlenKnob = clamp(nlenKnob + inputs[NLEN_CV].getVoltage() / 5.f * params[ATT_NOISE_PARAM].getValue(), 0.f, 1.f);

            int noiseType = 2 - (int)params[NTYPE_PARAM].getValue();

            float decayRate = std::pow(10.f, (1.f - decayKnob) * 2.3f) * 0.4f + 0.4f;
            float masterEnv = std::exp(-t * decayRate);

            float sum = 0.f;

            if (mode == 0) {
                // ── PURE — additieve synthese ──────────
                float totalAmp = 0.f;
                for (int i = 0; i < NUM_OSC; i++) {
                    float harmonic = (float)(i + 1);
                    float mult = harmonic;
                    float freqMult = (i == 0) ? pitchMult : 1.f;
                    float freq = clamp(baseFreq * mult * freqMult, 10.f, 20000.f);
                    float wave = 0.f;
                    if (i == 0) {
                        wave = osc0now;
                    } else {
                        phase[i] += freq * args.sampleTime;
                        if (phase[i] >= 1.f) phase[i] -= 1.f;
                        wave = morphWave(phase[i], morphKnob);
                    }
                    float harmAmp = 0.f;
                    if (i == 0) {
                        harmAmp = 1.f;
                    } else {
                        float threshold = (float)i / (float)(NUM_OSC - 1);
                        float fade = clamp((harmKnob - threshold * 0.8f) / 0.2f, 0.f, 1.f);
                        float harmDecay = decayRate * (1.f + (float)i * 0.8f);
                        harmAmp = fade * std::exp(-t * harmDecay) * (1.f / (float)(i + 1));
                    }
                    sum      += wave * harmAmp;
                    totalAmp += harmAmp;
                }
                if (totalAmp > 0.f) sum /= totalAmp;
                sum *= masterEnv;

            } else {
                // ── HARSH — FM kick ────────────────────
                float fmRatio = 0.5f + harmKnob * 1.5f;
                float modFreq = clamp(baseFreq * 0.5f * fmRatio * pitchMult, 10.f, 20000.f);
                float fmIndex = 2.f + morphKnob * 6.f;
                fmPhase[0] += modFreq * args.sampleTime;
                if (fmPhase[0] >= 1.f) fmPhase[0] -= 1.f;
                float modSig   = std::sin(2.f * float(M_PI) * fmPhase[0]) * fmIndex;
                float carPhase = phase[0] + modSig * 0.1f;
                float carrier  = std::sin(2.f * float(M_PI) * carPhase);
                sum = carrier * masterEnv;
                sum = fold(sum, foldKnob);
                sum = clamp(sum, -1.f, 1.f);
            }

            // ── Noise ──────────────────────────────
            float noiseDecayRate = std::pow(10.f, (1.f - nlenKnob) * 3.0f) * 0.05f + 0.003f;
            float noiseEnv  = std::exp(-t * noiseDecayRate);
            float noiseAmt  = std::pow(noiseKnob, 1.5f) * 1.4f;

            float rawNoiseL = 0.f;
            float rawNoiseR = 0.f;

            if (noiseType == 0) {
                // DUST
                rawNoiseL = nextRadioNoise(args.sampleTime);
                rawNoiseR = nextRadioNoiseR(args.sampleTime);

            } else if (noiseType == 1) {
                // CRUNCH
                float w1 = nextWhite();
                float w2 = nextWhite();
                float n  = std::tanh((w1 + w2 * 0.5f) * 4.f);
                float diff = n - foldNoisePrev;
                foldNoisePrev = n;
                rawNoiseL = std::tanh(diff * 6.f);

                float w3 = nextWhite();
                float w4 = nextWhite();
                float nR = std::tanh((w3 + w4 * 0.5f) * 4.f);
                float diffR = nR - foldNoisePrevR;
                foldNoisePrevR = nR;
                rawNoiseR = std::tanh(diffR * 6.f);

            } else {
                // RUMBLE
                float w  = nextWhite();
                float wR = nextWhite();
                staticPink  = 0.992f * staticPink  + w  * 0.008f;
                staticPinkR = 0.992f * staticPinkR + wR * 0.008f;
                staticLfo   = 0.988f * staticLfo   + staticPink  * 0.012f;
                staticLfoR  = 0.985f * staticLfoR  + staticPinkR * 0.015f;
                rawNoiseL = std::tanh(staticLfo  * 14.f) * 2.5f;
                rawNoiseR = std::tanh(staticLfoR * 14.f) * 2.5f;
            }

            float noiseOut  = fold(rawNoiseL * noiseEnv * noiseAmt, foldKnob);
            float noiseOutR = fold(rawNoiseR * noiseEnv * noiseAmt, foldKnob);
            float bodyOut   = fold(sum, foldKnob);
            float snapEnv = std::exp(-t * (1800.f + snapKnob * 2600.f));
            float snapTone = std::sin(2.f * float(M_PI) * phase[0] * (7.f + snapKnob * 13.f));
            float snapOut = snapTone * snapEnv * std::pow(snapKnob, 1.25f) * 0.65f;

            t += args.sampleTime;
            if (t > 10.f) { active = false; }
            // Smooth accLevel — voorkomt tikken bij overgang
            accSmooth = accLevel;
            attackT += args.sampleTime;
            float attackPhase = clamp(attackT / ATTACK_TIME, 0.f, 1.f);
            float attackEnv = attackPhase * attackPhase * (3.f - 2.f * attackPhase);
            float snapPhase = clamp(attackT / 0.00025f, 0.f, 1.f);
            float snapAttack = snapPhase * snapPhase * (3.f - 2.f * snapPhase);
            float outL = (bodyOut * accSmooth + noiseOut) * attackEnv + snapOut * snapAttack;
            float outR = (bodyOut * accSmooth + noiseOutR) * attackEnv + snapOut * snapAttack;
            outputs[OUT_L_OUTPUT].setVoltage(clamp(outL * 5.f, -10.f, 10.f));
            outputs[OUT_R_OUTPUT].setVoltage(clamp(outR * 5.f, -10.f, 10.f));
        }

        if (!active) {
            outputs[OUT_L_OUTPUT].setVoltage(0.f);
            outputs[OUT_R_OUTPUT].setVoltage(0.f);
        }

        lights[TRIG_LIGHT].setBrightness(ledPulse.process(args.sampleTime) ? 1.f : 0.f);
        // ACC LED — altijd checken ongeacht active
        lights[ACC_LIGHT].setBrightness(accLevel > 1.f && active ? 1.f : 0.f);
    }
};

struct ImpactWidget : SubmitModuleWidget {
    ImpactWidget(Impact* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Impact.svg")));

        // ── Knoppen ──────────────────────────
        addParam(createParamCentered<SubmitImpactKnobMedium>(
            Vec(43.137f, 80.012f), module, Impact::PITCH_PARAM));
        addParam(createParamCentered<SubmitImpactKnobSmall>(
            Vec(101.967f, 87.251f), module, Impact::DECAY_PARAM));
        addParam(createParamCentered<SubmitImpactKnobSmall>(
            Vec(160.356f, 87.251f), module, Impact::PUNCH_PARAM));
        addParam(createParamCentered<SubmitImpactKnobSmall>(
            Vec(43.307f, 148.697f), module, Impact::HARM_PARAM));
        addParam(createParamCentered<SubmitImpactKnobSmall>(
            Vec(101.967f, 148.697f), module, Impact::SNAP_PARAM));
        addParam(createParamCentered<SubmitImpactKnobSmall>(
            Vec(160.356f, 148.697f), module, Impact::FOLD_PARAM));
        addParam(createParamCentered<SubmitImpactKnobSmall>(
            Vec(43.307f, 208.643f), module, Impact::MORPH_PARAM));
        addParam(createParamCentered<SubmitImpactKnobSmall>(
            Vec(101.717f, 208.893f), module, Impact::NOISE_PARAM));
        addParam(createParamCentered<SubmitImpactKnobSmall>(
            Vec(160.356f, 208.893f), module, Impact::NLEN_PARAM));

        // ── Attenuverters ─────────────────────
        addParam(createParamCentered<SubmitImpactKnobMini>(
            Vec(37.965f, 254.656f), module, Impact::ATT_DECAY_PARAM));
        addParam(createParamCentered<SubmitImpactKnobMini>(
            Vec(82.255f, 254.656f), module, Impact::ATT_PUNCH_PARAM));
        addParam(createParamCentered<SubmitImpactKnobMini>(
            Vec(127.755f, 254.656f), module, Impact::ATT_MORPH_PARAM));
        addParam(createParamCentered<SubmitImpactKnobMini>(
            Vec(172.635f, 254.656f), module, Impact::ATT_FOLD_PARAM));
        addParam(createParamCentered<SubmitImpactKnobMini>(
            Vec(218.598f, 254.656f), module, Impact::ATT_NOISE_PARAM));

        // ── Schakelaars ───────────────────────
        addParam(createParam<CKSS>(
            Vec(204.271f, 67.418f), module, Impact::MODE_PARAM));
        addParam(createParam<CKSSThree>(
            Vec(204.271f, 109.832f), module, Impact::NTYPE_PARAM));

        // ── TRY ME knop ───────────────────────
        addParam(createParam<TryMeButton>(
            Vec(203.789f, 158.81f), module, Impact::TRYME_PARAM));

        // ── LEDs ──────────────────────────────
        addChild(createLightCentered<SmallLight<YellowLight>>(
            Vec(23.764f, 323.91f), module, Impact::TRIG_LIGHT));
        addChild(createLightCentered<SmallLight<YellowLight>>(
            Vec(115.677f, 323.91f), module, Impact::ACC_LIGHT));

        // ── CV Inputs ─────────────────────────
        addInput(createInputCentered<PJ301MPort>(
            Vec(37.298f, 297.981f), module, Impact::DECAY_CV));
        addInput(createInputCentered<PJ301MPort>(
            Vec(81.88f, 297.981f), module, Impact::PUNCH_CV));
        addInput(createInputCentered<PJ301MPort>(
            Vec(127.755f, 297.981f), module, Impact::MORPH_CV));
        addInput(createInputCentered<PJ301MPort>(
            Vec(172.635f, 297.981f), module, Impact::FOLD_CV));
        addInput(createInputCentered<PJ301MPort>(
            Vec(218.691f, 297.981f), module, Impact::NLEN_CV));

        // ── Trigger / Pitch / Accent ──────────
        addInput(createInputCentered<PJ301MPort>(
            Vec(37.298f, 343.604f), module, Impact::TRIG_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            Vec(81.88f, 343.604f), module, Impact::VOCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            Vec(127.755f, 343.604f), module, Impact::ACC_INPUT));

        // ── Outputs ───────────────────────────
        addOutput(createOutputCentered<PJ301MPort>(
            Vec(172.635f, 343.604f), module, Impact::OUT_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(
            Vec(218.691f, 343.604f), module, Impact::OUT_R_OUTPUT));
    }

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Manual", "", []() {
            system::openBrowser("https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/impact/");
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


Model* modelImpact = createModel<Impact, ImpactWidget>("Impact");
