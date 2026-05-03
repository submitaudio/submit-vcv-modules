#include "plugin.hpp"

// dr_wav — single header WAV loader (public domain, mackron/dr_libs)
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include "osdialog.h"
#include <cstring>
#include <cmath>
#include <algorithm>

// ═══════════════════════════════════════════════════════════
//   twiN  —  Dual-head sample player
//   Twee onafhankelijke playheads door dezelfde sample
//   SCAN beweegt beide heads samen
//   Submit Audio 2026
// ═══════════════════════════════════════════════════════════

// Maximum sample buffer grootte (stereo, 44100 * 120 sec = ~10MB)
static constexpr int MAX_SAMPLES = 44100 * 120 * 2;

// ─────────────────────────────────────────────
//  Eenvoudige interpolerende sample reader
// ─────────────────────────────────────────────
struct SampleHead {
    float pos    = 0.f;   // positie in samples (float voor interpolatie)
    float speed  = 1.f;   // afspeelsnelheid (1.0 = normaal)
    float pitch  = 0.f;   // pitch offset in semitonen
    float vol    = 1.f;   // volume
    bool  active = false;

    // Lees geïnterpoleerde sample uit buffer
    // buffer = stereo interleaved [L0, R0, L1, R1, ...]
    std::pair<float,float> read(const float* buffer, int numSamples) {
        if (!buffer || numSamples < 2) return {0.f, 0.f};

        // Clamp positie
        float p = pos;
        while (p >= numSamples) p -= numSamples;
        while (p < 0)           p += numSamples;

        // Lineaire interpolatie tussen twee frames
        int   i0 = (int)p;
        int   i1 = (i0 + 1) % numSamples;
        float t  = p - i0;

        float L = buffer[i0*2]   * (1.f-t) + buffer[i1*2]   * t;
        float R = buffer[i0*2+1] * (1.f-t) + buffer[i1*2+1] * t;

        return {L * vol, R * vol};
    }

    // Stap vooruit
    void advance(float sr) {
        float pitchFactor = powf(2.f, pitch / 12.f);
        pos += speed * pitchFactor;
    }
};

// ─────────────────────────────────────────────
//  Module
// ─────────────────────────────────────────────
struct twiN : Module {

    enum ParamId {
        // HEAD 1
        PARAM_POS1, PARAM_SPEED1, PARAM_PITCH1, PARAM_VOL1,
        // HEAD 2
        PARAM_POS2, PARAM_SPEED2, PARAM_PITCH2, PARAM_VOL2,
        // GLOBAL
        PARAM_SCAN, PARAM_SIZE, PARAM_XFADE, PARAM_LOOP,
        PARAMS_LEN
    };

    enum InputId {
        INPUT_GATE,
        INPUT_CLOCK,
        INPUT_SCAN_CV,
        INPUT_POS1_CV,
        INPUT_POS2_CV,
        INPUTS_LEN
    };

    enum OutputId {
        OUTPUT_L,
        OUTPUT_R,
        OUTPUT_HEAD1_L, OUTPUT_HEAD1_R,
        OUTPUT_HEAD2_L, OUTPUT_HEAD2_R,
        OUTPUTS_LEN
    };

    enum LightId {
        LIGHT_LOADED,
        LIGHTS_LEN
    };

    // Sample buffer (stereo interleaved)
    float*  sampleBuffer = nullptr;
    int     numSamples   = 0;     // aantal frames
    float   sampleRate_wav = 44100.f;
    bool    loaded       = false;
    std::string loadedPath = "";

    SampleHead head1, head2;

    // Gate
    bool gateHigh = false;
    dsp::SchmittTrigger gateTrig;
    dsp::SchmittTrigger clockTrig;

    // Crossfade teller
    float xfadePos = 0.f;

    twiN() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // HEAD 1
        configParam(PARAM_POS1,   0.f, 1.f, 0.f,   "Head 1 Position");
        configParam(PARAM_SPEED1, 0.f, 4.f, 1.f,   "Head 1 Speed");
        configParam(PARAM_PITCH1,-12.f,12.f, 0.f,  "Head 1 Pitch",   " st");
        configParam(PARAM_VOL1,   0.f, 1.f, 1.f,   "Head 1 Volume");

