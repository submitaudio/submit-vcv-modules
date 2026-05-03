#include "plugin.hpp"

// ═══════════════════════════════════════════════════════════
//   Poly 008  —  4-stem parafone DCO+VCF synthesizer
//   Korg Poly-800 geïnspireerd, 2026 upgrade
//   Poly kabel: 1x V/Oct + 1x Gate (4 kanalen)
//   MetaModule-compatibel: geen dynamic allocatie in process()
// ═══════════════════════════════════════════════════════════

// ─────────────────────────────────────────────
//  DSP helpers
// ─────────────────────────────────────────────

inline float softClip(float x) {
    if (x >  3.f) return  1.f;
    if (x < -3.f) return -1.f;
    return x * (27.f + x*x) / (27.f + 9.f*x*x);
}

struct OnePole {
    float z = 0.f;
    float process(float in, float coeff) {
        z = in + coeff * (z - in);
        return z;
    }
};

// ─────────────────────────────────────────────
//  ADSR Envelope
// ─────────────────────────────────────────────
struct ADSR {
    enum Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE };
    Stage stage = IDLE;
    float env   = 0.f;
    float attackCoef   = 0.f;
    float decayCoef    = 0.f;
    float sustainLevel = 0.f;
    float releaseCoef  = 0.f;

    void setParams(float a, float d, float s, float r, float sr) {
        auto coef = [&](float t) -> float {
            return (t < 1e-4f) ? 0.f : expf(-1.f / (t * sr));
        };
        attackCoef   = coef(a);
        decayCoef    = coef(d);
        sustainLevel = s;
        releaseCoef  = coef(r);
    }

    void gate(bool on) {
        if (on  && stage == IDLE)    stage = ATTACK;
        if (on  && stage == RELEASE) stage = ATTACK;
        if (!on && stage != IDLE)    stage = RELEASE;
    }

    float process() {
        switch (stage) {
            case ATTACK:
                env = 1.f + attackCoef * (env - 1.f);
                if (env >= 0.999f) { env = 1.f; stage = DECAY; }
                break;
            case DECAY:
                env = sustainLevel + decayCoef * (env - sustainLevel);
                if (fabsf(env - sustainLevel) < 1e-4f) { env = sustainLevel; stage = SUSTAIN; }
                break;
            case SUSTAIN:
                break;
            case RELEASE:
                env = releaseCoef * env;
                if (env < 1e-5f) { env = 0.f; stage = IDLE; }
                break;
            default: env = 0.f; break;
        }
        return env;
    }

    bool isIdle() { return stage == IDLE; }
};

// ─────────────────────────────────────────────
//  DCO Voice  (Poly-800 stijl: Walsh-saw mix)
// ─────────────────────────────────────────────
struct DCOVoice {
    float phase[4] = {0,0,0,0};
    float freq      = 440.f;
    float drift     = 0.f;
    bool  active    = false;

    static constexpr float SAW_COEF[4] = { 0.50f, 0.25f, 0.125f, 0.0625f };
    static constexpr float SQ_COEF[4]  = { 0.25f, 0.25f, 0.25f,  0.25f  };

    float process(float sr, bool sawMode,
                  bool oct16, bool oct8, bool oct4, bool oct2,
                  float driftAmt, float detuneSemitones) {
        if (!active) return 0.f;

        // Drift: op 0 volledig stil, op 1 duidelijk hoorbaar
        // Alleen updaten als driftAmt > 0
        if (driftAmt > 0.001f) {
            drift += (((float)rand() / RAND_MAX) * 2.f - 1.f) * 0.0001f;
            drift *= 0.999f;
        } else {
            // Langzaam naar 0 bewegen zodat geen sprong bij opzetten
            drift *= 0.995f;
        }
        // Kwadraat schaling: onderkant stil, bovenkant veel meer
        float driftCurve = driftAmt * driftAmt;
        float driftSt = drift * driftCurve * 0.8f;

        float f = freq * powf(2.f, (detuneSemitones + driftSt) / 12.f);

        // Octaaf frequenties: 16'=f/4, 8'=f/2, 4'=f, 2'=f*2
        float freqs[4] = { f * 0.25f, f * 0.5f, f, f * 2.f };
        bool  octs[4]  = { oct16, oct8, oct4, oct2 };

        const float* coef = sawMode ? SAW_COEF : SQ_COEF;
        float out = 0.f, totalCoef = 0.f;

        for (int i = 0; i < 4; i++) {
            phase[i] += freqs[i] / sr;
            if (phase[i] >= 1.f) phase[i] -= 1.f;
            if (octs[i]) {
                float sq = (phase[i] < 0.5f) ? 1.f : -1.f;
                out += sq * coef[i];
                totalCoef += coef[i];
            }
        }

        return (totalCoef > 0.f) ? out / totalCoef : 0.f;
    }
};

