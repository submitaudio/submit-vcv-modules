// ZeroCoastV2.cpp - 0-Coast V2.05 for Submit
// No CTRL section
// Correct normalizations: Slope->Multiply, Contour->Dynamics
#include "plugin.hpp"

struct ZeroCoastV2 : Module {
    enum ParamId {
        PITCH_PARAM, FINE_PARAM, OVERTONE_PARAM, MULTIPLY_PARAM,
        RISE_PARAM, FALL_PARAM, TIME_PARAM, LOGEXP_PARAM, CYCLE_PARAM,
        ONSET_PARAM, SUSTAIN_PARAM, DECAY_PARAM, EXP_PARAM,
        BALANCE_PARAM, BALNC_PARAM, PARAMS_LEN
    };
    enum InputId {
        VOCT_INPUT, LINFM_INPUT, OVRTN_INPUT, MLTPL_INPUT,
        TRIG_INPUT, GATE_INPUT, SLOPE_INPUT, DCY_INPUT, CNTR_INPUT,
        DYNMC_INPUT, FVND_INPUT, OVRTN_BAL_INPUT, EXT_INPUT, INPUTS_LEN
    };
    enum OutputId {
        OUT1_OUTPUT, OUT2_OUTPUT, EOC_OUTPUT, EON_OUTPUT,
        CNTR_OUTPUT, CONTOUR_OUTPUT, LINEOUT_OUTPUT, DYNMC_OUTPUT, OUTPUTS_LEN
    };
    enum LightId { CYCLE_LIGHT, ONSET_LIGHT, LIGHTS_LEN };

    float phase=0.f;
    enum SlopeStage { IDLE, RISE, FALL };
    SlopeStage slopeStage=IDLE;
    float slopeValue=0.f, slopeTime=0.f;
    bool lastGate=false, lastTrig=false;
    float contourValue=0.f;
    dsp::PulseGenerator eocPulse, eonPulse, onsetPulse;

    ZeroCoastV2() {
        config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
        configParam(PITCH_PARAM,-4.f,4.f,-1.f,"Octave"," oct");
        paramQuantities[PITCH_PARAM]->snapEnabled=true;
        configParam(FINE_PARAM,-7.f,7.f,-7.f,"Tune"," st");
        paramQuantities[FINE_PARAM]->snapEnabled=true;
        configParam(OVERTONE_PARAM,0.f,1.f,0.4f,"Overtone");
        configParam(MULTIPLY_PARAM,0.f,1.f,0.5f,"Multiply");
        configParam(RISE_PARAM,0.001f,4.f,0.001f,"Rise"," s");
        configParam(FALL_PARAM,0.001f,8.f,0.73534f,"Fall"," s");
        configParam(TIME_PARAM,0.1f,4.f,0.4806f,"Time");
        configParam(LOGEXP_PARAM,-1.f,1.f,-0.3f,"Log:Exp");
        configSwitch(CYCLE_PARAM,0.f,1.f,0.f,"Cycle",{"Off","On"});
        configParam(ONSET_PARAM,0.f,1.f,0.25f,"Onset");
        configParam(SUSTAIN_PARAM,0.f,1.f,0.1f,"Sustain");
        configParam(DECAY_PARAM,0.01f,8.f,0.2f,"Decay"," s");
        configParam(EXP_PARAM,0.f,1.f,0.5f,"Exp");
        configParam(BALANCE_PARAM,-1.f,1.f,-0.3f,"Balance");
        configSwitch(BALNC_PARAM,0.f,1.f,0.f,"Balnc",{"Off","On"});
        configInput(VOCT_INPUT,"1V/Oct");
        configInput(LINFM_INPUT,"Lin FM");
        configInput(OVRTN_INPUT,"Overtone CV");
        configInput(MLTPL_INPUT,"Multiply CV (normalled to Slope)");
        configInput(TRIG_INPUT,"Trig");
        configInput(GATE_INPUT,"Gate");
        configInput(SLOPE_INPUT,"Slope CV");
        configInput(DCY_INPUT,"Decay CV");
        configInput(CNTR_INPUT,"Contour in");
        configInput(DYNMC_INPUT,"Dynamics CV (normalled to Contour)");
        configInput(FVND_INPUT,"Fundamental CV");
        configInput(OVRTN_BAL_INPUT,"Overtone bal CV");
        configInput(EXT_INPUT,"Ext In");
        configOutput(OUT1_OUTPUT,"Out 1 triangle");
        configOutput(OUT2_OUTPUT,"Out 2 (square)");
        configOutput(EOC_OUTPUT,"EOC");
        configOutput(EON_OUTPUT,"EON");
        configOutput(CNTR_OUTPUT,"Slope CV out");
        configOutput(CONTOUR_OUTPUT,"Contour CV out");
        configOutput(LINEOUT_OUTPUT,"Line Out");
    }