        // HEAD 2
        configParam(PARAM_POS2,   0.f, 1.f, 0.1f,  "Head 2 Position");
        configParam(PARAM_SPEED2, 0.f, 4.f, 1.f,   "Head 2 Speed");
        configParam(PARAM_PITCH2,-12.f,12.f, 0.f,  "Head 2 Pitch",   " st");
        configParam(PARAM_VOL2,   0.f, 1.f, 1.f,   "Head 2 Volume");

        // GLOBAL
        configParam(PARAM_SCAN,  0.f, 1.f, 0.f,   "Scan");
        configParam(PARAM_SIZE,  0.f, 1.f, 1.f,   "Loop Size");
        configParam(PARAM_XFADE, 0.f, 1.f, 0.5f,  "Crossfade");
        configSwitch(PARAM_LOOP, 0.f, 1.f, 1.f,   "Loop", {"Off", "On"});

        configInput(INPUT_GATE,    "Gate");
        configInput(INPUT_CLOCK,   "Clock");
        configInput(INPUT_SCAN_CV, "Scan CV");
        configInput(INPUT_POS1_CV, "Head 1 Position CV");
        configInput(INPUT_POS2_CV, "Head 2 Position CV");

        configOutput(OUTPUT_L,      "Mix Left");
        configOutput(OUTPUT_R,      "Mix Right");
        configOutput(OUTPUT_HEAD1_L,"Head 1 Left");
        configOutput(OUTPUT_HEAD1_R,"Head 1 Right");
        configOutput(OUTPUT_HEAD2_L,"Head 2 Left");
        configOutput(OUTPUT_HEAD2_R,"Head 2 Right");

