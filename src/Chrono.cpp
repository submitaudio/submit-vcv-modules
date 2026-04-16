#include "plugin.hpp"
#include <cstring>

static const int MAX_BUFFER = 48000 * 2;

struct Chrono : Module {

    enum ParamId {
        TIME_PARAM,
        FEEDBACK_PARAM,
        MIX_PARAM,
        DRIVE_PARAM,
        DIVISION_PARAM,
        SPACING_PARAM,
        SURGE_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        AUDIO_INPUT,
        CLOCK_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        AUDIO_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    // ── Delay buffer ──────────────────────────
    float buffer[MAX_BUFFER] = {};
    int   writePos    = 0;
    float filterState = 0.f;
    float phase       = 0.f;

    // ── Clock sync ────────────────────────────
    bool  lastClockHigh        = false;
    int   clockSampleCount     = 0;
    int   clockInterval        = 0;
    float smoothedDelaySamples = 0.f;
    float targetDelaySamples   = 0.f;
    float smoothedSpacing      = 0.f;
    float fbSmooth    = 0.4f;
    float driveSmooth = 0.2f;
    float toneSmooth  = 1.0f;

    // ── Divisies ──────────────────────────────
    static const int NUM_DIVISIONS = 5;
    const float divisions[NUM_DIVISIONS] = {0.25f, 0.5f, 1.0f, 2.0f, 4.0f};

    Chrono() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(TIME_PARAM,     0.f, 1.f, 0.3f, "Time",     " s", 0.f, 2.f);
        configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.4f, "Feedback", "%",  0.f, 100.f);
        configParam(MIX_PARAM,      0.f, 1.f, 0.5f, "Mix",      "%",  0.f, 100.f);
        configParam(DRIVE_PARAM,    0.f, 1.f, 0.2f, "Drive",    "%",  0.f, 100.f);

        // Division: 5 stappen (0=1/4, 1=1/2, 2=1x, 3=2x, 4=4x)
        configParam(DIVISION_PARAM, 0.f, 4.f, 2.f, "Division");
        paramQuantities[DIVISION_PARAM]->snapEnabled = true;
        configParam(SPACING_PARAM,  0.f, 4.f, 0.f, "Spacing");
        paramQuantities[SPACING_PARAM]->snapEnabled = true;
        configParam(SURGE_PARAM, 0.f, 1.f, 0.f, "Surge");

