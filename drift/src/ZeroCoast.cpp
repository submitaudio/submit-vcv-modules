#include "plugin.hpp"

struct ZeroCoast : Module {
    enum ParamId {
        PITCH_PARAM,
        FINE_PARAM,
        OVERTONE_PARAM,
        MULTIPLY_PARAM,
        RISE_PARAM,
        FALL_PARAM,
        TIME_PARAM,
        LOGEXP_PARAM,
        CYCLE_PARAM,
        ONSET_PARAM,
        SUSTAIN_PARAM,
        DECAY_PARAM,
        EXP_PARAM,
        BALANCE_PARAM,
        BALNC_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        CLK_INPUT,
        VOCT_INPUT,
        LINFM_INPUT,
        OVRTN_INPUT,
        MLTPL_INPUT,
        TRIG_INPUT,
        GATE_INPUT,
        SLOPE_INPUT,
        DCY_INPUT,
        CNTR_INPUT,
        DYNMC_INPUT,
        FVND_INPUT,
        OVRTN_BAL_INPUT,
        EXT_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        CLK_OUTPUT,
        RND_OUTPUT,
        CV_OUTPUT,
        GATE_OUTPUT,
        SUM_OUTPUT,
        OUT1_OUTPUT,
        OUT2_OUTPUT,
        EOC_OUTPUT,
        EON_OUTPUT,
        CNTR_OUTPUT,
        LINEOUT_OUTPUT,
        DYNMC_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        CYCLE_LIGHT,
        ONSET_LIGHT,
        LIGHTS_LEN
    };

    float phase = 0.f;
    float clkGateTime = 0.f;
    bool clkGate = false;
    bool lastClkGate = false;
    float rndValue = 0.f;

    enum SlopeStage { IDLE, RISE, FALL };
    SlopeStage slopeStage = IDLE;
    float slopeValue = 0.f;
    float slopeTime = 0.f;
    bool lastGate = false;
    bool lastTrig = false;

    float contourValue = 0.f;

    dsp::PulseGenerator eocPulse;
    dsp::PulseGenerator eonPulse;
    dsp::PulseGenerator onsetPulse;

    static constexpr float CLK_GATE_LENGTH = 0.01f;