// ─────────────────────────────────────────────
//  Per-stem Moog-stijl ladder filter
//  Elke stem heeft eigen filter instantie,
//  knoppen sturen alle 4 tegelijk (zoals Poly-800)
// ─────────────────────────────────────────────
struct LadderFilter {
    float s[4] = {0,0,0,0};

    float process(float in, float cutoff, float resonance, float sr) {
        float f = 2.f * cutoff / sr;
        f = clamp(f, 0.f, 0.999f);
        float k = 4.f * resonance;

        float fb = s[3] * k;
        float x  = softClip(in - fb);

        s[0] = x    * f + s[0] * (1.f - f);
        s[1] = s[0] * f + s[1] * (1.f - f);
        s[2] = s[1] * f + s[2] * (1.f - f);
        s[3] = s[2] * f + s[3] * (1.f - f);

        return s[3];
    }
};

// ─────────────────────────────────────────────
//  LFO
// ─────────────────────────────────────────────
struct LFO808 {
    float phase = 0.f;
    enum Shape { TRI, SAW, SQR, SH };
    Shape shape = TRI;
    float held  = 0.f;

    float process(float rate, float sr) {
        phase += rate / sr;
        if (phase >= 1.f) {
            phase -= 1.f;
            if (shape == SH)
                held = ((float)rand() / RAND_MAX) * 2.f - 1.f;
        }
        switch (shape) {
            case TRI: return (phase < 0.5f) ? (phase * 4.f - 1.f) : (3.f - phase * 4.f);
            case SAW: return phase * 2.f - 1.f;
            case SQR: return (phase < 0.5f) ? 1.f : -1.f;
            case SH:  return held;
        }
        return 0.f;
    }
};

// ═══════════════════════════════════════════════════════════
//   Module
// ═══════════════════════════════════════════════════════════

struct Poly008 : Module {

    enum ParamId {
        // DCO
        PARAM_OCT16, PARAM_OCT8, PARAM_OCT4, PARAM_OCT2,
        PARAM_WAVE, PARAM_DOUBLE,
        PARAM_DETUNE, PARAM_DRIFT, PARAM_SPREAD,
        // MIX
        PARAM_BREATH, PARAM_SAT,
        // VCF
        PARAM_CUTOFF, PARAM_RESON, PARAM_ENVF,
        // VCF ENV
        PARAM_VCFA, PARAM_VCFD, PARAM_VCFS, PARAM_VCFR,
        // VCA ENV
        PARAM_VCAA, PARAM_VCAD, PARAM_VCAS, PARAM_VCAR,
        // LFO
        PARAM_LRATE, PARAM_LSHAPE, PARAM_LDCO, PARAM_LVCF,
        PARAMS_LEN
    };

    enum InputId {
        INPUT_VOCT,    // poly kabel — max 4 kanalen
        INPUT_GATE,    // poly kabel — max 4 kanalen
        INPUT_DETCV,
        INPUT_FCUTCV,
        INPUT_RATECV,
        INPUTS_LEN
    };

    enum OutputId {
        OUTPUT_L, OUTPUT_R,
        OUTPUT_NOISEOUT,
        OUTPUT_VCFENV,
        OUTPUT_VCAENV,
        OUTPUT_LFOOUT,
        OUTPUTS_LEN
    };

    enum LightId { LIGHTS_LEN };

