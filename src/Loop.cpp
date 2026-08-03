#include "plugin.hpp"
#include "osdialog.h"
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#ifndef DR_WAV_IMPLEMENTATION
#define DR_WAV_IMPLEMENTATION
#endif
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

struct Reel : Module {
    enum ParamIds {
        BARS_PARAM,
        BPM_PARAM,
        SPEED_PARAM,
        SYNC_PARAM,
        REVERSE_PARAM,
        CUE_PARAM,
        RESET_PARAM,
        BARSHIFT_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CLOCK_INPUT,
        TRIG_INPUT,
        SPEED_CV_INPUT,
        BARS_CV_INPUT,
        BPM_CV_INPUT,
        BARSHIFT_CV_INPUT,
        REVERSE_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        MAIN_L_OUTPUT,
        MAIN_R_OUTPUT,
        CUE_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        SYNC_LIGHT,
        REVERSE_LIGHT,
        CUE_LIGHT,
        NUM_LIGHTS
    };

    // Audio buffer
    std::vector<float> bufferL;
    std::vector<float> bufferR;
    bool stereo = false;
    int totalFrames = 0;
    int fileSampleRate = 44100;

    // Playback
    double playPos = 0.0;
    double displayPos = 0.0;  // voor waveform display
    double playSpeed = 1.0;
    bool fileLoaded = false;

    // Sample info
    float fileBpm = 0.f;
    float detectedBars = 0.f;
    std::string fileName = "";
    std::string displayName = "";

    // Clock tracking
    float clockPrev = 0.f;
    double clockBpm = 120.0;
    int clockSampleCount = 0;
    bool clockReceived = false;

    // Trig / Reset
    float trigPrev = 0.f;
    float resetPrev = 0.f;
    bool resetPending = false;
    double barProgress = 0.0;  // positie binnen huidige bar
    float fadeGain = 1.f;      // huidige gain 0.0-1.0
    bool fading = false;        // fade bezig
    bool pendingLive = false;   // wacht op einde cyclus
    bool activeCue = false;     // actieve cue staat
    float lastCue = 0.f;
    bool pendingReverse = false;
    bool activeReverse = false;
    bool reversePending = false;
    float lastReverse = 0.f;
    float lastBarShift = 0.f;
    float pendingBarShift = 0.f;
    bool barShiftPending = false;
    double activeLoopOffset = 0.0;

