// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// Licensed under GPL v3 — see LICENSE file for details
// https://github.com/submitaudio/submit-vcv-modules

#include "plugin.hpp"
#include <cmath>
#include <cstring>

static const int MAX_BUFFER = 96000 * 4;  // 4 sec @ 96kHz max

static inline float wrapBufferPos(float pos) {
    pos = std::fmod(pos, (float)MAX_BUFFER);
    if (pos < 0.f)
        pos += (float)MAX_BUFFER;
    return pos;
}

// ─────────────────────────────────────────────
//  CUSTOM WIDGETS
// ─────────────────────────────────────────────

struct Drift13KnobLarge : SvgKnob {
    Drift13KnobLarge() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/Drift13KnobLarge.svg")));
        shadow->opacity = 0.f;
    }
};

struct Drift13KnobSmall : SvgKnob {
    Drift13KnobSmall() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobSmall.svg")));
        shadow->opacity = 0.f;
    }
};

struct Drift13KnobMedium : SvgKnob {
    Drift13KnobMedium() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobMedium.svg")));
        shadow->opacity = 0.f;
    }
};

struct ChronoSlider : SvgSlider {
    ChronoSlider() {
        setHandleSvg(Svg::load(asset::plugin(pluginInstance, "res/ChronoSliderHandle.svg")));
        setHandlePosCentered(
            math::Vec(6.76f, 98.187f),
            math::Vec(6.76f, 0.f)
        );
        horizontal = false;
        box.size = math::Vec(13.52f, 98.187f);
        // Handle hitbox groter maken — meer grijpruimte boven en onder
        handle->box.size.y = 60.f;
    }
};

struct ChronoSurgeButton : SvgSwitch {
    ChronoSurgeButton() {
        momentary = true;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChronoSurge_0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChronoSurge_1.svg")));
        shadow->opacity = 0.f;
    }
};

struct ChronoBreakButton : SvgSwitch {
    ChronoBreakButton() {
        momentary = true;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChronoBreak_0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChronoBreak_1.svg")));
        shadow->opacity = 0.f;
    }
};

// ─────────────────────────────────────────────
//  MODULE
// ─────────────────────────────────────────────

struct Chrono : Module {

    enum ParamId {
        TIME_PARAM,
        FEEDBACK_PARAM,
        MIX_PARAM,
        DRIVE_PARAM,
        TAPE_PARAM,
        HEADS_PARAM,
        DIVISION_PARAM,
        SPACING_PARAM,
        SPREAD_PARAM,
        SURGE_PARAM,
        BREAK_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        AUDIO_L_INPUT,
        AUDIO_R_INPUT,
        TIME_CV_INPUT,
        FEEDBACK_CV_INPUT,
        MIX_CV_INPUT,
        DRIVE_CV_INPUT,
        TAPE_CV_INPUT,
        HEADS_CV_INPUT,
        CLOCK_INPUT,
        SPACING_CV_INPUT,
        SPREAD_CV_INPUT,
        SURGE_CV_INPUT,
        BREAK_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        AUDIO_L_OUTPUT,
        AUDIO_R_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        SURGE_LIGHT,
        BREAK_LIGHT,
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
    int   clockPpqn            = 1;
    float smoothedDelaySamples = 0.f;
    float targetDelaySamples   = 0.f;
    float smoothedSpacing      = 0.f;

    // ── Surge + Break smooth ──────────────────
    float fbSmooth    = 0.4f;
    float driveSmooth = 0.2f;
    float toneSmooth  = 1.0f;

    // ── Freeze state ──────────────────────────
    float freezeGain    = 0.f;
    float freezeHpState = 0.f;

    // ── Brake state ───────────────────────────
    float brakeSpeed       = 1.f;
    float brakeSpeedSmooth = 1.f;

    // Momentary smooth bypass: 0 = Chrono mix, 1 = direct dry signal.
    float dryFade = 0.f;

    // New modules use Time as an extra clocked ratio. Legacy patches keep the
    // original behavior until this is enabled from the context menu.
    bool clockedTimeEnabled = true;

    // Drijvende lees posities per head
    float headReadPos[3] = {0.f, 0.f, 0.f};
    bool  headPosInit    = false;

    // ── Dry brake buffer ──────────────────────
    static const int DRY_BUFFER = 48000; // 1 sec buffer
    float dryBuffer[48000] = {};
    int   dryWritePos = 0;
    float dryReadPos  = 0.f;