        // Alloceer sample buffer
        sampleBuffer = new float[MAX_SAMPLES * 2]();
    }

    ~twiN() {
        delete[] sampleBuffer;
    }

    void loadSample(const std::string& path) {
        // Laad WAV via Rack's ingebouwde audio file support
        unsigned int channels, sampleRateWav;
        drwav_uint64 frameCount;

        float* data = drwav_open_file_and_read_pcm_frames_f32(
            path.c_str(), &channels, &sampleRateWav, &frameCount, nullptr);

        if (!data) return;

        int frames = (int)std::min((drwav_uint64)MAX_SAMPLES, frameCount);

        if (channels == 1) {
            // Mono naar stereo
            for (int i = frames-1; i >= 0; i--) {
                sampleBuffer[i*2]   = data[i];
                sampleBuffer[i*2+1] = data[i];
            }
        } else {
            // Stereo (of meer kanalen — neem alleen L en R)
            for (int i = 0; i < frames; i++) {
                sampleBuffer[i*2]   = data[i * channels];
                sampleBuffer[i*2+1] = data[i * channels + 1];
            }
        }

        drwav_free(data, nullptr);

        numSamples     = frames;
        sampleRate_wav = (float)sampleRateWav;
        loaded         = true;
        loadedPath     = path;

        // Reset heads naar begin
        head1.pos = 0.f;
        head2.pos = numSamples * 0.1f;
    }

    void process(const ProcessArgs& args) override {
        if (!loaded || numSamples == 0) {
            outputs[OUTPUT_L].setVoltage(0.f);
            outputs[OUTPUT_R].setVoltage(0.f);
            return;
        }

        // ── Parameters lezen ─────────────────────────────────
        float scan   = params[PARAM_SCAN].getValue()
                     + inputs[INPUT_SCAN_CV].getVoltage() * 0.1f;
        scan = clamp(scan, 0.f, 1.f);

        float size   = params[PARAM_SIZE].getValue();
        float xfade  = params[PARAM_XFADE].getValue();
        bool  loop   = params[PARAM_LOOP].getValue() > 0.5f;

        // Positie van de twee heads (relatief + scan offset)
        float pos1 = params[PARAM_POS1].getValue()
                   + inputs[INPUT_POS1_CV].getVoltage() * 0.1f;
        float pos2 = params[PARAM_POS2].getValue()
                   + inputs[INPUT_POS2_CV].getVoltage() * 0.1f;
        pos1 = clamp(pos1, 0.f, 1.f);
        pos2 = clamp(pos2, 0.f, 1.f);

        // SCAN: verschuift beide heads samen, onderlinge afstand blijft
        float scanOffset = scan;
        float absPos1 = fmodf(pos1 + scanOffset, 1.f);
        float absPos2 = fmodf(pos2 + scanOffset, 1.f);

        // Loop grootte in samples
        // SIZE knob: 0 = 10ms, 1 = 1 seconde — volledige knop = fijne controle
        float minLoopSamples = 0.010f * args.sampleRate; // 10ms
        float maxLoopSamples = 1.000f * args.sampleRate; // 1 seconde
        float loopSize = minLoopSamples + size * (maxLoopSamples - minLoopSamples);

        // HEAD 1 instellen
        head1.speed = params[PARAM_SPEED1].getValue()
                    * (sampleRate_wav / args.sampleRate);
        head1.pitch = params[PARAM_PITCH1].getValue();
        head1.vol   = params[PARAM_VOL1].getValue();

        // HEAD 2 instellen
        head2.speed = params[PARAM_SPEED2].getValue()
                    * (sampleRate_wav / args.sampleRate);
        head2.pitch = params[PARAM_PITCH2].getValue();
        head2.vol   = params[PARAM_VOL2].getValue();

        // Gate / Clock
        if (gateTrig.process(inputs[INPUT_GATE].getVoltage())) {
            // Reset heads naar huidige positie bij gate trigger
            head1.pos = absPos1 * numSamples;
            head2.pos = absPos2 * numSamples;
        }

        if (clockTrig.process(inputs[INPUT_CLOCK].getVoltage())) {
            // Clock: spring naar volgende loop
            head1.pos = absPos1 * numSamples;
            head2.pos = absPos2 * numSamples;
        }

        // Loop boundaries
        float start1 = absPos1 * numSamples;
        float end1   = start1 + loopSize;
        if (end1 > numSamples) end1 = numSamples; // clamp aan einde sample
        float start2 = absPos2 * numSamples;
        float end2   = start2 + loopSize;
        if (end2 > numSamples) end2 = numSamples;

        // HEAD 1: zorg dat pos altijd binnen loop boundaries zit
        if (head1.pos < start1 || head1.pos >= end1) {
            head1.pos = start1; // reset naar begin van loop
        }

        // HEAD 2: zorg dat pos altijd binnen loop boundaries zit
        if (head2.pos < start2 || head2.pos >= end2) {
            head2.pos = start2;
        }

        // Lees samples
        std::pair<float,float> out1 = head1.read(sampleBuffer, numSamples);
        std::pair<float,float> out2 = head2.read(sampleBuffer, numSamples);
        float L1 = out1.first,  R1 = out1.second;
        float L2 = out2.first,  R2 = out2.second;

        // Stap vooruit
        head1.advance(args.sampleRate);
        head2.advance(args.sampleRate);

        // Crossfade tussen heads
        float mixL = L1 * (1.f - xfade) + L2 * xfade;
        float mixR = R1 * (1.f - xfade) + R2 * xfade;

        // Outputs (±5V)
        outputs[OUTPUT_L].setVoltage(clamp(mixL * 5.f, -10.f, 10.f));
        outputs[OUTPUT_R].setVoltage(clamp(mixR * 5.f, -10.f, 10.f));
        outputs[OUTPUT_HEAD1_L].setVoltage(clamp(L1 * 5.f, -10.f, 10.f));
        outputs[OUTPUT_HEAD1_R].setVoltage(clamp(R1 * 5.f, -10.f, 10.f));
        outputs[OUTPUT_HEAD2_L].setVoltage(clamp(L2 * 5.f, -10.f, 10.f));
        outputs[OUTPUT_HEAD2_R].setVoltage(clamp(R2 * 5.f, -10.f, 10.f));

        lights[LIGHT_LOADED].setBrightness(loaded ? 1.f : 0.f);
    }

    // Sla sample pad op in patch
    json_t* dataToJson() override {
        json_t* root = json_object();
        json_object_set_new(root, "samplePath", json_string(loadedPath.c_str()));
        return root;
    }

    void dataFromJson(json_t* root) override {
        json_t* pathJ = json_object_get(root, "samplePath");
        if (pathJ) {
            std::string path = json_string_value(pathJ);
            if (!path.empty()) loadSample(path);
        }
    }
};