    float waveFolder(float x, float amount) {
        if (amount<0.001f) return x;
        float y=x*(1.f+amount*3.f);
        y=std::fmod(y+1.f,4.f);
        if (y<0.f) y+=4.f;
        if (y>2.f) y=4.f-y;
        return y-1.f;
    }
    float overtoneShaper(float x, float amount) {
        if (amount<0.001f) return x;
        float shaped=std::tanh((x+amount*0.3f)*(1.f+amount*2.f));
        return x*(1.f-amount)+shaped*amount;
    }
    float applyCurve(float x, float curve) {
        x=clamp(x,0.f,1.f);
        if (std::abs(curve)<1e-4f) return x;
        float denom=curve-2.f*curve*std::abs(x)+1.f;
        if (std::abs(denom)<1e-6f) return x;
        return (x-curve*x)/denom;
    }

    void process(const ProcessArgs& args) override {
        // Oscillator
        float pitchV=params[PITCH_PARAM].getValue()+params[FINE_PARAM].getValue()/12.f;
        if (inputs[VOCT_INPUT].isConnected()) pitchV+=inputs[VOCT_INPUT].getVoltage();
        if (inputs[LINFM_INPUT].isConnected()) pitchV+=inputs[LINFM_INPUT].getVoltage()*0.1f;
        float freq=clamp(dsp::FREQ_C4*std::pow(2.f,pitchV),1.f,20000.f);
        phase+=freq*args.sampleTime;
        if (phase>=1.f) phase-=1.f;
        float tri=(phase<0.5f)?(4.f*phase-1.f):(3.f-4.f*phase);
        // Square wave van dezelfde oscillator
        float square = (tri > 0.f) ? 1.f : -1.f;
        outputs[OUT1_OUTPUT].setVoltage(tri*5.f);
        outputs[OUT2_OUTPUT].setVoltage(square*5.f);

        // Overtone
        float ovrAmt=clamp(params[OVERTONE_PARAM].getValue()+(inputs[OVRTN_INPUT].isConnected()?inputs[OVRTN_INPUT].getVoltage()/10.f:0.f),0.f,1.f);
        float shaped=overtoneShaper(tri,ovrAmt);

        // Slope
        bool gate=inputs[GATE_INPUT].getVoltage()>1.f;
        bool trig=inputs[TRIG_INPUT].getVoltage()>1.f;
        bool cycle=params[CYCLE_PARAM].getValue()>0.5f;
        if ((gate&&!lastGate)||(trig&&!lastTrig)){slopeStage=RISE;slopeTime=0.f;onsetPulse.trigger(1e-3f);}
        if (inputs[GATE_INPUT].isConnected()&&!gate&&lastGate&&slopeStage==RISE){slopeStage=FALL;slopeTime=0.f;}
        lastGate=gate; lastTrig=trig;
        float tScale=params[TIME_PARAM].getValue();
        float riseT=params[RISE_PARAM].getValue()*tScale;
        float fallT=params[FALL_PARAM].getValue()*tScale;
        float logexp=clamp(params[LOGEXP_PARAM].getValue()+(inputs[SLOPE_INPUT].isConnected()?inputs[SLOPE_INPUT].getVoltage()/5.f:0.f),-1.f,1.f);
        if (slopeStage==RISE){
            slopeTime+=args.sampleTime;
            float t=clamp(slopeTime/riseT,0.f,1.f);
            slopeValue=applyCurve(t,logexp);
            if (t>=1.f){slopeValue=1.f;slopeStage=FALL;slopeTime=0.f;}
        } else if (slopeStage==FALL){
            slopeTime+=args.sampleTime;
            float t=clamp(slopeTime/fallT,0.f,1.f);
            slopeValue=1.f-applyCurve(t,-logexp);
            if (t>=1.f){slopeStage=IDLE;slopeValue=0.f;eocPulse.trigger(1e-3f);if(!gate)eonPulse.trigger(1e-3f);if(cycle){slopeStage=RISE;slopeTime=0.f;}}
        } else if (cycle){slopeStage=RISE;slopeTime=0.f;}
        outputs[EOC_OUTPUT].setVoltage(eocPulse.process(args.sampleTime)?10.f:0.f);
        outputs[EON_OUTPUT].setVoltage(eonPulse.process(args.sampleTime)?10.f:0.f);
        outputs[CNTR_OUTPUT].setVoltage(slopeValue*10.f);
        lights[CYCLE_LIGHT].setBrightness(slopeValue);
        lights[ONSET_LIGHT].setBrightness(onsetPulse.process(args.sampleTime)?1.f:0.f);

        // Multiply — normalled to Slope
        float mltCV=inputs[MLTPL_INPUT].isConnected()?inputs[MLTPL_INPUT].getVoltage()/10.f:slopeValue;
        float mltAmt=clamp(params[MULTIPLY_PARAM].getValue()+mltCV,0.f,1.f);
        float folded=waveFolder(shaped,mltAmt);
        outputs[OUT2_OUTPUT].setVoltage(folded*5.f);

        // Contour
        float cntrCV=inputs[CNTR_INPUT].isConnected()?inputs[CNTR_INPUT].getVoltage()/10.f:slopeValue;
        float sustain=params[SUSTAIN_PARAM].getValue();
        float decT=params[DECAY_PARAM].getValue()*(inputs[DCY_INPUT].isConnected()?clamp(inputs[DCY_INPUT].getVoltage()/5.f,0.1f,4.f):1.f);
        float expAmt=params[EXP_PARAM].getValue();
        float onset=params[ONSET_PARAM].getValue();
        float targetDyn=(cntrCV>sustain)?onset+(1.f-onset)*cntrCV:sustain*cntrCV/std::max(sustain,0.001f);
        float rate=clamp(args.sampleTime/std::max(decT,0.001f)*(1.f+expAmt*20.f),0.f,1.f);
        contourValue+=(targetDyn-contourValue)*rate;
        outputs[CONTOUR_OUTPUT].setVoltage(contourValue*10.f);

        // Dynamics — normalled to Slope
        float dynCV=inputs[DYNMC_INPUT].isConnected()?clamp(inputs[DYNMC_INPUT].getVoltage()/10.f,0.f,1.f):slopeValue;
        dynCV=clamp((dynCV-0.01f)/0.99f,0.f,1.f);

        // Balance
        float balance=params[BALANCE_PARAM].getValue();
        if (inputs[FVND_INPUT].isConnected()) balance-=inputs[FVND_INPUT].getVoltage()/5.f;
        if (inputs[OVRTN_BAL_INPUT].isConnected()) balance+=inputs[OVRTN_BAL_INPUT].getVoltage()/5.f;
        if (params[BALNC_PARAM].getValue()>0.5f) balance=1.f;
        balance=clamp(balance,-1.f,1.f);
        float extIn=inputs[EXT_INPUT].isConnected()?inputs[EXT_INPUT].getVoltage()/10.f:0.f;
        float audioMix=(tri*clamp(1.f-balance,0.f,1.f)+folded*clamp(1.f+balance,0.f,1.f))*0.5f+extIn;
        outputs[LINEOUT_OUTPUT].setVoltage(audioMix*dynCV*5.f);
    }
    json_t* dataToJson() override { return json_object(); }
    void dataFromJson(json_t* rootJ) override { (void)rootJ; }
};

