// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// SPDX-License-Identifier: GPL-3.0-or-later
// https://github.com/submitaudio/submit-vcv-modules

#include "plugin.hpp"

struct DriftV2 : Module {
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
    enum ContourStage { CONTOUR_IDLE, CONTOUR_ATTACK, CONTOUR_DECAY, CONTOUR_SUSTAIN, CONTOUR_RELEASE };
    SlopeStage slopeStage=IDLE;
    ContourStage contourStage=CONTOUR_IDLE;
    float slopeValue=0.f, slopeTime=0.f, slopeStartValue=0.f;
    float contourValue=0.f, contourTime=0.f, contourStartValue=0.f;
    float smoothedDynCV=0.f;
    float smoothedOvertone=0.f, smoothedMultiply=0.f, smoothedBalance=0.f;
    float lpgState1=0.f, lpgState2=0.f;
    float dcInput=0.f, dcOutput=0.f;
    dsp::Decimator<4,16> triangleDecimator{0.88f};
    dsp::Decimator<4,16> squareDecimator{0.88f};
    dsp::Decimator<4,16> complexDecimator{0.88f};
    bool lastGate=false, lastTrig=false, lastContourGate=false;
    dsp::PulseGenerator eocPulse, eonPulse, onsetPulse;

    DriftV2() {
        config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
        configParam(PITCH_PARAM,-4.f,4.f,-2.f,"Octave"," oct");
        paramQuantities[PITCH_PARAM]->snapEnabled=true;
        configParam(FINE_PARAM,-7.f,7.f,0.f,"Tune"," st");
        configParam(OVERTONE_PARAM,0.f,1.f,0.69639f,"Overtone");
        configParam(MULTIPLY_PARAM,0.f,1.f,0.74578f,"Multiply");
        configParam(RISE_PARAM,0.001f,0.5f,0.24328f,"Rise"," s");
        configParam(FALL_PARAM,0.001f,8.f,6.352f,"Fall"," s");
        configParam(TIME_PARAM,0.1f,4.f,3.1683f,"Time");
        configParam(LOGEXP_PARAM,-1.f,1.f,-0.6747f,"Curve");
        configSwitch(CYCLE_PARAM,0.f,1.f,0.f,"Cycle",{"Off","On"});
        configParam(ONSET_PARAM,0.f,1.f,0.f,"Onset");
        configParam(SUSTAIN_PARAM,0.f,1.f,0.33373f,"Sustain");
        configParam(DECAY_PARAM,0.16483f,3.f,0.4381f,"Decay"," s");
        configParam(EXP_PARAM,0.f,1.f,0.83976f,"Exp");
        configParam(BALANCE_PARAM,0.f,1.f,0.71687f,"Timbre");
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
        amount=clamp(amount,0.f,1.f);
        if (amount<0.001f) return x;
        float bias=0.025f*amount;
        float drive=1.f+amount*4.5f;
        float y=(x+bias*(1.f-x*x))*drive;
        y=std::fmod(y+1.f,4.f);
        if (y<0.f) y+=4.f;
        if (y>2.f) y=4.f-y;
        y-=1.f;
        float saturation=1.f+amount*0.65f;
        float folded=std::tanh(y*saturation)/std::tanh(saturation);
        float blend=amount*amount*(3.f-2.f*amount);
        return x+(folded-x)*blend;
    }

    float polyBlep(float t, float dt) {
        if (t<dt) {
            t/=dt;
            return t+t-t*t-1.f;
        }
        if (t>1.f-dt) {
            t=(t-1.f)/dt;
            return t*t+t+t+1.f;
        }
        return 0.f;
    }

    float applySlopeCurve(float x, float curve) {
        x=clamp(x,0.f,1.f);
        if (std::abs(curve)<1e-4f) return x;
        float denom=curve-2.f*curve*std::abs(x)+1.f;
        if (std::abs(denom)<1e-6f) return x;
        return (x-curve*x)/denom;
    }