    ZeroCoast() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PITCH_PARAM,   -4.f,  4.f,  0.f,   "Pitch",   " V");
        configParam(FINE_PARAM,   -0.1f,  0.1f, 0.f,   "Fine");
        configParam(OVERTONE_PARAM, 0.f,  1.f,  0.f,   "Overtone");
        configParam(MULTIPLY_PARAM, 0.f,  1.f,  0.f,   "Multiply");
        configParam(RISE_PARAM,  0.001f,  4.f,  0.1f,  "Rise",    " s");
        configParam(FALL_PARAM,  0.001f,  8.f,  0.5f,  "Fall",    " s");
        configParam(TIME_PARAM,    0.1f,  4.f,  1.f,   "Time");
        configParam(LOGEXP_PARAM, -1.f,   1.f,  0.f,   "Log:Exp");
        configSwitch(CYCLE_PARAM,  0.f,   1.f,  0.f,   "Cycle", {"Off","On"});
        configParam(ONSET_PARAM,   0.f,   1.f,  0.7f,  "Onset");
        configParam(SUSTAIN_PARAM, 0.f,   1.f,  0.5f,  "Sustain");
        configParam(DECAY_PARAM,  0.01f,  8.f,  1.f,   "Decay",   " s");
        configParam(EXP_PARAM,     0.f,   1.f,  0.f,   "Exp");
        configParam(BALANCE_PARAM,-1.f,   1.f,  0.f,   "Balance");
        configSwitch(BALNC_PARAM,  0.f,   1.f,  0.f,   "Balnc", {"Off","On"});
        configInput(CLK_INPUT,       "Clock in");
        configInput(VOCT_INPUT,      "1V/Oct");
        configInput(LINFM_INPUT,     "Lin FM");
        configInput(OVRTN_INPUT,     "Overtone CV");
        configInput(MLTPL_INPUT,     "Multiply CV");
        configInput(TRIG_INPUT,      "Trig");
        configInput(GATE_INPUT,      "Gate");
        configInput(SLOPE_INPUT,     "Slope CV");
        configInput(DCY_INPUT,       "Decay CV");
        configInput(CNTR_INPUT,      "Contour in");
        configInput(DYNMC_INPUT,     "Dynamics CV");
        configInput(FVND_INPUT,      "Fundamental CV");
        configInput(OVRTN_BAL_INPUT, "Overtone bal CV");
        configInput(EXT_INPUT,       "Ext In");
        configOutput(CLK_OUTPUT,     "Clock out");
        configOutput(RND_OUTPUT,     "Random CV");
        configOutput(CV_OUTPUT,      "CV out");
        configOutput(GATE_OUTPUT,    "Gate out");
        configOutput(SUM_OUTPUT,     "Sum out");
        configOutput(OUT1_OUTPUT,    "Out 1 triangle");
        configOutput(OUT2_OUTPUT,    "Out 2 multiply");
        configOutput(EOC_OUTPUT,     "EOC");
        configOutput(EON_OUTPUT,     "EON");
        configOutput(CNTR_OUTPUT,    "Contour CV");
        configOutput(LINEOUT_OUTPUT, "Line Out");
        configOutput(DYNMC_OUTPUT,   "Dynamics out");
    }

    float waveFolder(float x, float amount) {
        if (amount < 0.001f) return x;
        float y = x * (1.f + amount * 3.f);
        y = std::fmod(y + 1.f, 4.f);
        if (y < 0.f) y += 4.f;
        if (y > 2.f) y = 4.f - y;
        return y - 1.f;
    }

    float overtoneShaper(float x, float amount) {
        if (amount < 0.001f) return x;
        float shaped = std::tanh((x + amount * 0.3f) * (1.f + amount * 2.f));
        return x * (1.f - amount) + shaped * amount;
    }

    float applyCurve(float x, float curve) {
        x = clamp(x, 0.f, 1.f);
        if (std::abs(curve) < 1e-4f) return x;
        float denom = curve - 2.f * curve * std::abs(x) + 1.f;
        if (std::abs(denom) < 1e-6f) return x;
        return (x - curve * x) / denom;
    }

    uint32_t rngState = 12345;
    float nextRandom() {
        rngState = rngState * 1664525u + 1013904223u;
        return (rngState >> 8) / 16777216.f;
    }

    void process(const ProcessArgs& args) override {
        // Clock
        bool clkTrig = false;
        if (inputs[CLK_INPUT].isConnected()) {
            bool clkHigh = inputs[CLK_INPUT].getVoltage() > 1.f;
            if (clkHigh && !lastClkGate) { clkTrig = true; clkGate = true; clkGateTime = 0.f; }
            lastClkGate = clkHigh;
        }
        if (clkGate) { clkGateTime += args.sampleTime; if (clkGateTime > CLK_GATE_LENGTH) clkGate = false; }
        if (clkTrig) rndValue = nextRandom() * 10.f - 5.f;
        outputs[CLK_OUTPUT].setVoltage(clkGate ? 10.f : 0.f);
        outputs[RND_OUTPUT].setVoltage(rndValue);
        outputs[GATE_OUTPUT].setVoltage(clkGate ? 10.f : 0.f);

        // Oscillator
        float pitchV = params[PITCH_PARAM].getValue() + params[FINE_PARAM].getValue();
        if (inputs[VOCT_INPUT].isConnected()) pitchV += inputs[VOCT_INPUT].getVoltage();
        if (inputs[LINFM_INPUT].isConnected()) pitchV += inputs[LINFM_INPUT].getVoltage() * 0.1f;
        float freq = clamp(dsp::FREQ_C4 * std::pow(2.f, pitchV), 1.f, 20000.f);
        phase += freq * args.sampleTime;
        if (phase >= 1.f) phase -= 1.f;
        float tri = (phase < 0.5f) ? (4.f * phase - 1.f) : (3.f - 4.f * phase);
        outputs[OUT1_OUTPUT].setVoltage(tri * 5.f);

        // Overtone + Multiply
        float ovrAmt = clamp(params[OVERTONE_PARAM].getValue() + (inputs[OVRTN_INPUT].isConnected() ? inputs[OVRTN_INPUT].getVoltage()/10.f : 0.f), 0.f, 1.f);
        float mltAmt = clamp(params[MULTIPLY_PARAM].getValue() + (inputs[MLTPL_INPUT].isConnected() ? inputs[MLTPL_INPUT].getVoltage()/10.f : 0.f), 0.f, 1.f);
        float folded = waveFolder(overtoneShaper(tri, ovrAmt), mltAmt);
        outputs[OUT2_OUTPUT].setVoltage(folded * 5.f);

        // Slope
        bool gate = inputs[GATE_INPUT].getVoltage() > 1.f;
        bool trig = inputs[TRIG_INPUT].getVoltage() > 1.f;
        bool cycle = params[CYCLE_PARAM].getValue() > 0.5f;
        if ((gate && !lastGate) || (trig && !lastTrig) || (clkTrig && !gate)) {
            slopeStage = RISE; slopeTime = 0.f; onsetPulse.trigger(1e-3f);
        }
        if (inputs[GATE_INPUT].isConnected() && !gate && lastGate && slopeStage == RISE) {
            slopeStage = FALL; slopeTime = 0.f;
        }
        lastGate = gate; lastTrig = trig;
        float tScale = params[TIME_PARAM].getValue();
        float riseT = params[RISE_PARAM].getValue() * tScale;
        float fallT = params[FALL_PARAM].getValue() * tScale;
        float logexp = clamp(params[LOGEXP_PARAM].getValue() + (inputs[SLOPE_INPUT].isConnected() ? inputs[SLOPE_INPUT].getVoltage()/5.f : 0.f), -1.f, 1.f);
        if (slopeStage == RISE) {
            slopeTime += args.sampleTime;
            float t = clamp(slopeTime / riseT, 0.f, 1.f);
            slopeValue = applyCurve(t, logexp);
            if (t >= 1.f) { slopeStage = FALL; slopeTime = 0.f; }
        } else if (slopeStage == FALL) {
            slopeTime += args.sampleTime;
            float t = clamp(slopeTime / fallT, 0.f, 1.f);
            slopeValue = 1.f - applyCurve(t, -logexp);
            if (t >= 1.f) {
                slopeStage = IDLE; slopeValue = 0.f;
                eocPulse.trigger(1e-3f);
                if (!gate) eonPulse.trigger(1e-3f);
                if (cycle) { slopeStage = RISE; slopeTime = 0.f; }
            }
        } else if (cycle) {
            slopeStage = RISE; slopeTime = 0.f;
        }
        outputs[EOC_OUTPUT].setVoltage(eocPulse.process(args.sampleTime) ? 10.f : 0.f);
        outputs[EON_OUTPUT].setVoltage(eonPulse.process(args.sampleTime) ? 10.f : 0.f);
        lights[CYCLE_LIGHT].setBrightness(slopeValue);
        lights[ONSET_LIGHT].setBrightness(onsetPulse.process(args.sampleTime) ? 1.f : 0.f);

        // Contour / Dynamics
        float cntrCV = inputs[CNTR_INPUT].isConnected() ? inputs[CNTR_INPUT].getVoltage()/10.f : slopeValue;
        float sustain = params[SUSTAIN_PARAM].getValue();
        float decT = params[DECAY_PARAM].getValue() * (inputs[DCY_INPUT].isConnected() ? clamp(inputs[DCY_INPUT].getVoltage()/5.f, 0.1f, 4.f) : 1.f);
        float expAmt = params[EXP_PARAM].getValue();
        float onset = params[ONSET_PARAM].getValue();
        float targetDyn = (cntrCV > sustain) ? onset + (1.f-onset)*cntrCV : sustain * cntrCV / std::max(sustain, 0.001f);
        float rate = clamp(args.sampleTime / std::max(decT, 0.001f) * (1.f + expAmt * 9.f), 0.f, 1.f);
        contourValue += (targetDyn - contourValue) * rate;
        float dynCV = clamp(contourValue + (inputs[DYNMC_INPUT].isConnected() ? inputs[DYNMC_INPUT].getVoltage()/10.f : 0.f), 0.f, 1.f);
        outputs[CNTR_OUTPUT].setVoltage(contourValue * 10.f);
        outputs[DYNMC_OUTPUT].setVoltage(dynCV * 10.f);

        // Balance
        float balance = params[BALANCE_PARAM].getValue();
        if (inputs[FVND_INPUT].isConnected()) balance -= inputs[FVND_INPUT].getVoltage()/5.f;
        if (inputs[OVRTN_BAL_INPUT].isConnected()) balance += inputs[OVRTN_BAL_INPUT].getVoltage()/5.f;
        if (params[BALNC_PARAM].getValue() > 0.5f) balance = 1.f;
        balance = clamp(balance, -1.f, 1.f);
        float extIn = inputs[EXT_INPUT].isConnected() ? inputs[EXT_INPUT].getVoltage()/10.f : 0.f;
        float audioMix = (tri * clamp(1.f-balance,0.f,1.f) + folded * clamp(1.f+balance,0.f,1.f)) * 0.5f + extIn;
        outputs[LINEOUT_OUTPUT].setVoltage(audioMix * dynCV * 5.f);

        // SUM
        float cv1 = inputs[VOCT_INPUT].isConnected() ? inputs[VOCT_INPUT].getVoltage() : 0.f;
        float cv2 = inputs[LINFM_INPUT].isConnected() ? inputs[LINFM_INPUT].getVoltage() : 0.f;
        outputs[SUM_OUTPUT].setVoltage(clamp((cv1+cv2+rndValue)/3.f, -10.f, 10.f));
        outputs[CV_OUTPUT].setVoltage(cv1);
    }

    json_t* dataToJson() override { return json_object(); }
    void dataFromJson(json_t* rootJ) override { (void)rootJ; }
};