struct ZCV2Label : Widget {
    std::string text; float fontSize; NVGcolor color;
    ZCV2Label(Vec pos,Vec size,std::string t,float fs,NVGcolor c):text(t),fontSize(fs),color(c){box.pos=pos;box.size=size;}
    void draw(const DrawArgs& args) override {
        nvgFontSize(args.vg,fontSize);nvgFontFaceId(args.vg,APP->window->uiFont->handle);
        nvgFillColor(args.vg,color);nvgTextAlign(args.vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
        nvgText(args.vg,box.size.x*.5f,box.size.y*.5f,text.c_str(),nullptr);
    }
};

struct ZeroCoastV2Widget : ModuleWidget {
    const NVGcolor GOLD=nvgRGB(255,200,0),WHITE=nvgRGB(255,255,255),PINK=nvgRGB(255,133,133),GRAY=nvgRGB(160,160,160),CYAN=nvgRGB(100,220,220);
    void L(float x,float y,float w,float h,const char* t,float fs,NVGcolor c){addChild(new ZCV2Label(Vec(x,y),Vec(w,h),t,fs,c));}

    ZeroCoastV2Widget(ZeroCoastV2* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance,"res/ZeroCoast.svg")));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x-2*RACK_GRID_WIDTH,RACK_GRID_HEIGHT-RACK_GRID_WIDTH)));

        L(140,1,200,13,"0-COAST V2.06",11.f,GOLD); L(175,13,130,9,"Submit",8.f,GOLD);

        // OSCILLATOR
        L(50,1,100,9,"OSCILLATOR",8.f,GOLD);
        L(50,20,80,9,"PITCH",9.f,WHITE);
        addParam(createParamCentered<RoundHugeBlackKnob>(Vec(80,72),module,ZeroCoastV2::PITCH_PARAM));
        L(50,110,36,8,"FINE",7.f,WHITE); addParam(createParamCentered<Trimpot>(Vec(68,128),module,ZeroCoastV2::FINE_PARAM));
        L(50,148,36,8,"1V/Oct",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(68,166),module,ZeroCoastV2::VOCT_INPUT));
        L(95,148,36,8,"LIN FM",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(113,166),module,ZeroCoastV2::LINFM_INPUT));
        L(50,310,36,8,"Out1",7.f,PINK); addOutput(createOutputCentered<PJ301MPort>(Vec(68,343),module,ZeroCoastV2::OUT1_OUTPUT));
        L(95,310,36,8,"Out2",7.f,PINK); addOutput(createOutputCentered<PJ301MPort>(Vec(113,343),module,ZeroCoastV2::OUT2_OUTPUT));

        // OVERTONE + MULTIPLY
        L(140,1,80,9,"OVERTONE",8.f,GOLD);
        addParam(createParamCentered<RoundBlackKnob>(Vec(170,40),module,ZeroCoastV2::OVERTONE_PARAM));
        L(140,55,36,8,"OVRTN",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(158,73),module,ZeroCoastV2::OVRTN_INPUT));
        L(140,90,80,9,"MULTIPLY",8.f,GOLD);
        addParam(createParamCentered<RoundBlackKnob>(Vec(170,130),module,ZeroCoastV2::MULTIPLY_PARAM));
        L(140,145,36,8,"MLTPL",7.f,CYAN); addInput(createInputCentered<PJ301MPort>(Vec(158,163),module,ZeroCoastV2::MLTPL_INPUT));
        L(140,177,60,7,"~Slope",6.f,CYAN);

        // SLOPE
        L(220,1,90,9,"SLOPE",8.f,GOLD);
        L(288,8,36,8,"CYCLE",7.f,WHITE); addParam(createParamCentered<CKSS>(Vec(306,25),module,ZeroCoastV2::CYCLE_PARAM));
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(306,40),module,ZeroCoastV2::CYCLE_LIGHT));
        L(220,18,36,9,"RISE",8.f,WHITE); addParam(createParamCentered<RoundBlackKnob>(Vec(238,48),module,ZeroCoastV2::RISE_PARAM));
        L(220,68,36,9,"FALL",8.f,WHITE); addParam(createParamCentered<RoundBlackKnob>(Vec(238,98),module,ZeroCoastV2::FALL_PARAM));
        L(220,118,36,9,"TIME",7.f,WHITE); addParam(createParamCentered<RoundSmallBlackKnob>(Vec(238,138),module,ZeroCoastV2::TIME_PARAM));
        L(264,118,44,9,"LOG:EXP",7.f,WHITE); addParam(createParamCentered<RoundSmallBlackKnob>(Vec(286,138),module,ZeroCoastV2::LOGEXP_PARAM));
        L(220,160,30,8,"TRIG",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(235,178),module,ZeroCoastV2::TRIG_INPUT));
        L(252,160,34,8,"SLOPE",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(269,178),module,ZeroCoastV2::SLOPE_INPUT));
        L(284,160,30,8,"GATE",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(299,178),module,ZeroCoastV2::GATE_INPUT));
        L(220,200,30,8,"EOC",7.f,PINK); addOutput(createOutputCentered<PJ301MPort>(Vec(235,218),module,ZeroCoastV2::EOC_OUTPUT));
        L(258,200,36,8,"SLOPE",7.f,CYAN); addOutput(createOutputCentered<PJ301MPort>(Vec(276,218),module,ZeroCoastV2::CNTR_OUTPUT));

        // CONTOUR
        L(315,1,85,9,"CONTOUR",8.f,GOLD);
        L(315,18,40,9,"ONSET",8.f,WHITE); addParam(createParamCentered<RoundBlackKnob>(Vec(335,48),module,ZeroCoastV2::ONSET_PARAM));
        addChild(createLightCentered<SmallLight<GreenLight>>(Vec(335,68),module,ZeroCoastV2::ONSET_LIGHT));
        L(315,80,44,9,"SUSTAIN",7.f,WHITE); addParam(createParamCentered<RoundSmallBlackKnob>(Vec(335,100),module,ZeroCoastV2::SUSTAIN_PARAM));
        L(315,118,36,9,"DECAY",8.f,WHITE); addParam(createParamCentered<RoundBlackKnob>(Vec(335,148),module,ZeroCoastV2::DECAY_PARAM));
        L(362,118,36,9,"EXP",8.f,WHITE); addParam(createParamCentered<RoundSmallBlackKnob>(Vec(379,148),module,ZeroCoastV2::EXP_PARAM));
        L(315,168,30,8,"DCY",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(330,186),module,ZeroCoastV2::DCY_INPUT));
        L(315,206,30,8,"EON",7.f,PINK); addOutput(createOutputCentered<PJ301MPort>(Vec(330,224),module,ZeroCoastV2::EON_OUTPUT));
        L(353,206,36,8,"CNTR",7.f,CYAN); addOutput(createOutputCentered<PJ301MPort>(Vec(371,224),module,ZeroCoastV2::CONTOUR_OUTPUT));
        L(315,244,36,8,"CNTR IN",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(333,262),module,ZeroCoastV2::CNTR_INPUT));

        // BALANCE
        L(400,1,80,9,"BALANCE",7.f,GOLD);
        L(445,12,30,8,"BALNC",6.f,GRAY); addParam(createParamCentered<CKSS>(Vec(460,28),module,ZeroCoastV2::BALNC_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(Vec(428,65),module,ZeroCoastV2::BALANCE_PARAM));
        L(400,80,24,8,"FVND",6.f,CYAN); L(435,80,30,8,"OVRTN",6.f,CYAN);
        L(400,92,30,8,"FVND",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(415,110),module,ZeroCoastV2::FVND_INPUT));
        L(435,92,30,8,"OVRTN",6.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(450,110),module,ZeroCoastV2::OVRTN_BAL_INPUT));
        L(400,128,36,8,"Ext In",7.f,WHITE); addInput(createInputCentered<PJ301MPort>(Vec(415,146),module,ZeroCoastV2::EXT_INPUT));
        L(400,164,36,8,"DYNMC",7.f,CYAN); addInput(createInputCentered<PJ301MPort>(Vec(415,182),module,ZeroCoastV2::DYNMC_INPUT));
        L(400,194,50,7,"~Contour",6.f,CYAN);
        L(400,310,44,8,"LINE OUT",7.f,PINK); addOutput(createOutputCentered<PJ301MPort>(Vec(418,343),module,ZeroCoastV2::LINEOUT_OUTPUT));
    }
};

Model* modelZeroCoastV2 = createModel<ZeroCoastV2, ZeroCoastV2Widget>("ZeroCoastV2");
