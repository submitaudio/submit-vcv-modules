#include "plugin.hpp"

struct Impact : Module {
    enum ParamId {
        PITCH_PARAM,
        DECAY_PARAM,
        PUNCH_PARAM,
        MORPH_PARAM,
        HARM_PARAM,
        FOLD_PARAM,
        NOISE_PARAM,
        SPREAD_PARAM,
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
    float accLevel  = 1.f;
    float accSmooth = 1.f;

    float fmPhase[3]  = {};
    float modPhase[3] = {};

    float dcX = 0.f;
    float dcY = 0.f;

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
        configParam(SPREAD_PARAM,  0.f,   1.f,  0.f,  "Spread");
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
        configOutput(OUT_L_OUTPUT, "Out L");
        configOutput(OUT_R_OUTPUT, "Out R");
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

    float dcBlock(float x) {
        dcY = x - dcX + 0.9995f * dcY;
        dcX = x;
        return dcY;
    }

    void process(const ProcessArgs& args) override {

        bool tryMePressed = params[TRYME_PARAM].getValue() > 0.5f;
        float trigVoltage = inputs[TRIG_INPUT].getVoltage();
        if (tryMePressed) trigVoltage = 5.f;

        if (trigIn.process(trigVoltage, 0.1f, 2.f)) {
            accLevel = (inputs[ACC_INPUT].isConnected() && inputs[ACC_INPUT].getVoltage() > 1.f) ? 1.5f : 1.f;
            waitZero = true;
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

        if (waitZero) {
            if (prevOsc0 <= 0.f && osc0now > 0.f) {
                waitZero = false;
                active   = true;
                t        = 0.f;
                for (int i = 1; i < NUM_OSC; i++)
                    phase[i] = phase[0] / (float)(i + 1);
                for (int i = 0; i < 3; i++) {
                    fmPhase[i]  = 0.f;
                    modPhase[i] = 0.f;
                }
            }
        }
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
            float spreadKnob = params[SPREAD_PARAM].getValue();
            float nlenKnob   = params[NLEN_PARAM].getValue();
            if (inputs[NLEN_CV].isConnected())
                nlenKnob = clamp(nlenKnob + inputs[NLEN_CV].getVoltage() / 5.f * params[ATT_NOISE_PARAM].getValue(), 0.f, 1.f);

            int noiseType = 2 - (int)params[NTYPE_PARAM].getValue();

            float decayRate = std::pow(10.f, (1.f - decayKnob) * 2.3f) * 0.4f + 0.4f;
            float masterEnv = std::exp(-t * decayRate);

            float sum = 0.f;

            if (mode == 0) {
                static const float inharmonic[NUM_OSC] = {
                    1.0f, 2.83f, 5.24f, 8.66f, 13.4f, 20.1f
                };
                float totalAmp = 0.f;
                for (int i = 0; i < NUM_OSC; i++) {
                    float harmonic = (float)(i + 1);
                    float mult = harmonic + spreadKnob * (inharmonic[i] - harmonic);
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
                float fmRatio = 0.5f + harmKnob * 1.5f + spreadKnob * 1.f;
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

            float noiseDecayRate = std::pow(10.f, (1.f - nlenKnob) * 3.0f) * 0.05f + 0.003f;
            float noiseEnv  = std::exp(-t * noiseDecayRate);
            float noiseAmt  = std::pow(noiseKnob, 1.5f) * 1.4f;

            float rawNoiseL = 0.f;
            float rawNoiseR = 0.f;

            if (noiseType == 0) {
                rawNoiseL = nextRadioNoise(args.sampleTime);
                rawNoiseR = nextRadioNoiseR(args.sampleTime);
            } else if (noiseType == 1) {
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

            t += args.sampleTime;
            if (t > 10.f) { active = false; }

            accSmooth += 0.001f * (accLevel - accSmooth);
            float accentBodyL = dcBlock(bodyOut * accSmooth + noiseOut);
            float accentBodyR = dcBlock(bodyOut * accSmooth + noiseOutR);
            outputs[OUT_L_OUTPUT].setVoltage(clamp(accentBodyL * 5.f, -10.f, 10.f));
            outputs[OUT_R_OUTPUT].setVoltage(clamp(accentBodyR * 5.f, -10.f, 10.f));
        }

        if (!active) {
            outputs[OUT_L_OUTPUT].setVoltage(0.f);
            outputs[OUT_R_OUTPUT].setVoltage(0.f);
        }

        lights[TRIG_LIGHT].setBrightness(ledPulse.process(args.sampleTime) ? 1.f : 0.f);
        lights[ACC_LIGHT].setBrightness(accLevel > 1.f && active ? 1.f : 0.f);
    }

    json_t* dataToJson() override { return json_object(); }
    void dataFromJson(json_t* rootJ) override { (void)rootJ; }
};

struct ImpactWidget : ModuleWidget {
    ImpactWidget(Impact* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Impact.svg")));

        // ── Knoppen ──────────────────────────
        addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(14.59f, 27.06f)), module, Impact::PITCH_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(34.48f, 29.50f)), module, Impact::DECAY_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(54.23f, 29.50f)), module, Impact::PUNCH_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(14.64f, 50.28f)), module, Impact::HARM_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(34.48f, 50.28f)), module, Impact::SPREAD_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(54.23f, 50.28f)), module, Impact::FOLD_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(14.64f, 70.55f)), module, Impact::MORPH_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(34.40f, 70.64f)), module, Impact::NOISE_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(54.23f, 70.64f)), module, Impact::NLEN_PARAM));

        // ── Attenuverters ─────────────────────
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(12.84f, 86.11f)), module, Impact::ATT_DECAY_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(27.82f, 86.11f)), module, Impact::ATT_PUNCH_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(43.20f, 86.11f)), module, Impact::ATT_MORPH_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(58.38f, 86.11f)), module, Impact::ATT_FOLD_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(73.92f, 86.11f)), module, Impact::ATT_NOISE_PARAM));

        // ── Schakelaars ───────────────────────
        addParam(createParamCentered<CKSS>(mm2px(Vec(71.53f, 26.18f)), module, Impact::MODE_PARAM));
        addParam(createParamCentered<CKSSThree>(mm2px(Vec(71.53f, 41.90f)), module, Impact::NTYPE_PARAM));

        // ── TRY ME knop ───────────────────────
        addParam(createParamCentered<VCVButton>(mm2px(Vec(75.06f, 58.65f)), module, Impact::TRYME_PARAM));

        // ── LEDs ──────────────────────────────
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(7.19f, 109.53f)), module, Impact::TRIG_LIGHT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(38.27f, 109.53f)), module, Impact::ACC_LIGHT));

        // ── CV Inputs ─────────────────────────
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.61f, 100.76f)), module, Impact::DECAY_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(27.69f, 100.76f)), module, Impact::PUNCH_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(43.20f, 100.76f)), module, Impact::MORPH_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(58.38f, 100.76f)), module, Impact::FOLD_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(73.95f, 100.76f)), module, Impact::NLEN_CV));

        // ── Trigger / Pitch / Accent ──────────
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.61f, 116.19f)), module, Impact::TRIG_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(27.69f, 116.19f)), module, Impact::VOCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(43.20f, 116.19f)), module, Impact::ACC_INPUT));

        // ── Outputs ───────────────────────────
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(58.38f, 116.19f)), module, Impact::OUT_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(73.95f, 116.19f)), module, Impact::OUT_R_OUTPUT));
    }
};

Model* modelImpact = createModel<Impact, ImpactWidget>("Impact");
