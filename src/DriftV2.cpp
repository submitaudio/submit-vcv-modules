// Copyright (c) 2025 Submit Audio (submitaudio.nl)
// SPDX-License-Identifier: GPL-3.0-or-later
// https://github.com/submitaudio/submit-vcv-modules

#ifdef DRIFT_V2_TEST
#include <rack.hpp>
using namespace rack;
#else
#include "plugin.hpp"
#endif

struct DriftBetaV2 : Module {
    enum ParamId {
        PITCH_PARAM, FINE_PARAM, OVERTONE_PARAM, MULTIPLY_PARAM,
        RISE_PARAM, FALL_PARAM, TIME_PARAM, LOGEXP_PARAM, CYCLE_PARAM,
        ONSET_PARAM, SUSTAIN_PARAM, DECAY_PARAM, EXP_PARAM,
        BALANCE_PARAM, BALNC_PARAM,
        FM_DEPTH_PARAM, MULTIPLY_DEPTH_PARAM, DRONE_PARAM, PARAMS_LEN
    };
    enum InputId {
        VOCT_INPUT, LINFM_INPUT, OVRTN_INPUT, MLTPL_INPUT,
        TRIG_INPUT, GATE_INPUT, SLOPE_INPUT, DCY_INPUT, CNTR_INPUT,
        DYNMC_INPUT, FVND_INPUT, OVRTN_BAL_INPUT, EXT_INPUT, TIMBRE_INPUT,
        ONSET_CV_INPUT, SUSTAIN_CV_INPUT, INPUTS_LEN
    };
    enum OutputId {
        OUT1_OUTPUT, OUT2_OUTPUT, EOC_OUTPUT, EON_OUTPUT,
        CNTR_OUTPUT, CONTOUR_OUTPUT, LINEOUT_OUTPUT, OUTPUTS_LEN
    };
    enum LightId { CYCLE_LIGHT, ONSET_LIGHT, TIMBRE_LIGHT, DRONE_LIGHT, LIGHTS_LEN };

    float phase=0.f;
    enum SlopeStage { IDLE, RISE, FALL };
    enum ContourStage { CONTOUR_IDLE, CONTOUR_ATTACK, CONTOUR_DECAY, CONTOUR_SUSTAIN, CONTOUR_RELEASE };
    SlopeStage slopeStage=IDLE;
    ContourStage contourStage=CONTOUR_IDLE;
    float slopeValue=0.f, slopeTime=0.f, slopeStartValue=0.f;
    float contourValue=0.f, contourTime=0.f, contourStartValue=0.f;
    float smoothedDynCV=0.f;
    float smoothedOvertone=0.f, smoothedMultiply=0.f, smoothedBalance=0.f;
    float previousOvertone=0.f, previousMultiply=0.f, previousBalance=0.f;
    float previousSlope=0.f;
    float fmInput=0.f, fmOutput=0.f;
    float lpgState1=0.f, lpgState2=0.f;
    float dcInput=0.f, dcOutput=0.f;
    dsp::Decimator<4,16> triangleDecimator{0.88f};
    dsp::Decimator<4,16> squareDecimator{0.88f};
    dsp::Decimator<4,16> complexDecimator{0.88f};
    bool lastGate=false, lastTrig=false, lastContourGate=false;
    dsp::PulseGenerator eocPulse, eonPulse, onsetPulse;