    Reel() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(BARS_PARAM, 0.f, 24.f, 0.f, "Bars (0 = auto)");
        getParamQuantity(BARS_PARAM)->snapEnabled = true;
        configParam(BPM_PARAM, 0.f, 300.f, 0.f, "BPM (0 = auto)");
        configParam(SPEED_PARAM, -1.f, 1.f, 0.f, "Speed");
        configParam(SYNC_PARAM, 0.f, 1.f, 1.f, "Sync");
        configParam(REVERSE_PARAM, 0.f, 1.f, 0.f, "Reverse");
        configParam(CUE_PARAM, 0.f, 1.f, 0.f, "Cue");
        configParam(RESET_PARAM, 0.f, 1.f, 0.f, "Reset");
        configParam(BARSHIFT_PARAM, 1.f, 8.f, 1.f, "Bar Shift");
        getParamQuantity(BARSHIFT_PARAM)->snapEnabled = true;
        configInput(CLOCK_INPUT, "Clock");
        configInput(TRIG_INPUT, "Trigger/Reset");
        configInput(SPEED_CV_INPUT, "Speed CV");
        configInput(BARS_CV_INPUT, "Bars CV");
        configInput(BPM_CV_INPUT, "BPM CV");
        configInput(BARSHIFT_CV_INPUT, "Bar Shift CV");
        configInput(REVERSE_CV_INPUT, "Reverse CV");
        configOutput(MAIN_L_OUTPUT, "Main L");
        configOutput(MAIN_R_OUTPUT, "Main R");
        configOutput(CUE_OUTPUT, "Cue Mono");
    }

    void loadSample(const std::string& path) {
        drwav wav;
        if (!drwav_init_file(&wav, path.c_str(), nullptr)) return;

        int frames = (int)wav.totalPCMFrameCount;
        int ch = wav.channels;
        fileSampleRate = wav.sampleRate;

        std::vector<float> raw(frames * ch);
        drwav_read_pcm_frames_f32(&wav, frames, raw.data());
        drwav_uninit(&wav);

        bufferL.resize(frames);
        bufferR.resize(frames);
        stereo = (ch >= 2);

        for (int i = 0; i < frames; i++) {
            bufferL[i] = raw[i * ch];
            bufferR[i] = stereo ? raw[i * ch + 1] : raw[i * ch];
        }

        totalFrames = frames;
        playPos = 0.0;
        fileLoaded = true;
        filePath = path;
        activeLoopOffset = 0.0;
        pendingBarShift = 1.f;
        lastBarShift = 1.f;
        barShiftPending = false;
        resetPending = false;
        activeReverse = false;
        reversePending = false;
        lastReverse = 0.f;
        activeCue = false;
        pendingLive = false;
        fading = false;
        fadeGain = 1.f;
        lastCue = 0.f;

        // Bestandsnaam
        size_t sep = path.find_last_of("/\\");
        fileName = (sep != std::string::npos) ? path.substr(sep + 1) : path;
        displayName = fileName;
        if (displayName.size() > 28)
            displayName = displayName.substr(0, 26) + "..";

        // BPM uit bestandsnaam
        fileBpm = 0.f;
        std::string lower = fileName;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        size_t bpmPos = lower.find("bpm");
        if (bpmPos != std::string::npos && bpmPos > 0) {
            size_t start = bpmPos;
            while (start > 0 && std::isdigit((unsigned char)lower[start-1])) start--;
            std::string bpmStr = lower.substr(start, bpmPos - start);
            if (!bpmStr.empty()) {
                try { fileBpm = std::stof(bpmStr); } catch (...) {}
            }
        }

        // Bars auto-detectie
        detectedBars = 0.f;
        if (fileBpm > 0.f) {
            float durationSec = (float)frames / (float)fileSampleRate;
            float beats = durationSec / (60.f / fileBpm);
            float bars = beats / 4.f;
            float rounded = std::round(bars);
            if (std::abs(bars - rounded) < 0.15f && rounded >= 1.f)
                detectedBars = rounded;
        }
    }

    float getSample(std::vector<float>& buf, double pos) {
        if (buf.empty() || totalFrames == 0) return 0.f;
        int n = totalFrames;
        int i0 = (int)pos;
        i0 = ((i0 % n) + n) % n;
        int i1 = (i0 + 1) % n;
        float frac = (float)(pos - std::floor(pos));
        return buf[i0] * (1.f - frac) + buf[i1] * frac;
    }

    void process(const ProcessArgs& args) override {
        // Reset knop — quantized: wacht op volgende clock
        float resetBtn = params[RESET_PARAM].getValue();
        if (resetPrev < 0.5f && resetBtn >= 0.5f) resetPending = true;
        resetPrev = resetBtn;

        // Trig input reset — ook quantized
        float trigIn = inputs[TRIG_INPUT].getVoltage();
        if (trigPrev < 1.f && trigIn >= 1.f) resetPending = true;
        trigPrev = trigIn;

        if (!fileLoaded || totalFrames == 0) {
            outputs[MAIN_L_OUTPUT].setVoltage(0.f);
            outputs[MAIN_R_OUTPUT].setVoltage(0.f);
            outputs[CUE_OUTPUT].setVoltage(0.f);
            return;
        }

        // Fade in updaten
        if (fading) {
            fadeGain += 1.f / (1.f * args.sampleRate); // 1 seconde
            if (fadeGain >= 1.f) {
                fadeGain = 1.f;
                fading = false;
            }
        }

        // Bar shift — detecteer wijziging, wacht op einde cyclus
        float currentBarShift = params[BARSHIFT_PARAM].getValue();
        if (inputs[BARSHIFT_CV_INPUT].isConnected())
            currentBarShift = clamp(currentBarShift + inputs[BARSHIFT_CV_INPUT].getVoltage() * 0.8f, 1.f, 8.f);
        currentBarShift = std::round(currentBarShift);
        if (currentBarShift != lastBarShift) {
            pendingBarShift = currentBarShift;
            barShiftPending = true;
            lastBarShift = currentBarShift;
        }

        // Reverse — detecteer wijziging, wacht op einde cyclus
        float currentReverse = params[REVERSE_PARAM].getValue();
        if (inputs[REVERSE_CV_INPUT].isConnected() && inputs[REVERSE_CV_INPUT].getVoltage() >= 1.f)
            currentReverse = 1.f - currentReverse;
        if (currentReverse != lastReverse) {
            pendingReverse = currentReverse > 0.5f;
            reversePending = true;
            lastReverse = currentReverse;
        }

        // CUE schakelaar — quantized + fade in bij LIVE
        float currentCue = params[CUE_PARAM].getValue();
        if (currentCue != lastCue) {
            if (currentCue < 0.5f) {
                // Naar LIVE — direct met fade in
                activeCue = false;
                fadeGain = 0.f;
                fading = true;
                pendingLive = false;
            } else {
                // Naar CUE — direct
                activeCue = true;
                fading = false;
                fadeGain = 1.f;
            }
            lastCue = currentCue;
        }

        // Clock BPM meten
        float clockIn = inputs[CLOCK_INPUT].getVoltage();
        if (clockPrev < 1.f && clockIn >= 1.f) {
            if (clockReceived && clockSampleCount > 0) {
                double interval = (double)clockSampleCount / args.sampleRate;
                double measured = 60.0 / interval;
                if (measured > 20.0 && measured < 400.0)
                    clockBpm = clockBpm * 0.85 + measured * 0.15;
            }
            clockSampleCount = 0;
            clockReceived = true;
            // Quantized reset — spring naar 0 op clock pulse
            if (resetPending) {
                playPos = 0.0;
                resetPending = false;
            }
        }
        clockSampleCount++;
        clockPrev = clockIn;

        // BPM bepalen
        float bpmOverride = params[BPM_PARAM].getValue();
        if (inputs[BPM_CV_INPUT].isConnected())
            bpmOverride = clamp(bpmOverride + inputs[BPM_CV_INPUT].getVoltage() * 30.f, 0.f, 300.f);
        float sourceBpm = (bpmOverride > 0.f) ? bpmOverride : fileBpm;
        if (sourceBpm <= 0.f) sourceBpm = 120.f;

        // Bars bepalen
        float barsParam = params[BARS_PARAM].getValue();
        if (inputs[BARS_CV_INPUT].isConnected())
            barsParam = clamp(barsParam + inputs[BARS_CV_INPUT].getVoltage() * 2.4f, 1.f, 24.f);
        float bars = (barsParam > 0.f) ? barsParam : detectedBars;
        if (bars <= 0.f) bars = 4.f;

        // Speed berekenen
        bool sync = params[SYNC_PARAM].getValue() > 0.5f;

        // SPEED knob: -1 tot +1 → 0.5x tot 2x (±1 octaaf)
        double speedKnob = std::pow(2.0, (double)params[SPEED_PARAM].getValue());
        // SPEED CV: 5V = 1 octaaf omhoog, -5V = 1 octaaf omlaag
        double speedCv = inputs[SPEED_CV_INPUT].getVoltage() / 5.0;
        double speedMult = speedKnob * std::pow(2.0, speedCv);
        speedMult = clamp((float)speedMult, 0.25f, 4.0f);

        // Samplerate correctie — altijd nodig
        double srRatio = (double)fileSampleRate / args.sampleRate;

        if (sync && clockReceived) {
            // speedMult past effectief het afspeeltempo aan t.o.v. clock
            double effectiveBpm = clockBpm * speedMult;
            double loopDurationSec = (bars * 4.0 * 60.0) / effectiveBpm;
            double loopFrames = loopDurationSec * fileSampleRate;
            playSpeed = ((double)totalFrames / loopFrames) * srRatio;
        } else {
            // Vrij afspelen — speed knop werkt direct
            playSpeed = srRatio * speedMult;
        }

        if (activeReverse) playSpeed = -playSpeed;

        // Audio lezen
        double readPos = std::fmod(playPos + activeLoopOffset, (double)totalFrames);
        if (readPos < 0.0) readPos += totalFrames;
        displayPos = readPos;
        float outL = getSample(bufferL, readPos) * 5.f;
        float outR = getSample(bufferR, readPos) * 5.f;
        float outMono = (outL + outR) * 0.5f;

        // CUE of LIVE
        bool cue = activeCue;
        if (cue) {
            outputs[MAIN_L_OUTPUT].setVoltage(0.f);
            outputs[MAIN_R_OUTPUT].setVoltage(0.f);
            outputs[CUE_OUTPUT].setVoltage(outMono);
        } else {
            outputs[MAIN_L_OUTPUT].setVoltage(outL * fadeGain);
            outputs[MAIN_R_OUTPUT].setVoltage(outR * fadeGain);
            outputs[CUE_OUTPUT].setVoltage(0.f);
        }

        // Bar lengte in frames
        double framesPerBar = (bars > 0.f && totalFrames > 0)
            ? (double)totalFrames / (double)bars : (double)totalFrames;

        // Bar grens detectie — anders voor forward en reverse
        double prevBarProgress = barProgress;
        barProgress = std::fmod(std::abs(playPos), framesPerBar);
        bool atBarBoundaryFwd = (!activeReverse && barProgress < prevBarProgress);
        bool atBarBoundaryRev = (activeReverse && barProgress > prevBarProgress);
        bool atBarBoundary = atBarBoundaryFwd || atBarBoundaryRev;

        // Reverse op bar grens — wacht altijd op bar grens
        if (atBarBoundary && reversePending) {
            activeReverse = pendingReverse;
            reversePending = false;
            // Snap playhead naar bar grens voor perfecte sync
            double currentBar = std::floor(playPos / framesPerBar);
            if (activeReverse) {
                // Naar reverse — spring naar einde van huidige bar
                playPos = (currentBar + 1.0) * framesPerBar - 1.0;
            } else {
                // Naar forward — spring naar begin van huidige bar
                playPos = currentBar * framesPerBar;
            }
        }

        // Playhead bewegen
        playPos += playSpeed;

        // Einde van cyclus — pas bar shift toe
        if (playPos >= totalFrames || playPos < 0.0) {
            if (barShiftPending) {
                double stepSize = (double)totalFrames / 8.0;
                activeLoopOffset = ((double)pendingBarShift - 1.0) * stepSize;
                barShiftPending = false;
            }
            if (playPos >= totalFrames) playPos -= totalFrames;
            if (playPos < 0.0) playPos += totalFrames;
        }

        // Lights
        lights[SYNC_LIGHT].setBrightness(sync ? 1.f : 0.f);
        lights[REVERSE_LIGHT].setBrightness(activeReverse ? 1.f : 0.f);
        lights[CUE_LIGHT].setBrightness(cue ? 1.f : 0.f);
    }



    std::string filePath = "";  // volledig pad

    json_t* dataToJson() override {
        json_t* root = json_object();
        json_object_set_new(root, "filePath", json_string(filePath.c_str()));
        return root;
    }

    void dataFromJson(json_t* root) override {
        json_t* fp = json_object_get(root, "filePath");
        if (fp) {
            std::string path = json_string_value(fp);
            if (!path.empty()) loadSample(path);
        }
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

struct LoopResetButton : SvgSwitch {
    LoopResetButton() {
        momentary = true;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/knob-reset-off.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/knob-reset-on.svg")));
    }
};

// Waveform display
struct ReelDisplay : Widget {
    Reel* module;
    ReelDisplay() {}

    void draw(const DrawArgs& args) override {
        // Achtergrond
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgFillColor(args.vg, nvgRGB(0x45, 0x45, 0x21));
        nvgFill(args.vg);

        // Border
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
        nvgStrokeColor(args.vg, nvgRGB(50, 50, 70));
        nvgStrokeWidth(args.vg, 1.0f);
        nvgStroke(args.vg);

        if (!module || !module->fileLoaded) {
            nvgFontSize(args.vg, 9.f);
            nvgFillColor(args.vg, nvgRGB(255, 255, 255));
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, box.size.x/2, box.size.y/2 - 6, "drop WAV file", nullptr);
            nvgText(args.vg, box.size.x/2, box.size.y/2 + 6, "or right-click", nullptr);
            return;
        }

        int w = (int)box.size.x;
        int h = (int)box.size.y;
        int frames = module->totalFrames;

        // Waveform
        nvgBeginPath(args.vg);
        nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
        nvgStrokeWidth(args.vg, 1.0f);
        for (int x = 0; x < w; x++) {
            int idx = (int)((float)x / w * frames);
            idx = clamp(idx, 0, frames - 1);
            float s = module->bufferL[idx];
            float y = h/2.f - s * (h/2.f) * 0.85f;
            if (x == 0) nvgMoveTo(args.vg, x, y);
            else nvgLineTo(args.vg, x, y);
        }
        nvgStroke(args.vg);

        // Middellijn
        nvgBeginPath(args.vg);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 255, 40));
        nvgStrokeWidth(args.vg, 0.5f);
        nvgMoveTo(args.vg, 0, h/2.f);
        nvgLineTo(args.vg, w, h/2.f);
        nvgStroke(args.vg);

        // Playhead
        float px = (float)(module->displayPos / frames) * w;
        px = clamp(px, 0.f, (float)w);
        nvgBeginPath(args.vg);
        nvgStrokeColor(args.vg, nvgRGBA(255, 255, 0, 230));
        nvgStrokeWidth(args.vg, 1.5f);
        nvgMoveTo(args.vg, px, 0);
        nvgLineTo(args.vg, px, h);
        nvgStroke(args.vg);

        // Bestandsnaam
        nvgFontSize(args.vg, 10.f);
        nvgFillColor(args.vg, nvgRGB(255, 255, 255));
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgText(args.vg, 3, 2, module->displayName.c_str(), nullptr);

        // BPM + Bars info
        if (module->fileBpm > 0.f) {
            char info[64];
            float bars = (module->params[Reel::BARS_PARAM].getValue() > 0.f)
                ? module->params[Reel::BARS_PARAM].getValue()
                : module->detectedBars;
            snprintf(info, sizeof(info), "%.0f BPM  %.0f bars", module->fileBpm, bars);
            nvgFontSize(args.vg, 10.f);
            nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
            nvgText(args.vg, 3, h - 2, info, nullptr);
        }
    }
};