// ═══════════════════════════════════════════════════════════
//   Waveform Display Widget
//   Toont de sample golfvorm met twee gekleurde lijnen
//   voor de positie van HEAD 1 en HEAD 2
// ═══════════════════════════════════════════════════════════
struct twiNDisplay : Widget {
    twiN* module = nullptr;

    void draw(const DrawArgs& args) override {
        // Achtergrond
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
        nvgFillColor(args.vg, nvgRGB(0x0a, 0x0a, 0x0a));
        nvgFill(args.vg);

        if (!module || !module->loaded || module->numSamples == 0) {
            // Geen sample — toon tekst
            nvgFontSize(args.vg, 11.f);
            nvgFillColor(args.vg, nvgRGB(0x55, 0x55, 0x55));
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, box.size.x * 0.5f, box.size.y * 0.5f, "load sample", nullptr);
            return;
        }

        // Teken waveform
        float w = box.size.x;
        float h = box.size.y;
        float mid = h * 0.5f;
        int   n   = module->numSamples;

        nvgBeginPath(args.vg);
        nvgStrokeColor(args.vg, nvgRGBA(0x33, 0xcc, 0x88, 0xaa));
        nvgStrokeWidth(args.vg, 0.8f);

        for (int x = 0; x < (int)w; x++) {
            int idx = (int)((float)x / w * n);
            idx = clamp(idx, 0, n-1);
            float samp = module->sampleBuffer[idx * 2]; // L kanaal
            float y = mid - samp * mid * 0.9f;
            if (x == 0) nvgMoveTo(args.vg, x, y);
            else        nvgLineTo(args.vg, x, y);
        }
        nvgStroke(args.vg);

        // HEAD 1 en HEAD 2 lijnen + zone ertussen
        if (module->numSamples > 0) {
            float scan = module->params[twiN::PARAM_SCAN].getValue();
            float p1   = fmodf(module->params[twiN::PARAM_POS1].getValue() + scan, 1.f);
            float p2   = fmodf(module->params[twiN::PARAM_POS2].getValue() + scan, 1.f);
            float x1   = p1 * w;
            float x2   = p2 * w;

            // Gekleurde zone tussen de twee heads
            float xLeft  = std::min(x1, x2);
            float xRight = std::max(x1, x2);
            float zoneW  = xRight - xLeft;
            if (zoneW < 1.f) zoneW = 1.f; // altijd zichtbaar

            nvgBeginPath(args.vg);
            nvgRect(args.vg, xLeft, 0, zoneW, h);
            nvgFillColor(args.vg, nvgRGBA(0xff, 0xd7, 0x00, 0x22)); // gele tint
            nvgFill(args.vg);

            // HEAD 1 lijn — geel
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, x1, 0);
            nvgLineTo(args.vg, x1, h);
            nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xd7, 0x00, 0xff));
            nvgStrokeWidth(args.vg, 1.5f);
            nvgStroke(args.vg);

            // HEAD 2 lijn — wit
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, x2, 0);
            nvgLineTo(args.vg, x2, h);
            nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xff, 0xff, 0xff));
            nvgStrokeWidth(args.vg, 1.5f);
            nvgStroke(args.vg);

            // Animerende lijn HEAD 1 — oranje, toont echte afspeelpositie
            {
                float playPos1 = module->head1.pos / module->numSamples;
                playPos1 = fmodf(playPos1, 1.f);
                float xPlay1 = playPos1 * w;
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, xPlay1, 0);
                nvgLineTo(args.vg, xPlay1, h);
                nvgStrokeColor(args.vg, nvgRGBA(0xff, 0x88, 0x00, 0xff)); // oranje
                nvgStrokeWidth(args.vg, 1.2f);
                nvgStroke(args.vg);
            }

            // Animerende lijn HEAD 2 — lichtblauw, toont echte afspeelpositie
            {
                float playPos2 = module->head2.pos / module->numSamples;
                playPos2 = fmodf(playPos2, 1.f);
                float xPlay2 = playPos2 * w;
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, xPlay2, 0);
                nvgLineTo(args.vg, xPlay2, h);
                nvgStrokeColor(args.vg, nvgRGBA(0x00, 0xcc, 0xff, 0xff)); // blauw
                nvgStrokeWidth(args.vg, 1.2f);
                nvgStroke(args.vg);
            }
        }

        // Border
        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, w, h);
        nvgStrokeColor(args.vg, nvgRGBA(0xff, 0xd7, 0x00, 0x66));
        nvgStrokeWidth(args.vg, 0.8f);
        nvgStroke(args.vg);
    }
};

