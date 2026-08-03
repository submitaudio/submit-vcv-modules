// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// Licensed under GPL v3 — see LICENSE file for details
// https://github.com/submitaudio/submit-vcv-modules

#include "plugin.hpp"

struct Drift13 : Module {
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
    enum SlopeStage { IDLE, RISE, SUSTAIN, FALL };
    SlopeStage slopeStage=IDLE;
    float slopeValue=0.f, slopeTime=0.f, fallStartValue=1.f;
    float smoothedDynCV=0.f;
    bool lastGate=false, lastTrig=false;
    float contourValue=0.f;
    dsp::PulseGenerator eocPulse, eonPulse, onsetPulse;

    Drift13() {
        config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
        configParam(PITCH_PARAM,-4.f,4.f,-1.f,"Octave"," oct");
        paramQuantities[PITCH_PARAM]->snapEnabled=true;
        configParam(FINE_PARAM,-7.f,7.f,0.f,"Tune"," st");
        paramQuantities[FINE_PARAM]->snapEnabled=true;
        configParam(OVERTONE_PARAM,0.f,1.f,0.43133f,"Overtone");
        configParam(MULTIPLY_PARAM,0.f,1.f,0.5f,"Multiply");
        configParam(RISE_PARAM,0.001f,4.f,2.1547f,"Rise"," s");
        configParam(FALL_PARAM,0.001f,8.f,0.001f,"Fall"," s");
        configParam(TIME_PARAM,0.1f,4.f,3.2529f,"Time");
        configParam(LOGEXP_PARAM,-1.f,1.f,-0.59518f,"Curve");
        configSwitch(CYCLE_PARAM,0.f,1.f,0.f,"Cycle",{"Off","On"});
        configParam(ONSET_PARAM,0.f,1.f,0.f,"Onset");
        configParam(SUSTAIN_PARAM,0.f,1.f,0.31084f,"Sustain");
        configParam(DECAY_PARAM,0.001f,8.f,0.01f,"Decay"," s");
        configParam(EXP_PARAM,0.f,1.f,0.83976f,"Exp");
        configParam(BALANCE_PARAM,0.f,1.f,0.61446f,"Timbre");
        configSwitch(BALNC_PARAM,0.f,1.f,1.f,"Timbre",{"Off","On"});
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
        float sustainLevel = params[SUSTAIN_PARAM].getValue();
        float sustain = sustainLevel; // alias voor gebruik in slope fase

        if ((gate&&!lastGate)||(trig&&!lastTrig)){slopeStage=RISE;slopeTime=0.f;onsetPulse.trigger(0.05f);}

        // Gate release: start FALL vanaf huidige waarde
        if (inputs[GATE_INPUT].isConnected()&&!gate&&lastGate&&(slopeStage==RISE||slopeStage==SUSTAIN||slopeValue>0.f)){
            fallStartValue=slopeValue;slopeStage=FALL;slopeTime=0.f;
        }

        lastGate=gate; lastTrig=trig;
        float tScale=params[TIME_PARAM].getValue();
        float riseT=params[RISE_PARAM].getValue()*tScale;
        float fallRaw=params[FALL_PARAM].getValue();
        float fallT=0.001f*std::pow(8000.f,fallRaw/8.f)*tScale;
        float logexp=clamp(params[LOGEXP_PARAM].getValue()+(inputs[SLOPE_INPUT].isConnected()?inputs[SLOPE_INPUT].getVoltage()/5.f:0.f),-1.f,1.f);

        if (slopeStage==RISE){
            slopeTime+=args.sampleTime;
            float t=clamp(slopeTime/riseT,0.f,1.f);
            slopeValue=applyCurve(t,logexp);
            if (t>=1.f){
                slopeValue=1.f;
                // Als SUSTAIN > 0 en gate nog hoog → sustain fase
                if (sustainLevel > 0.001f && gate && inputs[GATE_INPUT].isConnected()) {
                    slopeStage=SUSTAIN;
                } else {
                    fallStartValue=slopeValue;slopeStage=FALL;slopeTime=0.f;
                }
            }
        } else if (slopeStage==SUSTAIN){
            // B versie: blijf op sustain niveau zolang gate hoog is
            slopeValue = sustain;
            // Als geen gate connected → val gewoon
            if (!inputs[GATE_INPUT].isConnected()) {
                fallStartValue=slopeValue;slopeStage=FALL;slopeTime=0.f;
            }
        } else if (slopeStage==FALL){
            slopeTime+=args.sampleTime;
            float t=clamp(slopeTime/fallT,0.f,1.f);
            slopeValue=fallStartValue*(1.f-applyCurve(t,-logexp));
            if (t>=1.f){eocPulse.trigger(1e-3f);if(!gate)eonPulse.trigger(1e-3f);if(cycle){slopeStage=RISE;slopeTime=0.f;}else{slopeStage=IDLE;slopeValue=0.f;}}
        } else if (cycle){slopeStage=RISE;slopeTime=0.f;}
        outputs[EOC_OUTPUT].setVoltage(eocPulse.process(args.sampleTime)?10.f:0.f);
        outputs[EON_OUTPUT].setVoltage(eonPulse.process(args.sampleTime)?10.f:0.f);
        outputs[CNTR_OUTPUT].setVoltage(slopeValue*10.f);
        lights[CYCLE_LIGHT].setBrightness(params[CYCLE_PARAM].getValue() > 0.5f ? slopeValue : 0.f);
        lights[TIMBRE_LIGHT].setBrightness(params[BALNC_PARAM].getValue());
        lights[ONSET_LIGHT].setBrightness(onsetPulse.process(args.sampleTime)?1.f:0.f);

        float mltCV=inputs[MLTPL_INPUT].isConnected()?inputs[MLTPL_INPUT].getVoltage()/10.f:slopeValue;
        float mltAmt=clamp(params[MULTIPLY_PARAM].getValue()+mltCV,0.f,1.f);
        float folded=waveFolder(shaped,mltAmt);

        // CONTOUR
        // ONSET = attack tijd
        // SUSTAIN = niveau tijdens gate
        // DECAY = decay/release tijd
        // EXP = curve shaping
        // B versie: sustain is het niveau tijdens gate-hold
        float onsetT=params[ONSET_PARAM].getValue(); // attack tijd (0=snel, 1=langzaam)
        float decT=params[DECAY_PARAM].getValue()+(inputs[DCY_INPUT].isConnected()?clamp(inputs[DCY_INPUT].getVoltage()/5.f,-2.f,4.f):0.f);
        float expAmt=params[EXP_PARAM].getValue();

        // Gate/trigger bepaalt contour fase
        bool contourGate=(slopeStage!=IDLE)||inputs[CNTR_INPUT].isConnected();
        float contourGateVal=inputs[CNTR_INPUT].isConnected()?inputs[CNTR_INPUT].getVoltage()/10.f:(contourGate?1.f:0.f);

        // B versie: gate open = vol (1.0), gate dicht = 0
        // Sustain knop bepaalt het sustain niveau
        float contourTarget=contourGateVal>0.01f?1.0f:0.f;

        // Attack rate (ONSET knop: 0=instant, 1=langzaam)
        float attackT=onsetT*2.f; // 0 tot 2 seconden attack
        float rateUp=clamp(args.sampleTime/std::max(attackT,0.001f),0.f,1.f);

        // Decay rate met EXP curve
        float expFactor=std::pow(10.f,expAmt*2.f);
        float rateDown=clamp(args.sampleTime/std::max(decT,0.001f)*expFactor,0.f,1.f);

        float contourRate=(contourTarget>contourValue)?rateUp:rateDown;
        contourValue+=(contourTarget-contourValue)*contourRate;
        outputs[CONTOUR_OUTPUT].setVoltage(contourValue*10.f);

        // DYN input neemt over, anders gebruikt Contour waarde
        float dynCV=inputs[DYNMC_INPUT].isConnected()?clamp(inputs[DYNMC_INPUT].getVoltage()/10.f,0.f,1.f):contourValue;
        dynCV=clamp((dynCV-0.01f)/0.99f,0.f,1.f);

        // BALANCE / TIMBRE section - V1.2
        // 1. Baseline sound - stabiele mix ~30% fold karakter
        float extIn=inputs[EXT_INPUT].isConnected()?inputs[EXT_INPUT].getVoltage()/10.f:0.f;
        float baseSound=tri*0.5f+folded*0.5f+extIn;

        // 2. Timbre laag - echte wavefolder vervorming zoals 0-Coast
        float sustainDrive=1.0f; // B versie: sustain beïnvloedt drive niet
        float timbreTemp=clamp(params[BALANCE_PARAM].getValue(),0.f,1.f);
        if (inputs[TIMBRE_INPUT].isConnected()) timbreTemp+=inputs[TIMBRE_INPUT].getVoltage()/5.f;
        timbreTemp=clamp(timbreTemp,0.f,1.f);
        float shapedTimbreTemp=timbreTemp;
        float foldDrive=1.0f+shapedTimbreTemp*8.0f*sustainDrive;
        float driven=baseSound*foldDrive;
        float folded1=driven;
        folded1=std::fmod(folded1+1.f,4.f);
        if (folded1<0.f) folded1+=4.f;
        if (folded1>2.f) folded1=4.f-folded1;
        folded1=folded1-1.f;
        float folded2=driven*1.3f;
        folded2=std::fmod(folded2+1.f,4.f);
        if (folded2<0.f) folded2+=4.f;
        if (folded2>2.f) folded2=4.f-folded2;
        folded2=folded2-1.f;
        float timbreShaping=folded1*0.6f+folded2*0.3f+std::tanh(driven*0.5f)*0.1f;
        float extra=timbreShaping-baseSound;

        // 3. Timbre control - bereik 0.0 tot 1.0
        float timbreMax=clamp(params[BALANCE_PARAM].getValue(),0.f,1.f);
        float timbre=timbreMax;
        if (inputs[TIMBRE_INPUT].isConnected()) {
            float cvMod=inputs[TIMBRE_INPUT].getVoltage()/5.f;
            timbre=clamp(timbreMax*cvMod,0.f,timbreMax);
        }
        // B versie: vaste timbre boost (was gekoppeld aan sustain in A)
        timbre = clamp(timbre + 0.15f, 0.f, 1.f);
        float shapedTimbre=timbre;

        // Sustain-gedreven dynamische drive - meer sustain = iets meer saturatie


        // 4. Timbre schakelaar
        bool timbreOn=params[BALNC_PARAM].getValue()>0.5f;

        // 5. Finale output - additief, baseline altijd intact
        float out=baseSound;
        if (timbreOn) {
            out=baseSound+(extra*shapedTimbre*0.5f);
        }

        // 6. Lichte gain compensatie, geen harde clipping
        out=std::tanh(out*0.9f)*1.1f;

        // Smooth alleen bij release (dynCV daalt) — attack blijft instant
        float smoothCoeff = exp(-1.f / (0.002f * args.sampleRate));
        if (dynCV < smoothedDynCV)
            smoothedDynCV = smoothedDynCV * smoothCoeff + dynCV * (1.f - smoothCoeff);
        else
            smoothedDynCV = dynCV; // instant attack
        outputs[LINEOUT_OUTPUT].setVoltage(out*smoothedDynCV*5.f);
    }
    json_t* dataToJson() override { return json_object(); }
    void dataFromJson(json_t* rootJ) override { (void)rootJ; }
};


