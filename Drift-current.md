#include "plugin.hpp"

struct Drift : Module {
    enum ParamId {
        PITCH_PARAM, FINE_PARAM, OVERTONE_PARAM, MULTIPLY_PARAM,
        RISE_PARAM, FALL_PARAM, TIME_PARAM, LOGEXP_PARAM, CYCLE_PARAM,
        ONSET_PARAM, SUSTAIN_PARAM, DECAY_PARAM, EXP_PARAM,
        BALANCE_PARAM, BALNC_PARAM, PARAMS_LEN
    };
    enum InputId {
        VOCT_INPUT, LINFM_INPUT, OVRTN_INPUT, MLTPL_INPUT,
        TRIG_INPUT, GATE_INPUT, SLOPE_INPUT, DCY_INPUT, CNTR_INPUT,
        DYNMC_INPUT, FVND_INPUT, OVRTN_BAL_INPUT, EXT_INPUT, TIMBRE_INPUT, INPUTS_LEN
    };
    enum OutputId {
        OUT1_OUTPUT, OUT2_OUTPUT, EOC_OUTPUT, EON_OUTPUT,
        CNTR_OUTPUT, CONTOUR_OUTPUT, LINEOUT_OUTPUT, OUTPUTS_LEN
    };
    enum LightId { CYCLE_LIGHT, ONSET_LIGHT, TIMBRE_LIGHT, LIGHTS_LEN };

    float phase=0.f;
    enum SlopeStage { IDLE, RISE, FALL };
    SlopeStage slopeStage=IDLE;
    float slopeValue=0.f, slopeTime=0.f;
    bool lastGate=false, lastTrig=false;
    float contourValue=0.f;
    dsp::PulseGenerator eocPulse, eonPulse, onsetPulse;