    static constexpr int VOICES = 4;

    DCOVoice     voice[VOICES];
    ADSR         vcfEnv[VOICES];
    ADSR         vcaEnv[VOICES];
    LadderFilter filter[VOICES];   // per-stem filter
    LFO808       lfo;
    OnePole      dcBlockL;
    OnePole      dcBlockR;
    OnePole      noiseLP;

    bool  gateHigh[VOICES]    = {};
    // Vaste detune spreiding per stem voor analoge warmte
    float stemDetune[VOICES]  = { -0.04f, -0.013f, 0.015f, 0.042f };

    Poly008() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // DCO
        configSwitch(PARAM_OCT16, 0,1,1, "16' octave", {"Off","On"});
        configSwitch(PARAM_OCT8,  0,1,1, "8' octave",  {"Off","On"});
        configSwitch(PARAM_OCT4,  0,1,0, "4' octave",  {"Off","On"});
        configSwitch(PARAM_OCT2,  0,1,0, "2' octave",  {"Off","On"});
        configSwitch(PARAM_WAVE,  0,1,0, "Waveform",   {"Saw (Walsh)","Square"});
        configSwitch(PARAM_DOUBLE,0,1,0, "Double mode",{"4 voice","2 voice"});
        configParam(PARAM_DETUNE, -1.f, 1.f, 0.05f, "Detune",  " st");  // lichte detune
        configParam(PARAM_DRIFT,   0.f, 1.f, 0.35f, "Drift");            // analoge warmte
        configParam(PARAM_SPREAD,  0.f, 1.f, 0.2f,  "Spread");           // licht stereo

        // MIX
        configParam(PARAM_BREATH, 0.f, 1.f, 0.f,   "Breath");     // geen noise
        configParam(PARAM_SAT,    0.f, 1.f, 0.15f, "Saturation"); // lichte warmte

        // VCF
        configParam(PARAM_CUTOFF, 80.f, 18000.f, 2800.f, "Cutoff", " Hz"); // min 80Hz
        configParam(PARAM_RESON,  0.f, 0.95f, 0.15f, "Resonance");        // lichte resonantie
        configParam(PARAM_ENVF,  -1.f, 1.f, 0.55f,  "Env → Filter");     // filter volgt ENV

        // VCF ENV
        configParam(PARAM_VCFA, 0.001f, 4.f, 0.005f, "VCF Attack",  " s"); // snelle attack
        configParam(PARAM_VCFD, 0.001f, 4.f, 0.5f,  "VCF Decay",   " s"); // middellange decay
        configParam(PARAM_VCFS, 0.f,    1.f, 0.3f,  "VCF Sustain");       // filter half open
        configParam(PARAM_VCFR, 0.001f, 4.f, 0.35f, "VCF Release", " s");

        // VCA ENV
        configParam(PARAM_VCAA, 0.001f, 4.f, 0.008f, "VCA Attack",  " s"); // heel kort
        configParam(PARAM_VCAD, 0.001f, 4.f, 0.25f,  "VCA Decay",   " s");
        configParam(PARAM_VCAS, 0.f,    1.f, 0.75f,  "VCA Sustain");       // sustain hoog
        configParam(PARAM_VCAR, 0.001f, 4.f, 0.4f,   "VCA Release", " s"); // iets staart

        // LFO
        configParam(PARAM_LRATE,  0.05f, 20.f, 5.f, "LFO Rate", " Hz"); // typische vibrato snelheid
        configSwitch(PARAM_LSHAPE,0,3,0, "LFO Shape",{"Triangle","Saw","Square","S&H"});
        configParam(PARAM_LDCO,   0.f, 1.f, 0.f, "LFO → DCO");
        configParam(PARAM_LVCF,   0.f, 1.f, 0.f, "LFO → VCF");

        // Inputs / Outputs
        configInput(INPUT_VOCT,   "V/Oct (poly)");
        configInput(INPUT_GATE,   "Gate (poly)");
        configInput(INPUT_DETCV,  "Detune CV");
        configInput(INPUT_FCUTCV, "Filter Cutoff CV");
        configInput(INPUT_RATECV, "LFO Rate CV");