// ═══════════════════════════════════════════════════════════
//   Widget
// ═══════════════════════════════════════════════════════════
struct twiNWidget : ModuleWidget {
    twiNDisplay* display = nullptr;

    twiNWidget(twiN* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/twiN.svg")));

        // Screws
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2*RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2*RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Waveform display (bovenaan, bijna volledige breedte)
        display = createWidget<twiNDisplay>(mm2px(Vec(3.f, 8.f)));
        display->box.size = mm2px(Vec(134.f, 38.f)); // 28HP - marges
        display->module = module;
        addChild(display);

        // ── HEAD 1 (geel label) ──────────────────────────────
        // Rij: POS1, SPEED1, PITCH1, VOL1
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(18,  58)), module, twiN::PARAM_POS1));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(36,  58)), module, twiN::PARAM_SPEED1));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(54,  58)), module, twiN::PARAM_PITCH1));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(72,  58)), module, twiN::PARAM_VOL1));

        // ── HEAD 2 (wit label) ───────────────────────────────
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(18,  78)), module, twiN::PARAM_POS2));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(36,  78)), module, twiN::PARAM_SPEED2));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(54,  78)), module, twiN::PARAM_PITCH2));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(72,  78)), module, twiN::PARAM_VOL2));

        // ── GLOBAL ───────────────────────────────────────────
        addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(96,  58)), module, twiN::PARAM_SCAN));
        addParam(createParamCentered<RoundBlackKnob>     (mm2px(Vec(114, 58)), module, twiN::PARAM_SIZE));
        addParam(createParamCentered<RoundBlackKnob>     (mm2px(Vec(96,  78)), module, twiN::PARAM_XFADE));
        addParam(createParamCentered<CKSS>               (mm2px(Vec(114, 78)), module, twiN::PARAM_LOOP));

        // ── CV Inputs ────────────────────────────────────────
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(18,  98)), module, twiN::INPUT_GATE));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36,  98)), module, twiN::INPUT_CLOCK));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(54,  98)), module, twiN::INPUT_SCAN_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(72,  98)), module, twiN::INPUT_POS1_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(90,  98)), module, twiN::INPUT_POS2_CV));

        // ── Outputs ──────────────────────────────────────────
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(18,  114)), module, twiN::OUTPUT_HEAD1_L));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30,  114)), module, twiN::OUTPUT_HEAD1_R));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(54,  114)), module, twiN::OUTPUT_HEAD2_L));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(66,  114)), module, twiN::OUTPUT_HEAD2_R));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(108, 114)), module, twiN::OUTPUT_L));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(120, 114)), module, twiN::OUTPUT_R));

        // Loaded LED
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(134, 114)), module, twiN::LIGHT_LOADED));
    }

    // Context menu voor sample laden
    void appendContextMenu(Menu* menu) override {
        twiN* m = dynamic_cast<twiN*>(module);
        if (!m) return;

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Load sample...", "", [=]() {
            char* pathC = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);
            if (pathC) {
                m->loadSample(std::string(pathC));
                free(pathC);
            }
        }));

        if (m->loaded) {
            std::string filename = m->loadedPath;
            auto pos = filename.find_last_of("/\\");
            if (pos != std::string::npos) filename = filename.substr(pos + 1);
            menu->addChild(createMenuLabel("Loaded: " + filename));
        }
    }
};

Model* modelTwiN = createModel<twiN, twiNWidget>("twiN");