    // ── Print-through state ───────────────────
    float printThroughState = 0.f;

    // ── Stereo spread state ───────────────────
    float spreadSmooth = 0.f;  // Smoothed print-through signal

    // ── Tape state ────────────────────────────
    float tapeFilterState = 0.f;
    float wowPhase        = 0.f;
    float flutterPhase    = 0.f;
    float wowRate         = 0.f;
    float flutterRate     = 0.f;
    float hissState       = 0.f;
    float dropoutGain     = 1.f;
    float dropoutTarget   = 1.f;
    int   dropoutTimer    = 0;
    uint32_t rngState     = 12345;

    // ── Divisies ──────────────────────────────
    static const int NUM_DIVISIONS = 5;
    const float divisions[NUM_DIVISIONS] = {0.25f, 0.5f, 1.0f, 2.0f, 4.0f};

    // ── Head combinaties ──────────────────────
    // [positie][head] = volume factor
    // 3 heads: triplet(1/3), dotted(1/2), quarter(1/1)
    const float headMix[6][3] = {
        {1.0f, 1.0f, 1.0f},  // 0: ALL  — gelijk/gelijk/gelijk
        {1.0f, 0.5f, 0.5f},  // 1: TRP  — hoog/laag/laag
        {0.5f, 1.0f, 0.5f},  // 2: DOT  — laag/hoog/laag
        {0.5f, 0.5f, 1.0f},  // 3: QTR  — laag/laag/hoog
        {1.0f, 1.0f, 0.5f},  // 4: DUB  — hoog/hoog/laag
        {0.5f, 1.0f, 1.0f},  // 5: SUB  — laag/hoog/hoog
    };

    Chrono() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(TIME_PARAM,     0.f, 1.f, 0.3f, "Time");
        configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.5f, "Feedback");
        configParam(MIX_PARAM,      0.f, 1.f, 0.318f, "Mix");
        configParam(DRIVE_PARAM,    0.f, 1.f, 0.128f, "Drive");
        configParam(TAPE_PARAM,     0.f, 1.f, 0.324f, "Tape");
        configSwitch(HEADS_PARAM, 0.f, 5.f, 0.f, "Heads", {"SUB", "DUB", "QTR", "DOT", "TRP", "ALL"});
        paramQuantities[HEADS_PARAM]->snapEnabled = true;
        configParam(DIVISION_PARAM, 0.f, 4.f, 2.f,  "Division");
        paramQuantities[DIVISION_PARAM]->snapEnabled = true;
        configParam(SPACING_PARAM,  0.f, 4.f, 3.f,  "Offset");
        paramQuantities[SPACING_PARAM]->snapEnabled = true;
        configParam(SPREAD_PARAM,   0.f, 1.f, 1.f,  "Spread");
        configParam(SURGE_PARAM,    0.f, 1.f, 0.f,  "Surge");
        configParam(BREAK_PARAM,    0.f, 1.f, 0.f,  "Dry");

        configInput(AUDIO_L_INPUT,    "Audio In L");
        configInput(AUDIO_R_INPUT,    "Audio In R");
        configInput(TIME_CV_INPUT,    "Time CV");
        configInput(FEEDBACK_CV_INPUT,"Feedback CV");
        configInput(MIX_CV_INPUT,     "Mix CV");
        configInput(DRIVE_CV_INPUT,   "Drive CV");
        configInput(TAPE_CV_INPUT,    "Tape CV");
        configInput(HEADS_CV_INPUT,   "Heads CV");
        configInput(CLOCK_INPUT,      "Clock");
        configInput(SPACING_CV_INPUT, "Offset CV");
        configInput(SPREAD_CV_INPUT,  "Spread CV");
        configInput(SURGE_CV_INPUT,   "Surge Gate");
        configInput(BREAK_CV_INPUT,   "Dry Gate");

        configOutput(AUDIO_L_OUTPUT, "Audio Out L");
        configOutput(AUDIO_R_OUTPUT, "Audio Out R");