    Drift() {
        config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
        configParam(PITCH_PARAM,-4.f,4.f,-2.f,"Octave"," oct");
        paramQuantities[PITCH_PARAM]->snapEnabled=true;
        configParam(FINE_PARAM,-7.f,7.f,-7.f,"Tune"," st");
        paramQuantities[FINE_PARAM]->snapEnabled=true;
        configParam(OVERTONE_PARAM,0.f,1.f,0.4f,"Overtone");
        configParam(MULTIPLY_PARAM,0.f,1.f,0.5f,"Multiply");
        configParam(RISE_PARAM,0.001f,4.f,0.001f,"Rise"," s");
        configParam(FALL_PARAM,0.001f,8.f,0.73534f,"Fall"," s");
        configParam(TIME_PARAM,0.1f,4.f,0.4806f,"Time");
        configParam(LOGEXP_PARAM,-1.f,1.f,-0.3f,"Curve");
        configSwitch(CYCLE_PARAM,0.f,1.f,0.f,"Cycle",{"Off","On"});
        configParam(ONSET_PARAM,0.f,1.f,0.25f,"Onset");
        configParam(SUSTAIN_PARAM,0.f,1.f,0.1f,"Sustain");
        configParam(DECAY_PARAM,0.01f,8.f,0.2f,"Decay"," s");
        configParam(EXP_PARAM,0.f,1.f,0.5f,"Exp");
        configParam(BALANCE_PARAM,-1.f,1.f,-0.3f,"Timbre");
        configSwitch(BALNC_PARAM,0.f,1.f,0.f,"Timbre",{"Off","On"});
        configInput(VOCT_INPUT,"V/OCT");
        configInput(LINFM_INPUT,"FM");
        configInput(OVRTN_INPUT,"OVR");
        configInput(MLTPL_INPUT,"MLT");
        configInput(TRIG_INPUT,"TRIG");
        configInput(GATE_INPUT,"GATE");
        configInput(SLOPE_INPUT,"SLP");
        configInput(DCY_INPUT,"DCY");
        configInput(CNTR_INPUT,"CTR");
        configInput(DYNMC_INPUT,"DYN");
        configInput(FVND_INPUT,"Fundamental CV");
        configInput(OVRTN_BAL_INPUT,"Overtone bal CV");
        configInput(EXT_INPUT,"Ext In");
        configInput(TIMBRE_INPUT,"Timbre CV");
        configOutput(OUT1_OUTPUT,"TRI");
        configOutput(OUT2_OUTPUT,"SQR");
        configOutput(EOC_OUTPUT,"EOC");
        configOutput(EON_OUTPUT,"EON");
        configOutput(CNTR_OUTPUT,"SLP");
        configOutput(CONTOUR_OUTPUT,"ENV");
        configOutput(LINEOUT_OUTPUT,"LINE OUT");
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
        float pitchV=params[PITCH_PARAM].getValue()+params[FINE_PARAM].getValue()/12.f;
        if (inputs[VOCT_INPUT].isConnected()) pitchV+=inputs[VOCT_INPUT].getVoltage();
        if (inputs[LINFM_INPUT].isConnected()) pitchV+=inputs[LINFM_INPUT].getVoltage()*0.1f;
        float freq=clamp(dsp::FREQ_C4*std::pow(2.f,pitchV),1.f,20000.f);
        phase+=freq*args.sampleTime;
        if (phase>=1.f) phase-=1.f;
        float tri=(phase<0.5f)?(4.f*phase-1.f):(3.f-4.f*phase);
        float square=(tri>0.f)?1.f:-1.f;
        outputs[OUT1_OUTPUT].setVoltage(tri*5.f);
        outputs[OUT2_OUTPUT].setVoltage(square*5.f);

        float ovrAmt=clamp(params[OVERTONE_PARAM].getValue()+(inputs[OVRTN_INPUT].isConnected()?inputs[OVRTN_INPUT].getVoltage()/10.f:0.f),0.f,1.f);
        float shaped=overtoneShaper(tri,ovrAmt);

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
        lights[TIMBRE_LIGHT].setBrightness(params[BALNC_PARAM].getValue());
        lights[ONSET_LIGHT].setBrightness(onsetPulse.process(args.sampleTime)?1.f:0.f);

        float mltCV=inputs[MLTPL_INPUT].isConnected()?inputs[MLTPL_INPUT].getVoltage()/10.f:slopeValue;
        float mltAmt=clamp(params[MULTIPLY_PARAM].getValue()+mltCV,0.f,1.f);
        float folded=waveFolder(shaped,mltAmt);

        float cntrCV=inputs[CNTR_INPUT].isConnected()?inputs[CNTR_INPUT].getVoltage()/10.f:slopeValue;
        float sustain=params[SUSTAIN_PARAM].getValue();
        float decT=params[DECAY_PARAM].getValue()*(inputs[DCY_INPUT].isConnected()?clamp(inputs[DCY_INPUT].getVoltage()/5.f,0.1f,4.f):1.f);
        float expAmt=params[EXP_PARAM].getValue();
        float onset=params[ONSET_PARAM].getValue();
        float targetDyn=(cntrCV>sustain)?onset+(1.f-onset)*cntrCV:sustain*cntrCV/std::max(sustain,0.001f);
        float rate=clamp(args.sampleTime/std::max(decT,0.001f)*(1.f+expAmt*20.f),0.f,1.f);
        contourValue+=(targetDyn-contourValue)*rate;
        outputs[CONTOUR_OUTPUT].setVoltage(contourValue*10.f);

        float dynCV=inputs[DYNMC_INPUT].isConnected()?clamp(inputs[DYNMC_INPUT].getVoltage()/10.f,0.f,1.f):slopeValue;
        dynCV=clamp((dynCV-0.01f)/0.99f,0.f,1.f);

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


struct DriftKnobLarge : SvgKnob {
    DriftKnobLarge() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/DriftKnobLarge.svg")));
        shadow->opacity = 0.f;
    }
};
struct DriftKnobMedium : SvgKnob {
    DriftKnobMedium() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/DriftKnobMedium.svg")));
        shadow->opacity = 0.f;
    }
};
struct DriftKnobSmall : SvgKnob {
    DriftKnobSmall() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/DriftKnobSmall.svg")));
        shadow->opacity = 0.f;
    }
};
struct DriftWidget : ModuleWidget {
    DriftWidget(Drift* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance,"res/Drift.svg")));
                                
        // OSCILLATOR
        addParam(createParamCentered<DriftKnobLarge>(Vec(58.5f,129.7f),module,Drift::PITCH_PARAM));
        addParam(createParamCentered<DriftKnobSmall>(Vec(85.9f,224.5f),module,Drift::FINE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(39.3f,251.8f),module,Drift::VOCT_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(39.3f,343.7f),module,Drift::OUT1_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(78.7f,343.8f),module,Drift::OUT2_OUTPUT));

        // OVERTONE + MULTIPLY
        addParam(createParamCentered<DriftKnobMedium>(Vec(166.2f,129.5f),module,Drift::OVERTONE_PARAM));
        addParam(createParamCentered<DriftKnobMedium>(Vec(166.1f,215.7f),module,Drift::MULTIPLY_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(144.4f,343.8f),module,Drift::OVRTN_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(184.0f,343.7f),module,Drift::LINFM_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(184.1f,297.3f),module,Drift::MLTPL_INPUT));

        // SLOPE
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(263.3f,86.f),module,Drift::CYCLE_LIGHT));
        addParam(createParamCentered<CKSS>(Vec(229.6f,106.9f),module,Drift::CYCLE_PARAM));
        addParam(createParamCentered<DriftKnobSmall>(Vec(279.2f,113.8f),module,Drift::RISE_PARAM));
        addParam(createParamCentered<DriftKnobSmall>(Vec(279.2f,170.7f),module,Drift::FALL_PARAM));
        addParam(createParamCentered<DriftKnobSmall>(Vec(279.1f,226.6f),module,Drift::TIME_PARAM));
        addParam(createParamCentered<DriftKnobSmall>(Vec(279.1f,283.2f),module,Drift::LOGEXP_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(234.f,159.4f),module,Drift::TRIG_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(234.f,343.7f),module,Drift::GATE_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(274.6f,343.8f),module,Drift::SLOPE_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(233.9f,205.4f),module,Drift::EON_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(233.9f,251.2f),module,Drift::CNTR_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(234.0f,297.6f),module,Drift::EOC_OUTPUT));

        // CONTOUR
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(356.4f,86.f),module,Drift::ONSET_LIGHT));
        addParam(createParamCentered<DriftKnobSmall>(Vec(373.9f,113.6f),module,Drift::ONSET_PARAM));
        addParam(createParamCentered<DriftKnobSmall>(Vec(373.9f,170.7f),module,Drift::SUSTAIN_PARAM));
        addParam(createParamCentered<DriftKnobSmall>(Vec(373.9f,226.4f),module,Drift::DECAY_PARAM));
        addParam(createParamCentered<DriftKnobSmall>(Vec(373.9f,283.1f),module,Drift::EXP_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(328.1f,252.7f),module,Drift::DCY_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(328.0f,297.4f),module,Drift::CNTR_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(328.0f,343.8f),module,Drift::DYNMC_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(367.2f,343.9f),module,Drift::CONTOUR_OUTPUT));

        // BALANCE
        addParam(createParamCentered<DriftKnobMedium>(Vec(445.9f,129.3f),module,Drift::BALANCE_PARAM));
        addParam(createParamCentered<CKSS>(Vec(422.9f,199.f),module,Drift::BALNC_PARAM));
        addOutput(createOutputCentered<PJ301MPort>(Vec(463.6f,343.8f),module,Drift::LINEOUT_OUTPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(425.1f,343.8f),module,Drift::TIMBRE_INPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(419.1f,177.8f),module,Drift::TIMBRE_LIGHT));
    }
};

Model* modelDrift = createModel<Drift, DriftWidget>("Drift");