struct ReelWidget : SubmitModuleWidget {
    ReelWidget(Reel* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Loop.svg")));

        // Waveform display
        ReelDisplay* disp = createWidget<ReelDisplay>(mm2px(Vec(7.199f, 13.856f)));
        disp->box.size = mm2px(Vec(51.191f, 27.477f));
        disp->module = module;
        addChild(disp);

        // Knoppen
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(19.002f, 60.229f)), module, Reel::BARS_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(39.291f, 60.229f)), module, Reel::BARSHIFT_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(19.002f, 83.833f)), module, Reel::BPM_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(mm2px(Vec(39.376f, 83.918f)), module, Reel::SPEED_PARAM));

        // Schakelaars
        addParam(createParamCentered<CKSS>(mm2px(Vec(55.657f, 63.360f)), module, Reel::SYNC_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(55.657f, 86.832f)), module, Reel::REVERSE_PARAM));
        addParam(createParamCentered<CKSS>(mm2px(Vec(55.657f, 110.431f)), module, Reel::CUE_PARAM));

        // Reset knop
        addParam(createParamCentered<LoopResetButton>(mm2px(Vec(38.985f, 103.789f)), module, Reel::RESET_PARAM));

        // LEDs
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(51.795f, 57.076f)), module, Reel::SYNC_LIGHT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(51.795f, 80.707f)), module, Reel::REVERSE_LIGHT));
        addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(51.795f, 104.286f)), module, Reel::CUE_LIGHT));

        // Inputs
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(55.657f, 50.566f)), module, Reel::CLOCK_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.866f, 116.074f)), module, Reel::TRIG_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.219f, 74.177f)), module, Reel::SPEED_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.885f, 50.566f)), module, Reel::BARS_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.219f, 50.566f)), module, Reel::BARSHIFT_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.885f, 74.177f)), module, Reel::BPM_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(55.657f, 74.177f)), module, Reel::REVERSE_CV_INPUT));

        // Outputs
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(9.885f, 116.074f)), module, Reel::MAIN_L_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.886f, 116.074f)), module, Reel::MAIN_R_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(55.657f, 97.877f)), module, Reel::CUE_OUTPUT));
    }

    void appendContextMenu(Menu* menu) override {
        Reel* module = dynamic_cast<Reel*>(this->module);
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("submitaudio.nl", "", []() {
            system::openBrowser(SUBMIT_URL);
        }));
        menu->addChild(createMenuItem("Manual", "", []() {
            system::openBrowser("https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/loop/");
        }));
        menu->addChild(createMenuItem("Load WAV...", "", [=]() {
            char* pathC = osdialog_file(OSDIALOG_OPEN, NULL, NULL, NULL);
            if (pathC) {
                module->loadSample(std::string(pathC));
                free(pathC);
            }
        }));
		SubmitModuleWidget::appendContextMenu(menu);
    }
};

Model* modelLoop = createModel<Reel, ReelWidget>("Loop");