    float applyContourCurve(float x, float curve) {
        x=clamp(x,0.f,1.f);
        if (std::abs(curve)<1e-4f)
            return x;
        float exponent=std::pow(5.f,std::abs(curve));
        return curve>0.f?std::pow(x,exponent):1.f-std::pow(1.f-x,exponent);
    }

    float smoothValue(float current, float target, float time, const ProcessArgs& args) {
        float coefficient=1.f-std::exp(-args.sampleTime/std::max(time,1e-5f));
        return current+(target-current)*coefficient;
    }

    void process(const ProcessArgs& args) override {
        bool gate=inputs[GATE_INPUT].getVoltage()>(lastGate?0.1f:1.f);
        bool trig=inputs[TRIG_INPUT].getVoltage()>(lastTrig?0.1f:1.f);
        bool cycle=params[CYCLE_PARAM].getValue()>0.5f;
        float tScale=params[TIME_PARAM].getValue();
        float riseControl;
        if (cycle)
            riseControl=params[RISE_PARAM].getValue();
        else if (inputs[TRIG_INPUT].isConnected())
            riseControl=0.001f+clamp(inputs[TRIG_INPUT].getVoltage()/10.f,0.f,1.f)*0.499f;
        else
            riseControl=0.5f;
        float riseT=riseControl*tScale;
        float fallRaw=params[FALL_PARAM].getValue();
        float fallT=0.001f*std::pow(2000.f,fallRaw/8.f)*tScale;
        float slopeCV=inputs[SLOPE_INPUT].isConnected()?inputs[SLOPE_INPUT].getVoltage()/5.f:0.f;
        float logexp=clamp(params[LOGEXP_PARAM].getValue()+slopeCV,-1.f,1.f);

        if ((!cycle&&gate&&!lastGate)||(cycle&&trig&&!lastTrig)) {
            slopeStartValue=slopeValue;
            slopeStage=RISE;
            slopeTime=0.f;
            onsetPulse.trigger(0.05f);
        }
        if (cycle&&slopeStage==IDLE) {
            slopeStartValue=slopeValue;
            slopeStage=RISE;
            slopeTime=0.f;
        }

        if (slopeStage==RISE){
            slopeTime+=args.sampleTime;
            float t=clamp(slopeTime/riseT,0.f,1.f);
            slopeValue=slopeStartValue+(1.f-slopeStartValue)*applySlopeCurve(t,logexp);
            if (t>=1.f){
                slopeValue=1.f;
                eonPulse.trigger(1e-3f);
                slopeStartValue=1.f;
                slopeStage=FALL;
                slopeTime=0.f;
            }
        } else if (slopeStage==FALL){
            slopeTime+=args.sampleTime;
            float t=clamp(slopeTime/fallT,0.f,1.f);
            slopeValue=slopeStartValue*(1.f-applySlopeCurve(t,-logexp));
            if (t>=1.f){
                slopeValue=0.f;
                eocPulse.trigger(1e-3f);
                if(cycle){slopeStartValue=0.f;slopeStage=RISE;slopeTime=0.f;}
                else slopeStage=IDLE;
            }
        }

        lastGate=gate;
        lastTrig=trig;
        outputs[EOC_OUTPUT].setVoltage(eocPulse.process(args.sampleTime)?10.f:0.f);
        outputs[EON_OUTPUT].setVoltage(eonPulse.process(args.sampleTime)?10.f:0.f);
        outputs[CNTR_OUTPUT].setVoltage(slopeValue*10.f);
        lights[CYCLE_LIGHT].setBrightness(params[CYCLE_PARAM].getValue() > 0.5f ? slopeValue : 0.f);
        lights[TIMBRE_LIGHT].setBrightness(params[BALNC_PARAM].getValue());
        lights[ONSET_LIGHT].setBrightness(onsetPulse.process(args.sampleTime)?1.f:0.f);

        bool contourGate=inputs[CNTR_INPUT].isConnected()
            ? inputs[CNTR_INPUT].getVoltage()>(lastContourGate?0.1f:1.f)
            : (inputs[GATE_INPUT].isConnected()?gate:(slopeStage!=IDLE));
        float attackT=0.001f*std::pow(2000.f,params[ONSET_PARAM].getValue());
        float decayCV=inputs[DCY_INPUT].isConnected()?inputs[DCY_INPUT].getVoltage()/5.f:0.f;
        float decayT=clamp(params[DECAY_PARAM].getValue()+decayCV,0.16483f,3.f);
        float sustain=clamp(params[SUSTAIN_PARAM].getValue(),0.f,1.f);
        float contourCurve=clamp(params[EXP_PARAM].getValue()*1.8f-0.9f,-0.9f,0.9f);

        if (contourGate&&!lastContourGate) {
            contourStartValue=contourValue;
            contourTime=0.f;
            contourStage=CONTOUR_ATTACK;
        } else if (!contourGate&&lastContourGate&&contourStage!=CONTOUR_IDLE) {
            contourStartValue=contourValue;
            contourTime=0.f;
            contourStage=CONTOUR_RELEASE;
        }
        lastContourGate=contourGate;

        if (contourStage==CONTOUR_ATTACK) {
            contourTime+=args.sampleTime;
            float t=clamp(contourTime/attackT,0.f,1.f);
            contourValue=contourStartValue+(1.f-contourStartValue)*applyContourCurve(t,contourCurve);
            if (t>=1.f) {contourValue=1.f;contourStartValue=1.f;contourTime=0.f;contourStage=CONTOUR_DECAY;}
        } else if (contourStage==CONTOUR_DECAY) {
            contourTime+=args.sampleTime;
            float t=clamp(contourTime/decayT,0.f,1.f);
            contourValue=contourStartValue+(sustain-contourStartValue)*applyContourCurve(t,-contourCurve);
            if (t>=1.f) {contourValue=sustain;contourStage=contourGate?CONTOUR_SUSTAIN:CONTOUR_RELEASE;contourStartValue=contourValue;contourTime=0.f;}
        } else if (contourStage==CONTOUR_SUSTAIN) {
            contourValue=sustain;
        } else if (contourStage==CONTOUR_RELEASE) {
            contourTime+=args.sampleTime;
            float t=clamp(contourTime/decayT,0.f,1.f);
            contourValue=contourStartValue*(1.f-applyContourCurve(t,-contourCurve));
            if (t>=1.f) {contourValue=0.f;contourStage=CONTOUR_IDLE;}
        }
        outputs[CONTOUR_OUTPUT].setVoltage(contourValue*10.f);

        float dynCV=inputs[DYNMC_INPUT].isConnected()?clamp(inputs[DYNMC_INPUT].getVoltage()/10.f,0.f,1.f):contourValue;
        smoothedDynCV=smoothValue(smoothedDynCV,clamp(dynCV,0.f,1.f),0.0015f,args);

        float overtoneTarget=clamp(params[OVERTONE_PARAM].getValue()+(inputs[OVRTN_INPUT].isConnected()?inputs[OVRTN_INPUT].getVoltage()/10.f:0.f),0.f,1.f);
        float multiplyPanel=params[MULTIPLY_PARAM].getValue();
        float multiplyTarget;
        if (inputs[MLTPL_INPUT].isConnected())
            multiplyTarget=clamp(multiplyPanel+inputs[MLTPL_INPUT].getVoltage()/10.f,0.f,1.f);
        else if (cycle)
            multiplyTarget=clamp(multiplyPanel+(slopeValue-0.5f)*0.35f,0.f,1.f);
        else
            multiplyTarget=multiplyPanel;
        float balanceTarget=clamp(params[BALANCE_PARAM].getValue()+(inputs[TIMBRE_INPUT].isConnected()?inputs[TIMBRE_INPUT].getVoltage()/5.f:0.f),0.f,1.f);
        smoothedOvertone=smoothValue(smoothedOvertone,overtoneTarget,0.001f,args);
        smoothedMultiply=smoothValue(smoothedMultiply,multiplyTarget,0.001f,args);
        smoothedBalance=smoothValue(smoothedBalance,balanceTarget,0.001f,args);

        float pitchV=params[PITCH_PARAM].getValue()+params[FINE_PARAM].getValue()/12.f;
        if (inputs[VOCT_INPUT].isConnected()) pitchV+=inputs[VOCT_INPUT].getVoltage();
        if (inputs[LINFM_INPUT].isConnected()) pitchV+=inputs[LINFM_INPUT].getVoltage()*0.1f;
        float freq=clamp(dsp::FREQ_C4*std::pow(2.f,pitchV),1.f,20000.f);
        float dt=clamp(freq*args.sampleTime/4.f,1e-6f,0.49f);
        float triangleBuffer[4];
        float squareBuffer[4];
        float complexBuffer[4];
        for (int i=0;i<4;++i) {
            phase+=dt;
            if (phase>=1.f) phase-=1.f;
            float tri=(phase<0.5f)?(4.f*phase-1.f):(3.f-4.f*phase);
            float sineCore=-std::cos(2.f*M_PI*phase);
            float triangleCore=tri*0.92f+sineCore*0.08f;
            float square=phase<0.5f?1.f:-1.f;
            square+=polyBlep(phase,dt);
            float shifted=phase+0.5f;
            if (shifted>=1.f) shifted-=1.f;
            square-=polyBlep(shifted,dt);

            float harmonic=triangleCore*0.68f+square*0.32f;
            float slopeAudio=(slopeStage!=IDLE||cycle)?slopeValue*2.f-1.f:square;
            float overtoneSound;
            if (smoothedOvertone<0.5f)
                overtoneSound=triangleCore+(harmonic-triangleCore)*(smoothedOvertone*2.f);
            else
                overtoneSound=harmonic+(harmonic*0.7f+slopeAudio*0.3f-harmonic)*((smoothedOvertone-0.5f)*2.f);
            triangleBuffer[i]=triangleCore;
            squareBuffer[i]=square;
            complexBuffer[i]=waveFolder(overtoneSound,smoothedMultiply);
        }
        float triangleCore=triangleDecimator.process(triangleBuffer);
        float square=squareDecimator.process(squareBuffer);
        float complexSound=complexDecimator.process(complexBuffer);
        outputs[OUT1_OUTPUT].setVoltage(triangleCore*5.f);
        outputs[OUT2_OUTPUT].setVoltage(square*5.f);

        bool timbreOn=params[BALNC_PARAM].getValue()>0.5f;
        float mix=timbreOn?smoothedBalance:0.f;
        float voice=triangleCore+(complexSound-triangleCore)*mix;
        voice=std::tanh(voice*(1.f+0.35f*mix))/std::tanh(1.f+0.35f*mix);

        float cutoff=45.f+std::pow(smoothedDynCV,1.7f)*18000.f;
        float lpgCoeff=1.f-std::exp(-2.f*M_PI*cutoff*args.sampleTime);
        lpgState1+=(voice-lpgState1)*lpgCoeff;
        lpgState2+=(lpgState1-lpgState2)*lpgCoeff;
        float lpgTone=lpgState1*0.72f+lpgState2*0.28f;
        float gain=std::pow(smoothedDynCV,1.25f);
        float out=std::tanh(lpgTone*gain*1.15f);

        float dcBlocked=out-dcInput+0.995f*dcOutput;
        dcInput=out;
        dcOutput=dcBlocked;
        outputs[LINEOUT_OUTPUT].setVoltage(dcBlocked*5.f);
    }
    json_t* dataToJson() override { return json_object(); }
    void dataFromJson(json_t* rootJ) override { (void)rootJ; }
};