struct Drift13KnobLarge : SvgKnob {
    Drift13KnobLarge() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/Drift13KnobLarge.svg")));
        shadow->opacity = 0.f;
    }
};
struct Drift13KnobMedium : SvgKnob {
    Drift13KnobMedium() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/Drift13KnobMedium.svg")));
        shadow->opacity = 0.f;
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
struct Drift13Widget : SubmitModuleWidget {
    Drift13Widget(Drift13* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance,"res/Drift13.svg")));
                                
        // OSCILLATOR
        addParam(createParamCentered<Drift13KnobLarge>(Vec(60.443f,129.679f),module,Drift13::PITCH_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(Vec(85.365f,224.53f),module,Drift13::FINE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(31.044f,251.763f),module,Drift13::VOCT_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(30.803f,343.679f),module,Drift13::OUT1_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(70.196f,343.78f),module,Drift13::OUT2_OUTPUT));

        // OVERTONE + MULTIPLY
        addParam(createParamCentered<Drift13KnobMedium>(Vec(164.609f,128.963f),module,Drift13::OVERTONE_PARAM));
        addParam(createParamCentered<Drift13KnobMedium>(Vec(165.091f,215.161f),module,Drift13::MULTIPLY_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(145.351f,343.776f),module,Drift13::OVRTN_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(184.442f,343.745f),module,Drift13::LINFM_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(184.523f,297.341f),module,Drift13::MLTPL_INPUT));

        // SLOPE
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(271.935f,87.0f),module,Drift13::CYCLE_LIGHT));
        addParam(createParamCentered<CKSS>(Vec(236.282f,106.9f),module,Drift13::CYCLE_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(Vec(285.132f,113.759f),module,Drift13::RISE_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(Vec(285.121f,170.721f),module,Drift13::FALL_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(Vec(285.092f,226.632f),module,Drift13::TIME_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(Vec(285.059f,283.179f),module,Drift13::LOGEXP_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(235.934f,159.437f),module,Drift13::TRIG_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(236.282f,343.843f),module,Drift13::GATE_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(285.218f,343.759f),module,Drift13::SLOPE_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(235.888f,205.446f),module,Drift13::EON_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(235.895f,251.158f),module,Drift13::CNTR_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(235.913f,297.561f),module,Drift13::EOC_OUTPUT));

        // CONTOUR
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(376.995f,87.0f),module,Drift13::ONSET_LIGHT));
        addParam(createParamCentered<Drift13KnobSmall>(Vec(393.31f,113.603f),module,Drift13::ONSET_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(Vec(393.351f,170.721f),module,Drift13::SUSTAIN_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(Vec(393.343f,226.416f),module,Drift13::DECAY_PARAM));
        addParam(createParamCentered<Drift13KnobSmall>(Vec(393.31f,283.552f),module,Drift13::EXP_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(345.129f,252.694f),module,Drift13::DCY_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(345.076f,297.419f),module,Drift13::CNTR_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(345.094f,343.822f),module,Drift13::DYNMC_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(393.276f,343.854f),module,Drift13::CONTOUR_OUTPUT));

        // BALANCE
        addParam(createParamCentered<Drift13KnobMedium>(Vec(474.63f,128.847f),module,Drift13::BALANCE_PARAM));
        addParam(createParamCentered<CKSS>(Vec(452.889f,199.0f),module,Drift13::BALNC_PARAM));
        addOutput(createOutputCentered<PJ301MPort>(Vec(492.927f,343.82f),module,Drift13::LINEOUT_OUTPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(454.421f,343.82f),module,Drift13::TIMBRE_INPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(452.889f,179.271f),module,Drift13::TIMBRE_LIGHT));
    }

    void appendContextMenu(Menu* menu) override {
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Manual", "", []() {
            system::openBrowser("https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/drift/");
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


Model* modelDrift = createModel<Drift13, Drift13Widget>("Drift");