        configOutput(OUTPUT_L,       "Audio L");
        configOutput(OUTPUT_R,       "Audio R");
        configOutput(OUTPUT_NOISEOUT,"Noise out");
        configOutput(OUTPUT_VCFENV,  "VCF Env out");
        configOutput(OUTPUT_VCAENV,  "VCA Env out");
        configOutput(OUTPUT_LFOOUT,  "LFO out");
    }

    void process(const ProcessArgs& args) override {
        const float sr = args.sampleRate;

        // ── Parameters lezen ─────────────────────────────────
        bool oct16   = params[PARAM_OCT16].getValue() > 0.5f;
        bool oct8    = params[PARAM_OCT8 ].getValue() > 0.5f;
        bool oct4    = params[PARAM_OCT4 ].getValue() > 0.5f;
        bool oct2    = params[PARAM_OCT2 ].getValue() > 0.5f;
        bool sawMode = params[PARAM_WAVE  ].getValue() < 0.5f;
        bool dblMode = params[PARAM_DOUBLE].getValue() > 0.5f;

        float detune  = params[PARAM_DETUNE].getValue()
                      + inputs[INPUT_DETCV].getVoltage() * 0.1f;
        float driftAmt= params[PARAM_DRIFT ].getValue();
        float spread  = params[PARAM_SPREAD].getValue();

        float breath  = params[PARAM_BREATH].getValue();
        float satAmt  = params[PARAM_SAT   ].getValue();

        float cutoff  = params[PARAM_CUTOFF].getValue()
                      + inputs[INPUT_FCUTCV].getVoltage() * 1000.f;
        cutoff = clamp(cutoff, 80.f, sr * 0.45f);
        float resonance = params[PARAM_RESON].getValue();
        float envFAmt   = params[PARAM_ENVF ].getValue();

        // LFO
        lfo.shape = (LFO808::Shape)(int)params[PARAM_LSHAPE].getValue();
        float lfoRate = clamp(params[PARAM_LRATE].getValue()
                            + inputs[INPUT_RATECV].getVoltage(), 0.01f, 40.f);
        float lfoVal  = lfo.process(lfoRate, sr);
        float lfoToDCO = params[PARAM_LDCO].getValue() * lfoVal;
        float lfoToVCF = params[PARAM_LVCF].getValue() * lfoVal;

        // ── Poly kabel: aantal actieve kanalen ───────────────
        // Max 4 stemmen, ook als kabel minder kanalen heeft
        int chVoct = inputs[INPUT_VOCT].getChannels();
        int chGate = inputs[INPUT_GATE].getChannels();
        int numVoices = clamp(std::max(chVoct, chGate), 1, VOICES);

        // ── ENV params (gedeeld, sturen alle stemmen) ────────
        float vcfA = params[PARAM_VCFA].getValue();
        float vcfD = params[PARAM_VCFD].getValue();
        float vcfS = params[PARAM_VCFS].getValue();
        float vcfR = params[PARAM_VCFR].getValue();
        float vcaA = params[PARAM_VCAA].getValue();
        float vcaD = params[PARAM_VCAD].getValue();
        float vcaS = params[PARAM_VCAS].getValue();
        float vcaR = params[PARAM_VCAR].getValue();

        // ── Per-stem processing ──────────────────────────────
        float outL = 0.f, outR = 0.f;
        float mixedVCFenv = 0.f, mixedVCAenv = 0.f;

        // Ruis (eenmalig berekend, niet per stem)
        float noiseRaw  = ((float)rand() / RAND_MAX) * 2.f - 1.f;
        float pinkNoise = noiseLP.process(noiseRaw, 0.98f);
        // Mix wit en roze ruis op basis van NOISE knob (toekomstig gebruik)
        float noiseOut  = noiseRaw * 0.5f + pinkNoise * 0.5f;

        for (int v = 0; v < VOICES; v++) {

            // Gate — lees poly kanaal, of 0 als niet verbonden
            float gateV = (v < chGate) ? inputs[INPUT_GATE].getVoltage(v) : 0.f;
            bool  gate  = gateV > 0.5f;

            if (gate != gateHigh[v]) {
                gateHigh[v] = gate;
                vcfEnv[v].gate(gate);
                vcaEnv[v].gate(gate);
            }

            // Pitch — lees poly kanaal
            float voct = (v < chVoct) ? inputs[INPUT_VOCT].getVoltage(v) : 0.f;
            // lfoToDCO is 0..1 * lfoVal (-1..1) = max ±1 semitoon
            // Voor hoorbaar vibrato: schaal naar ±0.5 octaaf max
            voice[v].freq   = 261.626f * powf(2.f, voct + lfoToDCO * 0.5f);
            voice[v].active = gate || !vcaEnv[v].isIdle();

            // Double mode: stem 0+1 = één noot met detune, 2+3 = andere
            float stemDet = stemDetune[v];
            if (dblMode) {
                stemDet += (v % 2 == 0) ? -detune * 0.5f : detune * 0.5f;
            }

            // ENVs (zelfde knoppen, eigen instantie per stem)
            vcfEnv[v].setParams(vcfA, vcfD, vcfS, vcfR, sr);
            vcaEnv[v].setParams(vcaA, vcaD, vcaS, vcaR, sr);

            float fEnv = vcfEnv[v].process();
            float aEnv = vcaEnv[v].process();
            mixedVCFenv += fEnv;
            mixedVCAenv += aEnv;

            // DCO
            float osc = voice[v].process(sr, sawMode,
                                         oct16, oct8, oct4, oct2,
                                         driftAmt, stemDet);

            // Breath: noise in osc mixen
            float preFilt = osc * (1.f - breath) + noiseRaw * breath;

            // Per-stem filter (eigen cutoff met env modulatie)
            // Cutoff modulatie: env (4 octaven) + LFO (2 octaven)
            float envMod = envFAmt * fEnv * 4.f;
            float lfoMod = lfoToVCF * 2.f;
            float dynCutoff = cutoff * powf(2.f, envMod + lfoMod);
            dynCutoff = clamp(dynCutoff, 80.f, sr * 0.45f);

            float filtered = filter[v].process(preFilt, dynCutoff, resonance, sr);

            // VCA
            float voiced = filtered * aEnv;

            // Stereo spread: gebaseerd op actieve stemmen
            // Bij 1 actieve stem = midden, bij 4 = volledig gespreid
            float panPos = (numVoices > 1)
                         ? ((float)v / (numVoices - 1) - 0.5f) * 2.f * spread
                         : 0.f;
            float panL = clamp(1.f - panPos, 0.f, 1.f);
            float panR = clamp(1.f + panPos, 0.f, 1.f);

            outL += voiced * panL;
            outR += voiced * panR;
        }

        // Schaal naar aantal actieve stemmen
        float scale = (numVoices > 0) ? 1.f / numVoices : 1.f;
        outL *= scale;
        outR *= scale;
        mixedVCFenv /= VOICES;
        mixedVCAenv /= VOICES;

        // Simpele output — geen extra processing
        outL = clamp(outL, -1.f, 1.f);
        outR = clamp(outR, -1.f, 1.f);

        // Outputs (±5V)
        outputs[OUTPUT_L].setVoltage(clamp(outL * 5.f, -10.f, 10.f));
        outputs[OUTPUT_R].setVoltage(clamp(outR * 5.f, -10.f, 10.f));
        outputs[OUTPUT_NOISEOUT].setVoltage(noiseOut * 5.f);
        outputs[OUTPUT_VCFENV  ].setVoltage(mixedVCFenv * 10.f);
        outputs[OUTPUT_VCAENV  ].setVoltage(mixedVCAenv * 10.f);
        outputs[OUTPUT_LFOOUT  ].setVoltage(lfoVal * 5.f);
    }
};