struct DriftV2KnobLarge : SvgKnob {
    DriftV2KnobLarge() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobLarge.svg")));
        shadow->opacity = 0.f;
    }
};
struct DriftV2KnobMedium : SvgKnob {
    DriftV2KnobMedium() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobMedium.svg")));
        shadow->opacity = 0.f;
    }
};
struct DriftV2KnobSmall : SvgKnob {
    DriftV2KnobSmall() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobSmall.svg")));
        shadow->opacity = 0.f;
    }
};
struct DriftV2Widget : SubmitModuleWidget {
    DriftV2Widget(DriftV2* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance,"res/Drift13.svg")));

        // OSCILLATOR
        addParam(createParamCentered<DriftV2KnobLarge>(Vec(60.4f,129.7f),module,DriftV2::PITCH_PARAM));
        addParam(createParamCentered<DriftV2KnobSmall>(Vec(85.4f,224.5f),module,DriftV2::FINE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(31.0f,251.8f),module,DriftV2::VOCT_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(30.8f,343.7f),module,DriftV2::OUT1_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(70.2f,343.8f),module,DriftV2::OUT2_OUTPUT));

        // OVERTONE + MULTIPLY
        addParam(createParamCentered<DriftV2KnobMedium>(Vec(164.6f,129.0f),module,DriftV2::OVERTONE_PARAM));
        addParam(createParamCentered<DriftV2KnobMedium>(Vec(165.1f,215.2f),module,DriftV2::MULTIPLY_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(145.4f,343.8f),module,DriftV2::OVRTN_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(184.4f,343.7f),module,DriftV2::LINFM_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(184.5f,297.3f),module,DriftV2::MLTPL_INPUT));

        // SLOPE
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(273.2f,86.3f),module,DriftV2::CYCLE_LIGHT));
        addParam(createParamCentered<CKSS>(Vec(233.55f,106.9f),module,DriftV2::CYCLE_PARAM));
        addParam(createParamCentered<DriftV2KnobSmall>(Vec(285.1f,113.8f),module,DriftV2::RISE_PARAM));
        addParam(createParamCentered<DriftV2KnobSmall>(Vec(285.1f,170.7f),module,DriftV2::FALL_PARAM));
        addParam(createParamCentered<DriftV2KnobSmall>(Vec(285.1f,226.6f),module,DriftV2::TIME_PARAM));
        addParam(createParamCentered<DriftV2KnobSmall>(Vec(285.1f,283.2f),module,DriftV2::LOGEXP_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(234.2f,159.4f),module,DriftV2::TRIG_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(234.5f,343.8f),module,DriftV2::GATE_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(285.2f,343.8f),module,DriftV2::SLOPE_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(234.1f,205.4f),module,DriftV2::EON_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(234.1f,251.2f),module,DriftV2::CNTR_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(234.2f,297.6f),module,DriftV2::EOC_OUTPUT));

        // CONTOUR
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(379.5f,86.3f),module,DriftV2::ONSET_LIGHT));
        addParam(createParamCentered<DriftV2KnobSmall>(Vec(395.9f,113.6f),module,DriftV2::ONSET_PARAM));
        addParam(createParamCentered<DriftV2KnobSmall>(Vec(395.9f,170.5f),module,DriftV2::SUSTAIN_PARAM));
        addParam(createParamCentered<DriftV2KnobSmall>(Vec(395.9f,226.4f),module,DriftV2::DECAY_PARAM));
        addParam(createParamCentered<DriftV2KnobSmall>(Vec(395.9f,283.3f),module,DriftV2::EXP_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(345.1f,252.7f),module,DriftV2::DCY_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(345.1f,297.4f),module,DriftV2::CNTR_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(344.8f,343.8f),module,DriftV2::DYNMC_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(395.8f,343.9f),module,DriftV2::CONTOUR_OUTPUT));

        // BALANCE
        addParam(createParamCentered<DriftV2KnobMedium>(Vec(478.6f,129.2f),module,DriftV2::BALANCE_PARAM));
        addParam(createParamCentered<CKSS>(Vec(453.45f,200.0f),module,DriftV2::BALNC_PARAM));
        addOutput(createOutputCentered<PJ301MPort>(Vec(492.9f,343.8f),module,DriftV2::LINEOUT_OUTPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(454.4f,343.8f),module,DriftV2::TIMBRE_INPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(453.4f,179.3f),module,DriftV2::TIMBRE_LIGHT));
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


Model* modelDrift = createModel<DriftV2, DriftV2Widget>("Drift");