        std::memset(buffer, 0, sizeof(buffer));
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "clockedTimeVersion", json_integer(1));
        json_object_set_new(rootJ, "clockedTimeEnabled", json_boolean(clockedTimeEnabled));
        json_object_set_new(rootJ, "clockPpqn", json_integer(clockPpqn));
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        if (json_t* enabledJ = json_object_get(rootJ, "clockedTimeEnabled"))
            clockedTimeEnabled = json_boolean_value(enabledJ);
        if (json_t* ppqnJ = json_object_get(rootJ, "clockPpqn"))
            clockPpqn = json_integer_value(ppqnJ) == 4 ? 4 : 1;
    }

    void fromJson(json_t* rootJ) override {
        json_t* dataJ = json_object_get(rootJ, "data");
        if (!dataJ || !json_object_get(dataJ, "clockedTimeVersion"))
            clockedTimeEnabled = false;
        Module::fromJson(rootJ);
    }

    void setClockPpqn(int ppqn) {
        const int selected = ppqn == 4 ? 4 : 1;
        if (clockPpqn == selected)
            return;
        clockPpqn = selected;
        lastClockHigh = false;
        clockSampleCount = 0;
        clockInterval = 0;
    }

    void process(const ProcessArgs& args) override {

        // ── CV inputs koppelen aan parameters ─
        auto cvAdd = [&](int paramId, int inputId, float scale = 0.1f) -> float {
            float val = params[paramId].getValue();
            if (inputs[inputId].isConnected())
                val += inputs[inputId].getVoltage() * scale;
            return clamp(val, 0.f, 1.f);
        };

        float timeKnob = cvAdd(TIME_PARAM,     TIME_CV_INPUT);
        float mix      = cvAdd(MIX_PARAM,      MIX_CV_INPUT);
        float drive    = cvAdd(DRIVE_PARAM,    DRIVE_CV_INPUT);
        float tone     = 1.0f;

        // ── FEEDBACK curve ────────────────────
        float fbRaw = clamp(params[FEEDBACK_PARAM].getValue()
            + (inputs[FEEDBACK_CV_INPUT].isConnected()
                ? inputs[FEEDBACK_CV_INPUT].getVoltage() * 0.1f : 0.f), 0.f, 1.f);
        // Subtle perceptual lift in the middle while preserving both endpoints.
        float feedbackCurve = 1.f + 0.12f * (4.f * fbRaw * (1.f - fbRaw));
        float feedback = clamp(fbRaw * feedbackCurve * 0.98f, 0.f, 0.999f);

        // ── SURGE — FREEZE ────────────────────
        bool surgePressed = params[SURGE_PARAM].getValue() > 0.5f
            || (inputs[SURGE_CV_INPUT].isConnected() && inputs[SURGE_CV_INPUT].getVoltage() > 1.f);

        float freezeTarget = surgePressed ? 1.f : 0.f;
        if (surgePressed) {
            float fadeInSpeed = feedback < 0.5f ? 0.001f + (feedback / 0.5f) * 0.004f : 0.005f;
            freezeGain += (freezeTarget - freezeGain) * fadeInSpeed;
        } else {
            // Exact linear release: a fully developed Surge reaches zero in 3 s.
            freezeGain = fmaxf(0.f, freezeGain - 1.f / (3.f * args.sampleRate));
        }

        // Let Surge feedback and drive follow the same three-second release.
        float surgeFeedback = fmaxf(feedback + 0.08f, 0.90f);
        float fbUsed = feedback + (surgeFeedback - feedback) * freezeGain;
        fbUsed = clamp(fbUsed, 0.f, 0.990f);
        float driveUsed = drive + 0.30f * freezeGain;

        fbSmooth    += (fbUsed    - fbSmooth)    * 0.008f;
        driveSmooth += (driveUsed - driveSmooth) * 0.008f;
        toneSmooth  += (tone      - toneSmooth)  * 0.008f;
        fbSmooth    = clamp(fbSmooth,    0.f, 0.999f);
        driveSmooth = clamp(driveSmooth, 0.f, 2.f);
        toneSmooth  = clamp(toneSmooth,  0.f, 1.f);

        // ── DRY — smooth momentary bypass ─────
        bool dryPressed = params[BREAK_PARAM].getValue() > 0.5f
            || (inputs[BREAK_CV_INPUT].isConnected() && inputs[BREAK_CV_INPUT].getVoltage() > 1.f);

        const float dryFadeInStep  = 1.f / (1.f * args.sampleRate);
        const float dryFadeOutStep = 1.f / (1.f * args.sampleRate);
        if (dryPressed)
            dryFade = fminf(1.f, dryFade + dryFadeInStep);
        else
            dryFade = fmaxf(0.f, dryFade - dryFadeOutStep);

        // ── LEDs ──────────────────────────────
        lights[SURGE_LIGHT].setBrightness(surgePressed ? 1.f : 0.f);
        lights[BREAK_LIGHT].setBrightness(dryPressed ? 1.f : 0.f);

        // ── DIVISION ──────────────────────────
        int divIndex = clamp((int)roundf(params[DIVISION_PARAM].getValue()), 0, NUM_DIVISIONS - 1);
        float ratio  = divisions[divIndex];

        // ── CLOCK / FREE MODE ─────────────────
        bool clockConnected = inputs[CLOCK_INPUT].isConnected();
        bool clockHigh = inputs[CLOCK_INPUT].getVoltage() > 1.0f;

        if (clockConnected) {
            if (clockHigh && !lastClockHigh) {
                if (clockSampleCount > 0) {
                    const int quarterSamples = clockSampleCount * clockPpqn;
                    int minSamples = (int)(0.1f * args.sampleRate);
                    if (quarterSamples >= minSamples && quarterSamples <= MAX_BUFFER - 1) {
                        clockInterval = quarterSamples;
                    }
                }
                clockSampleCount = 0;
            }
            lastClockHigh = clockHigh;
            clockSampleCount++;
            if (clockInterval > 0) {
                float clockedTimeRatio = 1.f;
                if (clockedTimeEnabled) {
                    // Musical ratios with the existing 0.3 default as 1x.
                    // Counter-clockwise adds shorter delays; clockwise retains
                    // useful dotted/extended clock relationships.
                    static const float positions[9] = {
                        0.f, 0.075f, 0.15f, 0.225f, 0.30f,
                        0.475f, 0.65f, 0.825f, 1.f
                    };
                    static const float ratios[9] = {
                        0.25f, 1.f / 3.f, 0.5f, 2.f / 3.f, 1.f,
                        4.f / 3.f, 1.5f, 2.f, 4.f
                    };
                    int nearest = 0;
                    float nearestDistance = fabsf(timeKnob - positions[0]);
                    for (int i = 1; i < 9; ++i) {
                        float distance = fabsf(timeKnob - positions[i]);
                        if (distance < nearestDistance) {
                            nearest = i;
                            nearestDistance = distance;
                        }
                    }
                    clockedTimeRatio = ratios[nearest];
                }
                targetDelaySamples = (float)clockInterval * ratio * clockedTimeRatio;
            }
        } else {
            // Reset clock state als kabel losgehaald wordt
            clockInterval = 0;
            clockSampleCount = 0;
            targetDelaySamples = timeKnob * 2.f * args.sampleRate;
        }

        targetDelaySamples = clamp(targetDelaySamples, 1.f, (float)(MAX_BUFFER - 1));
        smoothedDelaySamples += (targetDelaySamples - smoothedDelaySamples) * 0.01f;
        // Bij brake: delay tijd wordt geleidelijk langer → pitch daalt
        float brakeMultiplier = 1.f + (1.f - brakeSpeedSmooth) * 4.f;
        float delaySamples = clamp(smoothedDelaySamples * brakeMultiplier, 1.f, (float)(MAX_BUFFER - 1));

        // ── HEADS ─────────────────────────────
        float headsRaw = params[HEADS_PARAM].getValue()
            + (inputs[HEADS_CV_INPUT].isConnected()
                ? inputs[HEADS_CV_INPUT].getVoltage() * 0.5f : 0.f);
        int headIndex = clamp((int)roundf(headsRaw), 0, 5);

        // Head tijden relatief aan delay tijd
        // Triplet = 2/3, Dotted = 3/2, Quarter = 1x
        float headTime[3];
        if (clockConnected && clockInterval > 0) {
            headTime[0] = delaySamples * (2.f / 3.f);  // Triplet
            headTime[1] = delaySamples * 1.0f;          // Beat
            headTime[2] = delaySamples * (3.f / 2.f);  // Dotted
        } else {
            headTime[0] = delaySamples * (2.f / 3.f);
            headTime[1] = delaySamples * 1.0f;
            headTime[2] = delaySamples * (3.f / 2.f);
        }

        // ── SPACING ───────────────────────────
        static const float spacingTable[5] = {0.0f, 0.15f, 0.35f, 0.6f, 0.9f};
        float spacingRaw = params[SPACING_PARAM].getValue()
            + (inputs[SPACING_CV_INPUT].isConnected()
                ? inputs[SPACING_CV_INPUT].getVoltage() * 0.4f : 0.f);
        int spacingIndex    = clamp((int)roundf(spacingRaw), 0, 4);
        float spacingFactor = spacingTable[spacingIndex];
        float spacingTarget = delaySamples * spacingFactor;
        smoothedSpacing += (spacingTarget - smoothedSpacing) * 0.01f;

        // Pas spacing toe op heads — head 0 dichterbij, head 2 verder weg
        headTime[0] = clamp(headTime[0] - smoothedSpacing, 1.f, (float)(MAX_BUFFER - 1));
        headTime[1] = clamp(headTime[1], 1.f, (float)(MAX_BUFFER - 1));
        headTime[2] = clamp(headTime[2] + smoothedSpacing, 1.f, (float)(MAX_BUFFER - 1));

        // ── DELAY BUFFER LEZEN ────────────────


        float dryRaw = inputs[AUDIO_L_INPUT].getVoltage();
        float dryRawR = inputs[AUDIO_R_INPUT].isConnected()
            ? inputs[AUDIO_R_INPUT].getVoltage() : dryRaw;

        // Dry direct — perfecte L/R sync
        float dry = dryRaw;

        // ── TAPE STOP — drijvende lees posities ─
        // Init lees posities op eerste run
        if (!headPosInit) {
            for (int i = 0; i < 3; i++) {
                headReadPos[i] = wrapBufferPos((float)writePos - headTime[i]);
            }
            headPosInit = true;
        }

        auto readHead = [&](int idx, float offset) -> float {
            // Target: waar de head normaal zou staan
            offset = clamp(offset, 1.f, (float)MAX_BUFFER - 2.f);
            float target = wrapBufferPos((float)writePos - offset);

            // Schuif lees positie op met brakeSpeedSmooth
            // 1.0 = normaal, 0.0 = stilstand
            headReadPos[idx] += brakeSpeedSmooth;
            headReadPos[idx] = wrapBufferPos(headReadPos[idx]);

            // Bij normale speed: sync naar target als te ver af
            if (brakeSpeedSmooth > 0.95f) {
                float diff = target - headReadPos[idx];
                if (diff > (float)MAX_BUFFER * 0.5f)  diff -= (float)MAX_BUFFER;
                if (diff < -(float)MAX_BUFFER * 0.5f) diff += (float)MAX_BUFFER;
                if (fabsf(diff) > 100.f) {
                    headReadPos[idx] = target;
                }
            }

            // Lineaire interpolatie
            float fpos = headReadPos[idx];
            int   pos0 = ((int)fpos) % MAX_BUFFER;
            int   pos1 = (pos0 + 1) % MAX_BUFFER;
            float frac = fpos - floorf(fpos);
            return buffer[pos0] * (1.f - frac) + buffer[pos1] * frac;
        };

        float h0 = readHead(0, headTime[0]);
        float h1 = readHead(1, headTime[1]);
        float h2 = readHead(2, headTime[2]);

        // Mix heads volgens geselecteerde combinatie
        float w0 = headMix[headIndex][0];
        float w1 = headMix[headIndex][1];
        float w2 = headMix[headIndex][2];
        float wTotal = w0 + w1 + w2;
        float delayed = (h0 * w0 + h1 * w1 + h2 * w2) / wTotal;

        // ── FEEDBACK PATH ─────────────────────
        const float alpha = 0.1f + toneSmooth * 0.5f;
        filterState += alpha * (delayed - filterState);
        float toned = filterState;

        float variation = 1.0f
            + 0.015f * sinf(phase)
            + 0.010f * sinf(phase * 0.37f)
            + 0.005f * sinf(phase * 1.73f);
        phase += 0.01f;

        float dynamicDrive = 1.0f + driveSmooth * 2.5f + powf(fabsf(toned), 1.5f);
        float driven = tanhf(toned * dynamicDrive * variation);

        driven += 0.05f * sinf(driven * 2.5f)
                + 0.03f * sinf(driven * 4.5f);

        // SURGE low-end control
        if (surgePressed) {
            static float surgeBassTight = 0.f;
            surgeBassTight += 0.08f * (driven - surgeBassTight);
            driven = driven - surgeBassTight * 0.35f;
        }

        float fb = driven * fbSmooth;
        fb *= 0.98f + 0.02f * sinf(phase * 0.23f);

        // ── FREEZE buffer write ────────────────
        freezeHpState += 0.05f * (fb - freezeHpState);

        // Altijd dry + feedback — surge bevriest bovenop
        buffer[writePos] = dry + fb;
        writePos++;
        if (writePos >= MAX_BUFFER)
            writePos = 0;

        // ── TAPE PROCESSING ───────────────────
        float tape = clamp(params[TAPE_PARAM].getValue()
            + (inputs[TAPE_CV_INPUT].isConnected()
                ? inputs[TAPE_CV_INPUT].getVoltage() * 0.1f : 0.f), 0.f, 1.f);

        // Twee fasen
        float damageStage = clamp((tape - 0.5f) / 0.5f, 0.f, 1.f); // 50→100%: wobble+damage

        // RNG
        rngState = rngState * 1664525u + 1013904223u;
        float rnd = (float)(rngState >> 16) / 65535.f;
        rngState = rngState * 1664525u + 1013904223u;
        float rnd2 = (float)(rngState >> 16) / 65535.f;

        // ── LAAG 1: TAPE HISS (0→50%) ─────────
        // Hoog frequent — tape hiss karakter
        rngState = rngState * 1664525u + 1013904223u;
        float rawNoise = ((float)(rngState >> 16) / 65535.f) * 2.f - 1.f;
        // Hoogpass — alleen hoge frequenties
        hissState += 0.8f * (rawNoise - hissState);
        float hissHigh = rawNoise - hissState;
        // Hiss bouwt op tot 50% en blijft daarna gelijk

        // ── LAAG 2: WOBBLE (50→100%, clock sync) ─
        if (clockConnected && clockInterval > 0) {
            float clockHz = args.sampleRate / (float)clockInterval;
            wowRate     = clockHz * 0.12f + rnd * clockHz * 0.04f;
            flutterRate = clockHz * 0.7f  + rnd2 * clockHz * 0.2f;
        } else {
            wowRate     = 0.25f + rnd * 0.15f;
            flutterRate = 5.5f  + rnd2 * 2.5f;
        }
        wowPhase     += wowRate     / args.sampleRate;
        flutterPhase += flutterRate / args.sampleRate;
        if (wowPhase     > 1.f) wowPhase     -= 1.f;
        if (flutterPhase > 1.f) flutterPhase -= 1.f;

        // Wobble alleen boven 50%, niet tijdens freeze
        float freezeMute   = 1.f - freezeGain;
        float wowDepth     = damageStage * 0.030f * freezeMute;
        float flutterDepth = damageStage * 0.012f * freezeMute;
        float wobble = wowDepth     * sinf(wowPhase     * 2.f * M_PI)
                     + flutterDepth * sinf(flutterPhase * 2.f * M_PI);
        float tapeSamples = clamp(delaySamples * (1.f + wobble), 1.f, (float)(MAX_BUFFER - 1));

        int tapeReadPos = writePos - (int)tapeSamples;
        if (tapeReadPos < 0) tapeReadPos += MAX_BUFFER;
        float tapeSignal = buffer[tapeReadPos];

        // Lowpass — donkerder bij hogere tape
        float tapeAlpha = 0.04f + (1.f - tape) * 0.36f;
        tapeFilterState += tapeAlpha * (tapeSignal - tapeFilterState);

        // Saturatie
        // tapeCurve lineair — geen vroege piek in het midden
        // Lagere drive — minder schelle vervorming in het midden

        // ── LAAG 3: DROPOUTS (50→100%) ────────
        // Korte volume dips — versleten band gevoel
        dropoutTimer--;
        if (dropoutTimer <= 0 && damageStage > 0.f) {
            rngState = rngState * 1664525u + 1013904223u;
            float roll = (float)(rngState >> 16) / 65535.f;
            if (roll < damageStage * 0.012f) {
                rngState = rngState * 1664525u + 1013904223u;
                float dur = (float)(rngState >> 16) / 65535.f;
                // Dropout duur: 10-60ms
                dropoutTimer  = (int)((0.01f + dur * 0.05f) * args.sampleRate);
                // Diepte schaalt met damage — nooit volledig stil
                dropoutTarget = 1.f - (damageStage * 0.75f);
            } else {
                dropoutTimer = 50;
            }
        }
        dropoutGain += (dropoutTarget - dropoutGain) * 0.01f;
        if (dropoutGain > 0.995f) dropoutTarget = 1.f;



        // ── DRIVE — Tape saturatie + wavefold ─
        // Stap 1: Asymmetrische tape saturatie
        auto tapeSaturate = [](float x, float amt) -> float {
            // Positief en negatief anders behandeld — tape karakter
            if (x >= 0.f)
                return tanhf(x * amt) / amt;
            else
                return -tanhf(-x * amt * 0.8f) / (amt * 0.8f);
        };


        float driveAmt  = 1.f + drive * 2.0f;


        // ── STEREO SPREAD ─────────────────────
        float spread = clamp(params[SPREAD_PARAM].getValue()
            + (inputs[SPREAD_CV_INPUT].isConnected()
                ? inputs[SPREAD_CV_INPUT].getVoltage() * 0.1f : 0.f), 0.f, 1.f);
        spreadSmooth += (spread - spreadSmooth) * 0.01f;

        // Spread: L iets eerder, R iets later
        float spreadSamples = spreadSmooth * delaySamples * 0.25f;

        auto readSpread = [&](float offset) -> float {
            int pos = writePos - (int)clamp(offset, 1.f, (float)(MAX_BUFFER - 1));
            if (pos < 0) pos += MAX_BUFFER;
            return buffer[pos];
        };

        // Wobble pitch effect — delayed leestijd schommelt
        float wobbleTimeL = delaySamples * (1.f + wobble * damageStage * 2.f);
        float wobbleTimeR = delaySamples * (1.f + wobble * damageStage * 2.f);
        float delayedL = readSpread(clamp(wobbleTimeL - spreadSamples, 1.f, (float)(MAX_BUFFER-1)));
        float delayedR = readSpread(clamp(wobbleTimeR + spreadSamples, 1.f, (float)(MAX_BUFFER-1)));

        // Tape blend op L en R
        // Tape karakter toepassen op delayedL/R — flutter, saturatie en hiss
        // Wet = delayed altijd volledig + tape karakter mengt erbij
        float wetL = delayedL;
        float wetR = delayedR;

        // Tape wobble — duidelijke modulatie
        float wobbleMod = 1.f + wobble * 3.f;
        wetL *= wobbleMod;
        wetR *= wobbleMod;

        // Dropouts + makeup gain bij hoge tape
        float tapeMakeup = 1.f + tape * 0.6f;
        wetL *= dropoutGain * tapeMakeup;
        wetR *= dropoutGain * tapeMakeup;

        // Hiss erbovenop — hoger na 50%
        float hissLvl = tape < 0.5f ? tape * 0.1f : 0.05f + (tape - 0.5f) * 0.6f;
        wetL += hissHigh * hissLvl;
        wetR += hissHigh * hissLvl;

        // Drive op wet
        float makeupGain = 1.f + clamp((drive - 0.75f) / 0.25f, 0.f, 1.f) * 0.8f;
        float wetDrivenL = tapeSaturate(wetL, driveAmt) * makeupGain;
        float wetDrivenR = tapeSaturate(wetR, driveAmt) * makeupGain;

        // ── MIX + OUTPUT ──────────────────────
        float dryGain = cosf(mix * M_PI * 0.5f);
        float wetGain = sinf(mix * M_PI * 0.5f) * 1.3f;

        float dryL = dry;
        float dryR = inputs[AUDIO_R_INPUT].isConnected() ? dryRawR : dry;

        // Brake: pitch op wet via readBufBrake, volume op alles
        float outL = (dryL * dryGain + wetDrivenL * wetGain) * brakeSpeedSmooth;
        float outR = (dryR * dryGain + wetDrivenR * wetGain) * brakeSpeedSmooth;

        // Surge bloom
        float bloom = delayed * freezeGain * mix * 1.3f;
        outL += bloom;
        outR += bloom;

        // Keep Chrono running behind the bypass and only crossfade its output.
        outL += (dryL - outL) * dryFade;
        outR += (dryR - outR) * dryFade;

        outputs[AUDIO_L_OUTPUT].setVoltage(outL);
        outputs[AUDIO_R_OUTPUT].setVoltage(outR);
    }
};