struct ZCLabel : Widget {
    std::string text; float fontSize; NVGcolor color;
    ZCLabel(Vec pos, Vec size, std::string t, float fs, NVGcolor c) : text(t), fontSize(fs), color(c) { box.pos=pos; box.size=size; }
    void draw(const DrawArgs& args) override {
        nvgFontSize(args.vg, fontSize); nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgFillColor(args.vg, color); nvgTextAlign(args.vg, NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
        nvgText(args.vg, box.size.x*.5f, box.size.y*.5f, text.c_str(), nullptr);
    }
};

struct ZeroCoastWidget : ModuleWidget {
    const NVGcolor GOLD=nvgRGB(255,200,0), WHITE=nvgRGB(255,255,255), PINK=nvgRGB(255,133,133), GRAY=nvgRGB(160,160,160);
    void L(float x,float y,float w,float h,const char* t,float fs,NVGcolor c){addChild(new ZCLabel(Vec(x,y),Vec(w,h),t,fs,c));}

    ZeroCoastWidget(ZeroCoast* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/ZeroCoast.svg")));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));

        L(160,1,160,13,"0-COAST",12.f,GOLD); L(175,13,130,9,"Submit",8.f,GOLD);

        // CTRL
        L(10,1,70,9,"CTRL",8.f,GOLD);
        L(10,14,30,8,"CLK",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(25,32),module,ZeroCoast::CLK_INPUT));
        L(48,14,30,8,"CLK",7.f,PINK);  addOutput(createOutputCentered<PJ301MPort>(Vec(63,32),module,ZeroCoast::CLK_OUTPUT));
        L(10,48,30,8,"RND",7.f,PINK);  addOutput(createOutputCentered<PJ301MPort>(Vec(25,66),module,ZeroCoast::RND_OUTPUT));
        L(48,48,30,8,"SUM",7.f,PINK);  addOutput(createOutputCentered<PJ301MPort>(Vec(63,66),module,ZeroCoast::SUM_OUTPUT));
        L(10,82,30,8,"CV",7.f,PINK);   addOutput(createOutputCentered<PJ301MPort>(Vec(25,100),module,ZeroCoast::CV_OUTPUT));
        L(48,82,30,8,"Gate",7.f,PINK); addOutput(createOutputCentered<PJ301MPort>(Vec(63,100),module,ZeroCoast::GATE_OUTPUT));