        configInput(AUDIO_INPUT,  "Audio");
        configInput(CLOCK_INPUT,  "Clock");
        configOutput(AUDIO_OUTPUT, "Audio");
        std::memset(buffer, 0, sizeof(buffer));
    }

    void process(const ProcessArgs& args) override {

        float timeKnob = params[TIME_PARAM].getValue();
        float mix      = params[MIX_PARAM].getValue();

        // ── Basis parameterwaarden ─────────────
        float feedback = params[FEEDBACK_PARAM].getValue() * 0.95f;
        float drive    = params[DRIVE_PARAM].getValue();
        float tone     = 1.0f; // toekomstige tone knob placeholder

        // ── SURGE — momentary override ─────────
        bool surgePressed = params[SURGE_PARAM].getValue() > 0.5f;

        float fbUsed    = surgePressed ? 0.99f              : feedback;
        float driveUsed = surgePressed ? drive + 0.15f      : drive;
        float toneUsed  = surgePressed ? tone  * 0.85f      : tone;

        // Smoothing — geen clicks bij indrukken/loslaten
        fbSmooth    += (fbUsed    - fbSmooth)    * 0.02f;
        driveSmooth += (driveUsed - driveSmooth) * 0.02f;
        toneSmooth  += (toneUsed  - toneSmooth)  * 0.02f;

        // Veiligheidsclamp — nooit instabiel
        fbSmooth = clamp(fbSmooth, 0.f, 0.999f);

        // ── DIVISION ──────────────────────────
        int divIndex = clamp((int)roundf(params[DIVISION_PARAM].getValue()), 0, NUM_DIVISIONS - 1);
        float ratio  = divisions[divIndex];

        // ── CLOCK / FREE MODE ─────────────────
        bool clockConnected = inputs[CLOCK_INPUT].isConnected();

        if (clockConnected) {
            // Rising edge detectie
            bool clockHigh = inputs[CLOCK_INPUT].getVoltage() > 1.0f;
            if (clockHigh && !lastClockHigh) {
                if (clockSampleCount > 0) {
                    int minSamples = (int)(0.1f * args.sampleRate);
                    if (clockSampleCount >= minSamples && clockSampleCount <= MAX_BUFFER - 1) {
                        clockInterval = clockSampleCount;
                    }
                }
                clockSampleCount = 0;
            }
            lastClockHigh = clockHigh;
            clockSampleCount++;

            // Target = interval × divisie — TIME knob volledig genegeerd
            if (clockInterval > 0) {
                targetDelaySamples = (float)clockInterval * ratio;
            }

        } else {
            // Vrije modus — alleen TIME knob, divisie niet actief
            targetDelaySamples = timeKnob * 2.f * args.sampleRate;
        }

        targetDelaySamples = clamp(targetDelaySamples, 1.f, (float)(MAX_BUFFER - 1));

        // Smoothing — geen clicks
        smoothedDelaySamples += (targetDelaySamples - smoothedDelaySamples) * 0.01f;
        float delaySamples = clamp(smoothedDelaySamples, 1.f, (float)(MAX_BUFFER - 1));

        // ── DELAY BUFFER ──────────────────────
        // Stepped spacing tabel — 5 vaste posities
        static const float spacingTable[5] = {
            0.0f,   // OFF  — enkel hoofd
            0.1f,   // Tight
            0.25f,  // Medium
            0.4f,   // Wide
            0.6f    // Extreme
        };
        int spacingIndex  = clamp((int)roundf(params[SPACING_PARAM].getValue()), 0, 4);
        float spacingFactor = spacingTable[spacingIndex];
        float spacingTarget = delaySamples * spacingFactor;

        // Smoothing — geen clicks bij stapwisseling
        smoothedSpacing += (spacingTarget - smoothedSpacing) * 0.01f;

        // Helper: lees buffer op offset met wrap
        auto readBuf = [&](float offset) -> float {
            int pos = writePos - (int)clamp(offset, 1.f, (float)(MAX_BUFFER - 1));
            if (pos < 0) pos += MAX_BUFFER;
            return buffer[pos];
        };

        float dry = inputs[AUDIO_INPUT].getVoltage();
        float s1  = readBuf(delaySamples);

        float delayed;
        if (spacingFactor == 0.0f) {
            // OFF — alleen hoofd, schoon enkelvoudig delay
            delayed = s1;
        } else {
            // Drie heads — hoofd 50%, flanken elk 25%
            // Clamp zodat head3 nooit onder minimum valt
            float sp   = clamp(smoothedSpacing, 0.f, delaySamples - 1.f);
            float s2   = readBuf(clamp(delaySamples + sp, 1.f, (float)(MAX_BUFFER - 1)));
            float s3   = readBuf(clamp(delaySamples - sp, 1.f, (float)(MAX_BUFFER - 1)));
            delayed    = s1 * 0.5f + s2 * 0.25f + s3 * 0.25f;
        }

        // ── FEEDBACK PATH ─────────────────────
        // toneSmooth beïnvloedt alpha — donkerder bij surge
        const float alpha = 0.3f * toneSmooth;
        filterState += alpha * (delayed - filterState);
        float toned = filterState;

        float variation = 1.0f
            + 0.015f * sinf(phase)
            + 0.010f * sinf(phase * 0.37f)
            + 0.005f * sinf(phase * 1.73f);
        phase += 0.01f;

        // driveSmooth — iets meer saturatie bij surge
        float dynamicDrive = 1.0f + driveSmooth * 2.5f + powf(fabsf(toned), 1.5f);
        float driven = tanhf(toned * dynamicDrive * variation);

        driven += 0.05f * sinf(driven * 2.5f)
                + 0.03f * sinf(driven * 4.5f);

        // fbSmooth — near-infinite bij surge, normaal anders
        float fb = driven * fbSmooth;
        fb *= 0.98f + 0.02f * sinf(phase * 0.23f);

        buffer[writePos] = dry + fb;
        writePos++;
        if (writePos >= MAX_BUFFER)
            writePos = 0;

        // ── MIX ───────────────────────────────
        float out = dry * (1.f - mix) + delayed * mix;
        outputs[AUDIO_OUTPUT].setVoltage(out);
    }
};

struct ChronoWidget : ModuleWidget {
    ChronoWidget(Chrono* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Chrono.svg")));

        // TIME — groot, bovenin
        addParam(createParamCentered<RoundBigBlackKnob>(
            mm2px(Vec(40.64f, 25.f)), module, Chrono::TIME_PARAM));

        // FEEDBACK + MIX
        addParam(createParamCentered<RoundBlackKnob>(
            mm2px(Vec(20.f, 55.f)), module, Chrono::FEEDBACK_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(
            mm2px(Vec(61.f, 55.f)), module, Chrono::MIX_PARAM));

        // DRIVE
        addParam(createParamCentered<RoundBlackKnob>(
            mm2px(Vec(20.f, 75.f)), module, Chrono::DRIVE_PARAM));

        // DIVISION — gesnapped schakelaar gevoel
        addParam(createParamCentered<RoundBlackKnob>(
            mm2px(Vec(61.f, 75.f)), module, Chrono::DIVISION_PARAM));

        // SPACING — tape head spreiding
        addParam(createParamCentered<RoundBlackKnob>(
            mm2px(Vec(40.64f, 75.f)), module, Chrono::SPACING_PARAM));

        // Poorten
        // SURGE — momentary button
        addParam(createParamCentered<CKD6>(
            mm2px(Vec(40.64f, 108.f)), module, Chrono::SURGE_PARAM));

        addInput(createInputCentered<PJ301MPort>(
            mm2px(Vec(15.f, 95.f)), module, Chrono::AUDIO_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            mm2px(Vec(40.64f, 95.f)), module, Chrono::CLOCK_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(
            mm2px(Vec(66.f, 95.f)), module, Chrono::AUDIO_OUTPUT));
    }
};

Model* modelChrono = createModel<Chrono, ChronoWidget>("Chrono");