// ─────────────────────────────────────────────
//  WIDGET
// ─────────────────────────────────────────────

struct ChronoWidget : SubmitModuleWidget {
    ChronoWidget(Chrono* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Chrono.svg")));

        // ── Grote knoppen ─────────────────────
        addParam(createParamCentered<Drift13KnobMedium>(
            Vec(59.771f, 103.966f), module, Chrono::TIME_PARAM));
        addParam(createParamCentered<Drift13KnobMedium>(
            Vec(59.771f, 186.561f), module, Chrono::FEEDBACK_PARAM));

        // ── Small knoppen ─────────────────────
        addParam(createParamCentered<Drift13KnobSmall>(
            Vec(34.152f, 264.151f), module, Chrono::DIVISION_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(
            Vec(87.892f, 264.401f), module, Chrono::SPACING_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(
            Vec(142.126f, 264.401f), module, Chrono::SPREAD_PARAM));

        // ── Sliders ───────────────────────────
        addParam(createParam<ChronoSlider>(
            Vec(117.924f, 80.58f), module, Chrono::MIX_PARAM));
        addParam(createParam<ChronoSlider>(
            Vec(154.506f, 80.58f), module, Chrono::DRIVE_PARAM));
        addParam(createParam<ChronoSlider>(
            Vec(191.614f, 80.58f), module, Chrono::TAPE_PARAM));
        addParam(createParam<ChronoSlider>(
            Vec(228.538f, 80.58f), module, Chrono::HEADS_PARAM));

        // ── Momentary buttons ─────────────────
        addParam(createParam<ChronoSurgeButton>(
            Vec(176.688f, 246.326f), module, Chrono::SURGE_PARAM));
        addParam(createParam<ChronoBreakButton>(
            Vec(229.956f, 246.326f), module, Chrono::BREAK_PARAM));

        // ── LEDs ──────────────────────────────
        addChild(createLightCentered<SmallLight<YellowLight>>(
            Vec(179.082f, 235.185f), module, Chrono::SURGE_LIGHT));
        addChild(createLightCentered<SmallLight<YellowLight>>(
            Vec(237.453f, 235.185f), module, Chrono::BREAK_LIGHT));

        // ── CV inputs sliders ─────────────────
        addInput(createInputCentered<PJ301MPort>(
            Vec(124.137f, 202.452f), module, Chrono::MIX_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            Vec(160.875f, 202.452f), module, Chrono::DRIVE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            Vec(197.667f, 202.452f), module, Chrono::TAPE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            Vec(236.061f, 202.452f), module, Chrono::HEADS_CV_INPUT));

        // ── CV inputs knoppen ─────────────────
        addInput(createInputCentered<PJ301MPort>(
            Vec(20.850f, 76.616f), module, Chrono::TIME_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            Vec(20.850f, 157.903f), module, Chrono::FEEDBACK_CV_INPUT));

        // ── Onderste rij inputs ───────────────
        addInput(createInputCentered<PJ301MPort>(
            Vec(34.100f, 299.959f), module, Chrono::CLOCK_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            Vec(87.892f, 299.959f), module, Chrono::SPACING_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            Vec(141.387f, 299.959f), module, Chrono::SPREAD_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            Vec(195.214f, 299.959f), module, Chrono::SURGE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            Vec(248.994f, 299.959f), module, Chrono::BREAK_CV_INPUT));

        // ── Audio poorten ─────────────────────
        addInput(createInputCentered<PJ301MPort>(
            Vec(34.397f, 343.604f), module, Chrono::AUDIO_L_INPUT));
        addInput(createInputCentered<PJ301MPort>(
            Vec(87.344f, 343.604f), module, Chrono::AUDIO_R_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(
            Vec(141.387f, 343.604f), module, Chrono::AUDIO_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(
            Vec(195.531f, 343.604f), module, Chrono::AUDIO_R_OUTPUT));
    }

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        Chrono* chrono = dynamic_cast<Chrono*>(module);
        if (chrono) {
            menu->addChild(createMenuLabel("Clock input rate"));
            menu->addChild(createCheckMenuItem("1 PPQN (Submit standard)", "",
                [=]() { return chrono->clockPpqn == 1; },
                [=]() { chrono->setClockPpqn(1); }));
            menu->addChild(createCheckMenuItem("4 PPQN (compatibility)", "",
                [=]() { return chrono->clockPpqn == 4; },
                [=]() { chrono->setClockPpqn(4); }));
            menu->addChild(new MenuSeparator);
            menu->addChild(createCheckMenuItem("Clocked Time: Extra divisions", "",
                [=]() { return chrono->clockedTimeEnabled; },
                [=]() { chrono->clockedTimeEnabled = !chrono->clockedTimeEnabled; }));
            menu->addChild(new MenuSeparator);
        }
        menu->addChild(createMenuItem("Manual", "", []() {
            system::openBrowser("https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/chrono/");
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


Model* modelChrono = createModel<Chrono, ChronoWidget>("Chrono");