    DriftBetaV2() {
        config(PARAMS_LEN,INPUTS_LEN,OUTPUTS_LEN,LIGHTS_LEN);
        configParam(PITCH_PARAM,-4.f,4.f,-2.f,"Octave"," oct");
        paramQuantities[PITCH_PARAM]->snapEnabled=true;
        configParam(FINE_PARAM,-7.f,7.f,0.f,"Tune"," st");
        configParam(OVERTONE_PARAM,0.f,1.f,0.4939756393432617f,"Overtone");
        configParam(MULTIPLY_PARAM,0.f,1.f,0.5036154389381409f,"Multiply");
        configParam(RISE_PARAM,0.001f,0.5f,0.43446865677833557f,"Rise");
        configParam(FALL_PARAM,0.001f,8.f,8.f,"Fall");
        configParam(TIME_PARAM,0.1f,4.f,0.1f,"Time");
        configParam(LOGEXP_PARAM,-1.f,1.f,-0.15421684086322784f,"Curve");
        configSwitch(CYCLE_PARAM,0.f,1.f,1.f,"Cycle",{"Off","On"});
        configParam(ONSET_PARAM,0.f,1.f,0.f,"Onset");
        configParam(SUSTAIN_PARAM,0.f,1.f,0.3951808214187622f,"Sustain");
        configParam(DECAY_PARAM,0.16483f,3.f,2.0743024349212646f,"Decay");
        configParam(EXP_PARAM,0.f,1.f,0.7301216125488281f,"Exp");
        configParam(BALANCE_PARAM,0.f,1.f,0.7662652730941772f,"Timbre");
        configSwitch(BALNC_PARAM,0.f,1.f,1.f,"Timbre",{"Off","On"});
        configParam(FM_DEPTH_PARAM,0.f,1.f,0.f,"Linear FM amount","%",0.f,100.f);
        configParam(MULTIPLY_DEPTH_PARAM,-1.f,1.f,0.19337385892868042f,"Multiply CV / Slope amount","%",0.f,100.f);
        configSwitch(DRONE_PARAM,0.f,1.f,0.f,"Dynamics drone",{"Off","On"});
        configInput(VOCT_INPUT,"V/OCT");
        configInput(LINFM_INPUT,"Linear FM");
        configInput(OVRTN_INPUT,"OVR");
        configInput(MLTPL_INPUT,"MLT");
        configInput(TRIG_INPUT,"TRIG");
        configInput(GATE_INPUT,"GATE");
        configInput(SLOPE_INPUT,"Slope rise/fall time CV");
        configInput(DCY_INPUT,"DCY");
        configInput(CNTR_INPUT,"CTR");
        configInput(DYNMC_INPUT,"DYN");
        configInput(FVND_INPUT,"Fundamental CV");
        configInput(OVRTN_BAL_INPUT,"Overtone bal CV");
        configInput(EXT_INPUT,"Ext In");
        configInput(TIMBRE_INPUT,"Timbre CV");
        configInput(ONSET_CV_INPUT,"Onset time CV (positive shortens)");
        configInput(SUSTAIN_CV_INPUT,"Sustain level CV (8 V full scale)");
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
        // Behavioral wave multiplier, not a transistor-level circuit model.
        // Each new reflection emerges continuously as drive crosses a threshold.
        float drive=1.f+amount*6.f;
        float y=x*drive;
        y=std::fmod(y+1.f,4.f);
        if (y<0.f) y+=4.f;
        if (y>2.f) y=4.f-y;
        y-=1.f;
        return y+0.18f*amount*(y-y*y*y);
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
        curve=clamp(curve,-0.9f,0.9f);
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

    float smoothPanel(float current, float target, const ProcessArgs& args) {
        // Reach exact endpoints despite float rounding, including overtone-only.
        float value=smoothValue(current,target,0.001f,args);
        return std::abs(value-target)<1e-5f?target:value;
    }

    void process(const ProcessArgs& args) override {
        const float fmDepth=params[FM_DEPTH_PARAM].getValue();
        const float multiplyDepth=params[MULTIPLY_DEPTH_PARAM].getValue();
        const float dynamicsBias=params[DRONE_PARAM].getValue();
        lights[DRONE_LIGHT].setBrightness(dynamicsBias);
        bool gate=inputs[GATE_INPUT].getVoltage()>(lastGate?0.1f:1.f);
        bool trig=inputs[TRIG_INPUT].getVoltage()>(lastTrig?0.1f:1.f);
        bool cycle=params[CYCLE_PARAM].getValue()>0.5f;
        float tScale=params[TIME_PARAM].getValue();
        float slopeCV=clamp(inputs[SLOPE_INPUT].getVoltage(),-10.f,10.f);
        float logexp=clamp(params[LOGEXP_PARAM].getValue(),-0.9f,0.9f);
        // Exponential time controls allow both slow modulation and audio-rate slopes.
        tScale*=std::exp2(-slopeCV)*(1.f-0.55f*std::abs(logexp));
        float risePosition=(params[RISE_PARAM].getValue()-0.001f)/0.499f;
        float fallPosition=(params[FALL_PARAM].getValue()-0.001f)/7.999f;
        float riseT=std::max(2.f*args.sampleTime,0.00025f*std::pow(80000.f,risePosition)*tScale);
        float fallT=std::max(2.f*args.sampleTime,0.00025f*std::pow(80000.f,fallPosition)*tScale);

        bool slopeTrigger=inputs[TRIG_INPUT].isConnected()?(trig&&!lastTrig):(gate&&!lastGate);
        if (slopeTrigger && slopeStage!=RISE) {
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
            slopeTime+=args.sampleTime/riseT;
            float t=clamp(slopeTime,0.f,1.f);
            slopeValue=slopeStartValue+(1.f-slopeStartValue)*applySlopeCurve(t,logexp);
            if (t>=1.f){
                slopeValue=1.f;
                slopeStartValue=1.f;
                slopeStage=FALL;
                slopeTime=0.f;
            }
        } else if (slopeStage==FALL){
            slopeTime+=args.sampleTime/fallT;
            float t=clamp(slopeTime,0.f,1.f);
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
        outputs[CNTR_OUTPUT].setVoltage(slopeValue*8.f);
        lights[CYCLE_LIGHT].setBrightness(params[CYCLE_PARAM].getValue() > 0.5f ? slopeValue : 0.f);
        lights[TIMBRE_LIGHT].setBrightness(params[BALNC_PARAM].getValue());
        lights[ONSET_LIGHT].setBrightness(onsetPulse.process(args.sampleTime)?1.f:0.f);

        bool contourGate=inputs[CNTR_INPUT].isConnected()
            ? inputs[CNTR_INPUT].getVoltage()>(lastContourGate?0.1f:1.f)
            : (inputs[GATE_INPUT].isConnected()?gate:(slopeStage==RISE));
        float response=clamp(params[EXP_PARAM].getValue(),0.f,1.f);
        float contourRate=1.f-0.65f*response;
        float attackT=0.0005f*std::pow(40000.f,params[ONSET_PARAM].getValue())*contourRate;
        // Leave the legacy unpatched attack exactly intact.
        if (inputs[ONSET_CV_INPUT].isConnected())
            attackT=clamp(attackT*std::exp2(-clamp(inputs[ONSET_CV_INPUT].getVoltage(),-10.f,10.f)),
                2.f*args.sampleTime,120.f);
        float decayPosition=(params[DECAY_PARAM].getValue()-0.16483f)/2.83517f;
        float decayT=clamp(0.002f*std::pow(10000.f,decayPosition)*contourRate*
            std::exp2(-clamp(inputs[DCY_INPUT].getVoltage(),-10.f,10.f)),2.f*args.sampleTime,120.f);
        float sustain=clamp(params[SUSTAIN_PARAM].getValue()+inputs[SUSTAIN_CV_INPUT].getVoltage()/8.f,0.f,1.f);
        float contourCurve=response*0.9f;

        if (contourGate&&!lastContourGate) {
            contourStartValue=contourValue;
            contourTime=0.f;
            contourStage=CONTOUR_ATTACK;
        } else if (!contourGate&&lastContourGate&&contourStage!=CONTOUR_IDLE&&sustain>0.001f) {
            contourStartValue=contourValue;
            contourTime=0.f;
            contourStage=CONTOUR_RELEASE;
        }
        lastContourGate=contourGate;

        if (contourStage==CONTOUR_ATTACK) {
            contourTime+=args.sampleTime/attackT;
            float t=clamp(contourTime,0.f,1.f);
            contourValue=contourStartValue+(1.f-contourStartValue)*applyContourCurve(t,contourCurve);
            if (t>=1.f) {contourValue=1.f;contourStartValue=1.f;contourTime=0.f;contourStage=CONTOUR_DECAY;eonPulse.trigger(1e-3f);}
        } else if (contourStage==CONTOUR_DECAY) {
            contourTime+=args.sampleTime/decayT;
            float t=clamp(contourTime,0.f,1.f);
            contourValue=contourStartValue+(sustain-contourStartValue)*applyContourCurve(t,-contourCurve);
            if (t>=1.f) {contourValue=sustain;contourStage=contourGate?CONTOUR_SUSTAIN:CONTOUR_RELEASE;contourStartValue=contourValue;contourTime=0.f;}
        } else if (contourStage==CONTOUR_SUSTAIN) {
            contourValue=sustain;
        } else if (contourStage==CONTOUR_RELEASE) {
            contourTime+=args.sampleTime/decayT;
            float t=clamp(contourTime,0.f,1.f);
            contourValue=contourStartValue*(1.f-applyContourCurve(t,-contourCurve));
            if (t>=1.f) {contourValue=0.f;contourStage=CONTOUR_IDLE;}
        }
        outputs[EON_OUTPUT].setVoltage(eonPulse.process(args.sampleTime)?10.f:0.f);
        outputs[CONTOUR_OUTPUT].setVoltage(contourValue*8.f);
        lights[ONSET_LIGHT].setBrightness(contourValue);

        float dynCV=inputs[DYNMC_INPUT].isConnected()?inputs[DYNMC_INPUT].getVoltage()/8.f:contourValue;
        // Fast transistor-style response, deliberately no slow vactrol tail.
        smoothedDynCV=smoothValue(smoothedDynCV,clamp(dynCV+dynamicsBias,0.f,1.f),0.00015f,args);

        // Only panel movements receive the 1 ms smoothing. CV and normalized
        // Slope bypass it; interpolate their sums on the 4x audio timeline below.
        smoothedOvertone=smoothPanel(smoothedOvertone,params[OVERTONE_PARAM].getValue(),args);
        smoothedMultiply=smoothPanel(smoothedMultiply,params[MULTIPLY_PARAM].getValue(),args);
        smoothedBalance=smoothPanel(smoothedBalance,params[BALANCE_PARAM].getValue(),args);
        float overtoneTarget=smoothedOvertone+inputs[OVRTN_INPUT].getVoltage()/10.f;
        float multiplyTarget=smoothedMultiply+multiplyDepth*(inputs[MLTPL_INPUT].isConnected()
            ? inputs[MLTPL_INPUT].getVoltage()/8.f : slopeValue);
        float balanceTarget=smoothedBalance+inputs[TIMBRE_INPUT].getVoltage()/5.f;

        // Provisional 5 Hz AC coupling, not a measured hardware cutoff.
        // Difference form rejects DC without a persistent lowpass rounding error.
        float fmVoltage=inputs[LINFM_INPUT].getVoltage();
        fmOutput=fmVoltage-fmInput+std::exp(-2.f*M_PI*5.f*args.sampleTime)*fmOutput;
        fmInput=fmVoltage;

        float pitchV=params[PITCH_PARAM].getValue()+params[FINE_PARAM].getValue()/12.f;
        if (inputs[VOCT_INPUT].isConnected()) pitchV+=inputs[VOCT_INPUT].getVoltage();
        float baseFreq=dsp::FREQ_C4*std::exp2(clamp(pitchV,-16.f,16.f));
        float freq=clamp(baseFreq*(1.f+fmDepth*fmOutput),0.f,args.sampleRate*0.4f);
        float dt=freq*args.sampleTime/4.f;
        float triangleBuffer[4];
        float squareBuffer[4];
        float complexBuffer[4];
        bool timbreOn=params[BALNC_PARAM].getValue()>0.5f;
        for (int i=0;i<4;++i) {
            float fraction=(i+1)*0.25f;
            float overtone=clamp(previousOvertone+(overtoneTarget-previousOvertone)*fraction,0.f,1.f);
            float multiply=clamp(previousMultiply+(multiplyTarget-previousMultiply)*fraction,-1.f,1.f);
            float balance=timbreOn?clamp(previousBalance+(balanceTarget-previousBalance)*fraction,0.f,1.f):0.f;
            float slope=previousSlope+(slopeValue-previousSlope)*fraction;
            phase+=dt;
            if (phase>=1.f) phase-=1.f;
            float tri=(phase<0.5f)?(4.f*phase-1.f):(3.f-4.f*phase);
            float triangleCore=tri;
            float square=phase<0.5f?1.f:-1.f;
            square+=polyBlep(phase,dt);
            float shifted=phase+0.5f;
            if (shifted>=1.f) shifted-=1.f;
            square-=polyBlep(shifted,dt);

            // Even partials -> odd partials -> Slope ring-modulation region.
            // Shape the same triangle core instead of mixing in a generic square.
            float even=2.f*std::abs(triangleCore)-1.f;
            float odd=triangleCore*(4.f*triangleCore*triangleCore-3.f);
            float harmonicMix=clamp(overtone/0.7f,0.f,1.f);
            float overtoneSound=even+(odd-even)*harmonicMix;
            float slopeMix=clamp((overtone-0.7f)/0.3f,0.f,1.f);
            float ring=triangleCore*(2.f*slope-1.f);
            overtoneSound+=(ring-overtoneSound)*slopeMix;
            triangleBuffer[i]=triangleCore;
            squareBuffer[i]=square;
            // Below panel minimum, close only the overtone branch continuously.
            // Existing CV scale is retained: sum -1 is silent, sum 0 is unity.
            float overtoneGain=1.f+std::min(multiply,0.f);
            float folded=waveFolder(overtoneSound,multiply)*overtoneGain;
            complexBuffer[i]=triangleCore+(folded-triangleCore)*balance;
        }
        previousOvertone=overtoneTarget;
        previousMultiply=multiplyTarget;
        previousBalance=balanceTarget;
        previousSlope=slopeValue;
        float triangleCore=triangleDecimator.process(triangleBuffer);
        float square=squareDecimator.process(squareBuffer);
        float voice=complexDecimator.process(complexBuffer);
        outputs[OUT1_OUTPUT].setVoltage(triangleCore*5.f);
        outputs[OUT2_OUTPUT].setVoltage(square*5.f);

        float cutoff=clamp(25.f*std::exp2(smoothedDynCV*10.f),25.f,args.sampleRate*0.45f);
        float lpgCoeff=1.f-std::exp(-2.f*M_PI*cutoff*args.sampleTime);
        lpgState1+=(voice-lpgState1)*lpgCoeff;
        lpgState2+=(lpgState1-lpgState2)*lpgCoeff;
        float lpgTone=lpgState1*0.35f+lpgState2*0.65f;
        float gain=smoothedDynCV*smoothedDynCV;
        float out=std::tanh(lpgTone*gain*1.4f)/std::tanh(1.4f);

        float dcBlocked=out-dcInput+std::exp(-2.f*M_PI*5.f*args.sampleTime)*dcOutput;
        dcInput=out;
        dcOutput=dcBlocked;
        outputs[LINEOUT_OUTPUT].setVoltage(dcBlocked*5.f);
    }
    json_t* dataToJson() override {
        json_t* j=json_object();
        json_object_set_new(j,"panelControlsVersion",json_integer(1));
        // Mirror values for older local V2 builds. Current builds use params.
        json_object_set_new(j,"fmDepth",json_real(params[FM_DEPTH_PARAM].getValue()));
        json_object_set_new(j,"multiplyDepth",json_real(params[MULTIPLY_DEPTH_PARAM].getValue()));
        json_object_set_new(j,"dynamicsBias",json_real(params[DRONE_PARAM].getValue()));
        return j;
    }
    void dataFromJson(json_t* j) override {
        if (json_integer_value(json_object_get(j,"panelControlsVersion"))>=1)
            return;
        auto read=[j](const char* key,float fallback,float lo,float hi) {
            json_t* v=json_object_get(j,key);
            double n=json_is_number(v)?json_number_value(v):fallback;
            return std::isfinite(n)?clamp(float(n),lo,hi):fallback;
        };
        params[FM_DEPTH_PARAM].setValue(read("fmDepth",0.1f,0.f,1.f));
        params[MULTIPLY_DEPTH_PARAM].setValue(read("multiplyDepth",0.35f,-1.f,1.f));
        params[DRONE_PARAM].setValue(read("dynamicsBias",0.f,0.f,1.f));
    }
};

#ifndef DRIFT_V2_TEST
struct DriftBetaV2KnobLarge : SvgKnob {
    DriftBetaV2KnobLarge() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobLarge.svg")));
        shadow->opacity = 0.f;
    }
};
struct DriftBetaV2KnobMedium : SvgKnob {
    DriftBetaV2KnobMedium() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobMedium.svg")));
        shadow->opacity = 0.f;
    }
};
struct DriftBetaV2KnobSmall : SvgKnob {
    DriftBetaV2KnobSmall() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobSmall.svg")));
        shadow->opacity = 0.f;
    }
};
struct DriftBetaV2Widget : SubmitModuleWidget {
    DriftBetaV2Widget(DriftBetaV2* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance,"res/DriftV2.svg")));

        // V9 controls; new IDs are appended for patch compatibility.
        addParam(createParamCentered<DriftBetaV2KnobSmall>(Vec(146.100f,286.100f),module,DriftBetaV2::MULTIPLY_DEPTH_PARAM));
        addParam(createParamCentered<DriftBetaV2KnobSmall>(Vec(193.600f,286.100f),module,DriftBetaV2::FM_DEPTH_PARAM));
        addParam(createParamCentered<CKSS>(Vec(465.950f,240.100f),module,DriftBetaV2::DRONE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(353.900f,160.900f),module,DriftBetaV2::ONSET_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(353.900f,204.300f),module,DriftBetaV2::SUSTAIN_CV_INPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(452.900f,220.600f),module,DriftBetaV2::DRONE_LIGHT));

        // OSCILLATOR
        addParam(createParamCentered<DriftBetaV2KnobLarge>(Vec(60.400f,144.700f),module,DriftBetaV2::PITCH_PARAM));
        addParam(createParamCentered<DriftBetaV2KnobSmall>(Vec(85.395f,224.458f),module,DriftBetaV2::FINE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(31.000f,251.800f),module,DriftBetaV2::VOCT_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(30.755f,343.654f),module,DriftBetaV2::OUT1_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(70.164f,343.754f),module,DriftBetaV2::OUT2_OUTPUT));

        // OVERTONE + MULTIPLY
        addParam(createParamCentered<DriftBetaV2KnobMedium>(Vec(168.600f,125.700f),module,DriftBetaV2::OVERTONE_PARAM));
        addParam(createParamCentered<DriftBetaV2KnobMedium>(Vec(169.100f,211.900f),module,DriftBetaV2::MULTIPLY_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(137.400f,343.810f),module,DriftBetaV2::OVRTN_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(199.584f,343.666f),module,DriftBetaV2::LINFM_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(168.878f,343.675f),module,DriftBetaV2::MLTPL_INPUT));

        // SLOPE
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(289.200f,85.300f),module,DriftBetaV2::CYCLE_LIGHT));
        addParam(createParamCentered<CKSS>(Vec(250.550f,106.900f),module,DriftBetaV2::CYCLE_PARAM));
        addParam(createParamCentered<DriftBetaV2KnobSmall>(Vec(302.108f,113.836f),module,DriftBetaV2::RISE_PARAM));
        addParam(createParamCentered<DriftBetaV2KnobSmall>(Vec(302.100f,170.700f),module,DriftBetaV2::FALL_PARAM));
        addParam(createParamCentered<DriftBetaV2KnobSmall>(Vec(302.100f,226.600f),module,DriftBetaV2::TIME_PARAM));
        addParam(createParamCentered<DriftBetaV2KnobSmall>(Vec(302.100f,283.200f),module,DriftBetaV2::LOGEXP_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(251.200f,159.400f),module,DriftBetaV2::TRIG_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(251.532f,343.767f),module,DriftBetaV2::GATE_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(297.200f,343.800f),module,DriftBetaV2::SLOPE_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(251.100f,205.400f),module,DriftBetaV2::EON_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(251.132f,251.222f),module,DriftBetaV2::CNTR_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(251.249f,297.610f),module,DriftBetaV2::EOC_OUTPUT));

        // CONTOUR
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(389.500f,85.800f),module,DriftBetaV2::ONSET_LIGHT));
        addParam(createParamCentered<DriftBetaV2KnobSmall>(Vec(405.744f,113.575f),module,DriftBetaV2::ONSET_PARAM));
        addParam(createParamCentered<DriftBetaV2KnobSmall>(Vec(405.700f,170.500f),module,DriftBetaV2::SUSTAIN_PARAM));
        addParam(createParamCentered<DriftBetaV2KnobSmall>(Vec(405.700f,226.400f),module,DriftBetaV2::DECAY_PARAM));
        addParam(createParamCentered<DriftBetaV2KnobSmall>(Vec(405.700f,283.300f),module,DriftBetaV2::EXP_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(353.900f,252.700f),module,DriftBetaV2::DCY_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(353.800f,297.400f),module,DriftBetaV2::CNTR_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(353.600f,343.800f),module,DriftBetaV2::DYNMC_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(403.500f,343.900f),module,DriftBetaV2::CONTOUR_OUTPUT));

        // BALANCE
        addParam(createParamCentered<DriftBetaV2KnobMedium>(Vec(480.600f,126.500f),module,DriftBetaV2::BALANCE_PARAM));
        addParam(createParamCentered<CKSS>(Vec(465.950f,189.200f),module,DriftBetaV2::BALNC_PARAM));
        addOutput(createOutputCentered<PJ301MPort>(Vec(492.900f,343.800f),module,DriftBetaV2::LINEOUT_OUTPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(454.400f,343.800f),module,DriftBetaV2::TIMBRE_INPUT));
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(452.900f,170.000f),module,DriftBetaV2::TIMBRE_LIGHT));
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


Model* modelDrift = createModel<DriftBetaV2, DriftBetaV2Widget>("Drift");
#endif