// ═══════════════════════════════════════════════════════════
//   Widget
// ═══════════════════════════════════════════════════════════

struct Poly008Widget : ModuleWidget {
    Poly008Widget(Poly008* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Poly008.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2*RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2*RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // ── DCO ──────────────────────────────────────────────
        addParam(createParamCentered<CKSS>(mm2px(Vec(24, 16)), module, Poly008::PARAM_OCT16));
        addParam(createParamCentered<CKSS>(mm2px(Vec(38, 16)), module, Poly008::PARAM_OCT8));
        addParam(createParamCentered<CKSS>(mm2px(Vec(52, 16)), module, Poly008::PARAM_OCT4));
        addParam(createParamCentered<CKSS>(mm2px(Vec(66, 16)), module, Poly008::PARAM_OCT2));
        addParam(createParamCentered<CKSS>(mm2px(Vec(83, 16)), module, Poly008::PARAM_WAVE));
        addParam(createParamCentered<CKSS>(mm2px(Vec(99, 16)), module, Poly008::PARAM_DOUBLE));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(19, 30)), module, Poly008::PARAM_DETUNE));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40, 30)), module, Poly008::PARAM_DRIFT));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(61, 30)), module, Poly008::PARAM_SPREAD));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(83, 30)), module, Poly008::INPUT_DETCV));

        // ── MIX ──────────────────────────────────────────────
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(19, 48)), module, Poly008::PARAM_BREATH));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40, 48)), module, Poly008::PARAM_SAT));

        // ── VCF ──────────────────────────────────────────────
        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(19, 68)), module, Poly008::PARAM_CUTOFF));
        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(46, 68)), module, Poly008::PARAM_RESON));
        addParam(createParamCentered<RoundBlackKnob>     (mm2px(Vec(72, 66)), module, Poly008::PARAM_ENVF));
        addInput(createInputCentered<PJ301MPort>         (mm2px(Vec(95, 64)), module, Poly008::INPUT_FCUTCV));

        // ── ENV ──────────────────────────────────────────────
        // VCF ADSR
        addParam(createParamCentered<Trimpot>(mm2px(Vec(10, 84)), module, Poly008::PARAM_VCFA));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(24, 84)), module, Poly008::PARAM_VCFD));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(38, 84)), module, Poly008::PARAM_VCFS));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(52, 84)), module, Poly008::PARAM_VCFR));
        // VCA ADSR
        addParam(createParamCentered<Trimpot>(mm2px(Vec(66, 84)), module, Poly008::PARAM_VCAA));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(80, 84)), module, Poly008::PARAM_VCAD));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(94, 84)), module, Poly008::PARAM_VCAS));
        addParam(createParamCentered<Trimpot>(mm2px(Vec(108,84)), module, Poly008::PARAM_VCAR));

        // ── LFO ──────────────────────────────────────────────
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(14, 100)), module, Poly008::PARAM_LRATE));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(36, 100)), module, Poly008::PARAM_LSHAPE));
        addParam(createParamCentered<Trimpot>       (mm2px(Vec(57, 100)), module, Poly008::PARAM_LDCO));
        addParam(createParamCentered<Trimpot>       (mm2px(Vec(72, 100)), module, Poly008::PARAM_LVCF));
        addInput(createInputCentered<PJ301MPort>    (mm2px(Vec(87, 100)), module, Poly008::INPUT_RATECV));

        // ── I/O zone ─────────────────────────────────────────
        // Poly inputs (links)
        addInput(createInputCentered<PJ301MPort> (mm2px(Vec( 9, 114)), module, Poly008::INPUT_VOCT));
        addInput(createInputCentered<PJ301MPort> (mm2px(Vec(20, 114)), module, Poly008::INPUT_GATE));
        // Utility outputs
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(33, 114)), module, Poly008::OUTPUT_VCFENV));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(44, 114)), module, Poly008::OUTPUT_VCAENV));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(55, 114)), module, Poly008::OUTPUT_NOISEOUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(66, 114)), module, Poly008::OUTPUT_LFOOUT));
        // Audio out (rechts, iets groter visueel gewicht)
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(72, 114)), module, Poly008::OUTPUT_L));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(78, 114)), module, Poly008::OUTPUT_R));
    }
};

Model* modelPoly008 = createModel<Poly008, Poly008Widget>("Poly008");