        // OSCILLATOR
        L(90,1,75,9,"OSCILLATOR",7.f,GOLD);
        L(90,20,75,9,"PITCH",9.f,WHITE);
        addParam(createParamCentered<RoundHugeBlackKnob>(Vec(128,72),module,ZeroCoast::PITCH_PARAM));
        L(90,110,36,8,"FINE",7.f,WHITE);
        addParam(createParamCentered<Trimpot>(Vec(108,128),module,ZeroCoast::FINE_PARAM));
        L(90,148,36,8,"1V/Oct",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(108,166),module,ZeroCoast::VOCT_INPUT));
        L(132,148,36,8,"LIN FM",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(150,166),module,ZeroCoast::LINFM_INPUT));
        L(90,310,36,8,"Out1",7.f,PINK);  addOutput(createOutputCentered<PJ301MPort>(Vec(108,343),module,ZeroCoast::OUT1_OUTPUT));
        L(132,310,36,8,"Out2",7.f,PINK); addOutput(createOutputCentered<PJ301MPort>(Vec(150,343),module,ZeroCoast::OUT2_OUTPUT));

        // OVERTONE + MULTIPLY
        L(170,1,80,9,"OVERTONE",8.f,GOLD);
        addParam(createParamCentered<RoundBlackKnob>(Vec(200,40),module,ZeroCoast::OVERTONE_PARAM));
        L(170,55,36,8,"OVRTN",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(188,73),module,ZeroCoast::OVRTN_INPUT));
        L(170,90,80,9,"MULTIPLY",8.f,GOLD);
        addParam(createParamCentered<RoundBlackKnob>(Vec(200,130),module,ZeroCoast::MULTIPLY_PARAM));
        L(170,145,36,8,"MLTPL",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(188,163),module,ZeroCoast::MLTPL_INPUT));

        // SLOPE
        L(250,1,90,9,"SLOPE",8.f,GOLD);
        L(318,8,36,8,"CYCLE",7.f,WHITE); addParam(createParamCentered<CKSS>(Vec(336,25),module,ZeroCoast::CYCLE_PARAM));
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(336,40),module,ZeroCoast::CYCLE_LIGHT));
        L(250,18,36,9,"RISE",8.f,WHITE); addParam(createParamCentered<RoundBlackKnob>(Vec(268,48),module,ZeroCoast::RISE_PARAM));
        L(250,68,36,9,"FALL",8.f,WHITE); addParam(createParamCentered<RoundBlackKnob>(Vec(268,98),module,ZeroCoast::FALL_PARAM));
        L(250,118,36,9,"TIME",7.f,WHITE); addParam(createParamCentered<RoundSmallBlackKnob>(Vec(268,138),module,ZeroCoast::TIME_PARAM));
        L(294,118,44,9,"LOG:EXP",7.f,WHITE); addParam(createParamCentered<RoundSmallBlackKnob>(Vec(316,138),module,ZeroCoast::LOGEXP_PARAM));
        L(250,160,30,8,"TRIG",7.f,WHITE);  addInput(createInputCentered<PJ301MPort>(Vec(265,178),module,ZeroCoast::TRIG_INPUT));
        L(282,160,34,8,"SLOPE",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(299,178),module,ZeroCoast::SLOPE_INPUT));
        L(314,160,30,8,"GATE",7.f,WHITE);  addInput(createInputCentered<PJ301MPort>(Vec(329,178),module,ZeroCoast::GATE_INPUT));
        L(250,200,30,8,"EOC",7.f,PINK); addOutput(createOutputCentered<PJ301MPort>(Vec(265,218),module,ZeroCoast::EOC_OUTPUT));

        // CONTOUR
        L(345,1,85,9,"CONTOUR",8.f,GOLD);
        L(345,18,40,9,"ONSET",8.f,WHITE); addParam(createParamCentered<RoundBlackKnob>(Vec(365,48),module,ZeroCoast::ONSET_PARAM));
        addChild(createLightCentered<SmallLight<GreenLight>>(Vec(365,68),module,ZeroCoast::ONSET_LIGHT));
        L(345,80,44,9,"SUSTAIN",7.f,WHITE); addParam(createParamCentered<RoundSmallBlackKnob>(Vec(365,100),module,ZeroCoast::SUSTAIN_PARAM));
        L(345,118,36,9,"DECAY",8.f,WHITE); addParam(createParamCentered<RoundBlackKnob>(Vec(365,148),module,ZeroCoast::DECAY_PARAM));
        L(395,118,36,9,"EXP",8.f,WHITE); addParam(createParamCentered<RoundSmallBlackKnob>(Vec(412,148),module,ZeroCoast::EXP_PARAM));
        L(345,168,30,8,"DCY",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(360,186),module,ZeroCoast::DCY_INPUT));
        L(345,206,30,8,"EON",7.f,PINK);  addOutput(createOutputCentered<PJ301MPort>(Vec(360,224),module,ZeroCoast::EON_OUTPUT));
        L(383,206,36,8,"CNTR",7.f,PINK); addOutput(createOutputCentered<PJ301MPort>(Vec(401,224),module,ZeroCoast::CNTR_OUTPUT));
        L(345,244,36,8,"CNTR IN",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(363,262),module,ZeroCoast::CNTR_INPUT));

        // BALANCE
        L(435,1,50,9,"BALANCE",7.f,GOLD);
        L(455,12,30,8,"BALNC",6.f,GRAY); addParam(createParamCentered<LEDButton>(Vec(470,28),module,ZeroCoast::BALNC_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(Vec(458,65),module,ZeroCoast::BALANCE_PARAM));
        L(435,80,24,8,"FVND",6.f,GRAY); L(465,80,30,8,"OVRTN",6.f,GRAY);
        L(435,92,30,8,"FVND",7.f,WHITE);  addInput(createInputCentered<PJ301MPort>(Vec(450,110),module,ZeroCoast::FVND_INPUT));
        L(462,92,30,8,"OVRTN",6.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(477,110),module,ZeroCoast::OVRTN_BAL_INPUT));
        L(435,128,36,8,"Ext In",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(450,146),module,ZeroCoast::EXT_INPUT));
        L(435,164,36,8,"DYNMC",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(450,182),module,ZeroCoast::DYNMC_INPUT));
        L(435,310,44,8,"LINE OUT",7.f,PINK); addOutput(createOutputCentered<PJ301MPort>(Vec(452,343),module,ZeroCoast::LINEOUT_OUTPUT));
        L(462,310,36,8,"DYNMC",7.f,PINK);   addOutput(createOutputCentered<PJ301MPort>(Vec(477,343),module,ZeroCoast::DYNMC_OUTPUT));
    }
};

Model* modelZeroCoast = createModel<ZeroCoast, ZeroCoastWidget>("ZeroCoast");
