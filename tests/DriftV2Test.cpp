// Test the actual Rack module DSP without constructing desktop widgets.
#define DRIFT_V2_TEST
#include "../src/DriftV2.cpp"
#include <cassert>
#include <cstdio>
#include <random>

using D=DriftBetaV2;
static Module::ProcessArgs args{48000.f,1.f/48000.f,0};
static void connect(D& m,int id,float voltage) {
    // Cable insertion normally sets this in Rack's engine.
    m.inputs[id].channels=1;
    m.inputs[id].setVoltage(voltage);
}
static void tick(D& m,int count) {
    for(int i=0;i<count;++i) m.process(args);
}
static void loadPatch(D& m,json_t* j) {
    static rack::plugin::Plugin plugin;
    static rack::plugin::Model* model=new rack::plugin::Model;
    static bool registered=false;
    plugin.slug="Submit";
    model->slug="Drift";m.model=model;
    if (!registered) {
        plugin.addModel(model);rack::plugin::plugins.push_back(&plugin);registered=true;
    }
    json_object_set_new(j,"plugin",json_string("Submit"));
    json_object_set_new(j,"model",json_string("Drift"));
    m.fromJson(j);
}
// Exercise the actual process path at each rate, including transitions.
static void basisTests(float rate) {
    args.sampleRate=rate; args.sampleTime=1.f/rate;
    D fm;
    connect(fm,D::LINFM_INPUT,4.f);
    tick(fm,int(rate*0.1f));
    assert(std::abs(fm.fmOutput-4.f*std::exp(-2.f*M_PI*5.f*(0.1f-args.sampleTime)))<2e-4f);
    tick(fm,int(rate));
    assert(std::abs(fm.fmOutput)<1e-6f);
    float phase=fm.phase; tick(fm,1);
    float increment=fm.phase-phase; if(increment<0.f) increment+=1.f;
    assert(std::abs(increment-dsp::FREQ_C4*0.25f/rate)<2e-7f);
    // Unplugging produces a bounded transient, then returns to the carrier.
    fm.inputs[D::LINFM_INPUT].channels=0; fm.inputs[D::LINFM_INPUT].setVoltage(0.f);
    tick(fm,1); assert(std::abs(fm.fmOutput+4.f)<1e-5f);
    tick(fm,int(rate)); assert(std::abs(fm.fmOutput)<1e-6f);
    connect(fm,D::VOCT_INPUT,1.f);
    phase=fm.phase; tick(fm,1); increment=fm.phase-phase; if(increment<0.f) increment+=1.f;
    assert(std::abs(increment-dsp::FREQ_C4*0.5f/rate)<2e-7f);

    // Positive/negative FM symmetry, sinusoidal passband and unipolar rejection.
    for(float hz : {100.f,1000.f,5000.f}) {
        D positive,negative;
        double energy=0.;
        for(int n=0;n<int(rate);++n) {
            float v=std::sin(2.f*M_PI*hz*n/rate);
            connect(positive,D::LINFM_INPUT,2.f+v);
            connect(negative,D::LINFM_INPUT,-2.f-v);
            tick(positive,1);tick(negative,1);
            assert(std::abs(positive.fmOutput+negative.fmOutput)<1e-6f);
            if(n>int(rate/2)) energy+=positive.fmOutput*positive.fmOutput;
        }
        assert(energy/(rate/2)>0.49 && energy/(rate/2)<0.51);
    }

    // CV modulation stays out of panel smoothing; core remains untouched.
    for(int input : {D::OVRTN_INPUT,D::MLTPL_INPUT,D::TIMBRE_INPUT}) {
        for(float hz : {100.f,1000.f,5000.f}) {
            D m,reference;
            for(D* v : {&m,&reference}) {
                v->params[D::OVERTONE_PARAM].setValue(0.4f);
                v->params[D::MULTIPLY_PARAM].setValue(0.4f);
                v->params[D::BALANCE_PARAM].setValue(0.5f);
                v->params[D::MULTIPLY_DEPTH_PARAM].setValue(1.f); v->params[D::DRONE_PARAM].setValue(1.f);
                connect(*v,D::MLTPL_INPUT,0.f);
                tick(*v,int(rate*0.05f));
            }
            double modulation=0.,difference=0.;
            for(int n=0;n<int(rate*0.1f);++n) {
                float cv=std::sin(2.f*M_PI*hz*n/rate);
                connect(m,input,cv);tick(m,1);tick(reference,1);
                assert(m.smoothedOvertone==reference.smoothedOvertone);
                assert(m.smoothedMultiply==reference.smoothedMultiply);
                assert(m.smoothedBalance==reference.smoothedBalance);
                assert(m.outputs[D::OUT1_OUTPUT].getVoltage()==reference.outputs[D::OUT1_OUTPUT].getVoltage());
                assert(m.outputs[D::OUT2_OUTPUT].getVoltage()==reference.outputs[D::OUT2_OUTPUT].getVoltage());
                float delta=input==D::OVRTN_INPUT?m.previousOvertone-m.smoothedOvertone:
                    input==D::MLTPL_INPUT?m.previousMultiply-m.smoothedMultiply:m.previousBalance-m.smoothedBalance;
                float scale=input==D::OVRTN_INPUT?10.f:input==D::MLTPL_INPUT?8.f:5.f;
                assert(std::abs(delta*scale-cv)<1e-6f);
                modulation+=delta*delta*scale*scale;
                float diff=m.outputs[D::LINEOUT_OUTPUT].getVoltage()-reference.outputs[D::LINEOUT_OUTPUT].getVoltage();
                difference+=diff*diff;
            }
            assert(modulation/(rate*0.1)>0.49);
            assert(difference>0.01);
        }
    }
    // A knob step retains its 1 ms time constant.
    D knob; knob.params[D::OVERTONE_PARAM].setValue(1.f);
    tick(knob,int(rate*0.001f));
    assert(knob.smoothedOvertone>0.62f && knob.smoothedOvertone<0.64f);
    // Internal audio-rate Slope follows the same direct path, including inversion.
    D slope; slope.params[D::CYCLE_PARAM].setValue(1.f);
    slope.params[D::RISE_PARAM].setValue(0.001f); slope.params[D::FALL_PARAM].setValue(0.001f);
    slope.params[D::MULTIPLY_DEPTH_PARAM].setValue(-1.f);
    for(int n=0;n<1000;++n) {
        tick(slope,1);
        assert(std::abs(slope.previousMultiply-(slope.smoothedMultiply-slope.slopeValue))<1e-7f);
    }
    // Negative Multiply attenuates the overtone route monotonically to silence.
    // Fundamental-only remains bit-identical even during closing modulation.
    double previousEnergy=1e20;
    for(float cv : {0.f,-2.f,-4.f,-6.f,-8.f,-12.f}) {
        D overtone,fundamental,reference;
        for(D* m : {&overtone,&fundamental,&reference}) {
            m->params[D::MULTIPLY_PARAM].setValue(0.f);
            m->params[D::MULTIPLY_DEPTH_PARAM].setValue(1.f); m->params[D::DRONE_PARAM].setValue(1.f);
            m->params[D::BALANCE_PARAM].setValue(m==&overtone?1.f:0.f);
            connect(*m,D::MLTPL_INPUT,m==&reference?0.f:cv);
            tick(*m,int(rate));
        }
        double energy=0.;
        for(int n=0;n<int(rate*0.1f);++n) {
            tick(overtone,1);tick(fundamental,1);tick(reference,1);
            float v=overtone.outputs[D::LINEOUT_OUTPUT].getVoltage();energy+=v*v;
            assert(fundamental.outputs[D::LINEOUT_OUTPUT].getVoltage()==reference.outputs[D::LINEOUT_OUTPUT].getVoltage());
        }
        assert(energy<=previousEnergy+1e-9); previousEnergy=energy;
        if(cv<=-8.f) assert(energy<1e-8);
        else assert(energy>0.01);
    }
    printf("%.0f Hz: AC FM, CV 100/1000/5000 Hz, knob smoothing, Slope, Multiply closure/core preservation passed\n",rate);
}
int main() {
    rack::Context context;
    context.engine=new rack::engine::Engine;
    rack::contextSet(&context);
    // Rise knob works with Cycle off, and TRIG does not depend on GATE.
    D fast,slow;
    fast.params[D::CYCLE_PARAM].setValue(0.f);
    slow.params[D::CYCLE_PARAM].setValue(0.f);
    fast.params[D::RISE_PARAM].setValue(0.001f);
    slow.params[D::RISE_PARAM].setValue(0.4f);
    connect(fast,D::TRIG_INPUT,10.f); connect(slow,D::TRIG_INPUT,10.f);
    tick(fast,200);tick(slow,200);
    assert(fast.slopeStage!=D::RISE && slow.slopeStage==D::RISE);
    // An unpatched Multiply receives a one-shot Slope too.
    tick(slow,4800);
    assert(slow.previousMultiply>slow.smoothedMultiply);
    // AD mode completes its attack after a one-sample trigger falls.
    D ad;
    ad.params[D::DECAY_PARAM].setValue(1.3f);
    ad.params[D::SUSTAIN_PARAM].setValue(0.f);
    ad.params[D::ONSET_PARAM].setValue(0.3f);
    connect(ad,D::CNTR_INPUT,10.f);tick(ad,1);
    connect(ad,D::CNTR_INPUT,0.f);
    float peak=0.f;bool eon=false;
    for(int i=0;i<48000;++i){tick(ad,1);peak=std::max(peak,ad.contourValue);eon|=ad.outputs[D::EON_OUTPUT].getVoltage()>0.f;}
    assert(peak>0.99f && eon && ad.contourValue==0.f);
    // Linear FM: +1V and -1V deviations are symmetric about carrier.
    D fm0,fmp,fmn;
    for (D* m : {&fm0,&fmp,&fmn}) m->params[D::FM_DEPTH_PARAM].setValue(0.1f);
    connect(fmp,D::LINFM_INPUT,1.f);connect(fmn,D::LINFM_INPUT,-1.f);
    tick(fm0,1);tick(fmp,1);tick(fmn,1);
    assert(std::abs(fmp.phase+fmn.phase-2.f*fm0.phase)<1e-7f);
    assert(std::abs(fmp.phase/fm0.phase-1.1f)<1e-5f);
    // Patch settings survive serialization, invalid types retain defaults.
    D saved,loaded;
    saved.params[D::FM_DEPTH_PARAM].setValue(0.5f);saved.params[D::MULTIPLY_DEPTH_PARAM].setValue(-0.5f);saved.params[D::DRONE_PARAM].setValue(1.f);
    json_t* j=json_object();
    json_object_set_new(j,"params",saved.paramsToJson());
    json_object_set_new(j,"data",saved.dataToJson());
    loadPatch(loaded,j);json_decref(j);
    assert(loaded.params[D::FM_DEPTH_PARAM].getValue()==0.5f && loaded.params[D::MULTIPLY_DEPTH_PARAM].getValue()==-0.5f && loaded.params[D::DRONE_PARAM].getValue()==1.f);
    // Old V2 menu values migrate via Rack's full loading path.
    D legacy;
    json_t* oldPatch=json_pack("{s:[],s:{s:f,s:f,s:f}}", "params", "data",
        "fmDepth",0.25,"multiplyDepth",-0.35,"dynamicsBias",1.0);
    loadPatch(legacy,oldPatch);json_decref(oldPatch);
    assert(legacy.params[D::FM_DEPTH_PARAM].getValue()==0.25f);
    assert(legacy.params[D::MULTIPLY_DEPTH_PARAM].getValue()==-0.35f);
    assert(legacy.params[D::DRONE_PARAM].getValue()==1.f);
    // Once migrated, serialized params win over stale legacy mirrors.
    legacy.params[D::FM_DEPTH_PARAM].setValue(0.75f);
    j=json_object();json_object_set_new(j,"params",legacy.paramsToJson());
    json_t* data=legacy.dataToJson();json_object_set_new(data,"fmDepth",json_real(0.1));
    json_object_set_new(j,"data",data);loadPatch(loaded,j);json_decref(j);
    assert(loaded.params[D::FM_DEPTH_PARAM].getValue()==0.75f);
    data=json_pack("{s:s,s:f,s:f}","fmDepth","invalid","multiplyDepth",8.0,"dynamicsBias",-2.0);
    legacy.dataFromJson(data);json_decref(data);
    assert(legacy.params[D::FM_DEPTH_PARAM].getValue()==0.1f);
    assert(legacy.params[D::MULTIPLY_DEPTH_PARAM].getValue()==1.f);
    assert(legacy.params[D::DRONE_PARAM].getValue()==0.f);
    static_assert(D::BALNC_PARAM==14 && D::TIMBRE_INPUT==13 && D::LINEOUT_OUTPUT==6,"Legacy IDs changed");
    puts("Legacy migration and V9 parameter round-trip passed");
    // Retain a live FM filter across sample-rate changes, then exercise feedback.
    D changing;
    connect(changing,D::LINFM_INPUT,4.f);tick(changing,100);
    for(float rate : {96000.f,44100.f,48000.f}) {
        args.sampleRate=rate;args.sampleTime=1.f/rate;
        tick(changing,int(rate*0.2f));
        assert(std::isfinite(changing.fmOutput));
    }
    assert(std::abs(changing.fmOutput)<1e-6f);
    // Finite/bounded outputs at several sample rates, extrema and fast modulation.
    std::mt19937 rng(1234);
    std::uniform_real_distribution<float> u(0.f,1.f);
    for(float rate : {44100.f,48000.f,96000.f}) {
        args.sampleRate=rate;args.sampleTime=1.f/rate;
        basisTests(rate);
        // Onset CV follows Decay CV polarity: +1 V halves attack time.
        D baseAttack,fastAttack,slowAttack;
        for(D* v : {&baseAttack,&fastAttack,&slowAttack}) {
            v->params[D::ONSET_PARAM].setValue(0.5f);
            connect(*v,D::CNTR_INPUT,10.f);
        }
        connect(fastAttack,D::ONSET_CV_INPUT,1.f);
        connect(slowAttack,D::ONSET_CV_INPUT,-1.f);
        tick(baseAttack,1);tick(fastAttack,1);tick(slowAttack,1);
        assert(std::abs(fastAttack.contourTime/baseAttack.contourTime-2.f)<1e-6f);
        assert(std::abs(slowAttack.contourTime/baseAttack.contourTime-0.5f)<1e-6f);
        // Sustain sums knob + CV/8 with safe endpoints during a held gate.
        D sus;
        sus.params[D::SUSTAIN_PARAM].setValue(0.25f);
        connect(sus,D::CNTR_INPUT,10.f);tick(sus,1);
        sus.contourStage=D::CONTOUR_SUSTAIN;
        for(float cv : {-12.f,0.f,2.f,8.f,12.f}) {
            connect(sus,D::SUSTAIN_CV_INPUT,cv);tick(sus,1);
            assert(sus.contourValue==clamp(0.25f+cv/8.f,0.f,1.f));
        }
        sus.inputs[D::SUSTAIN_CV_INPUT].channels=0;
        sus.inputs[D::SUSTAIN_CV_INPUT].setVoltage(0.f);tick(sus,1);
        assert(sus.contourValue==0.25f);
        // Drone switch opens audio without a gate and follows its status LED.
        D drone;drone.params[D::DRONE_PARAM].setValue(1.f);tick(drone,int(rate));
        assert(drone.smoothedDynCV>0.99f && drone.lights[D::DRONE_LIGHT].getBrightness()==1.f);
        drone.params[D::DRONE_PARAM].setValue(0.f);tick(drone,int(rate));
        assert(std::abs(drone.outputs[D::LINEOUT_OUTPUT].getVoltage())<1e-4f);
        puts("V9 Onset/Sustain CV and drone passed");
        D m;
        float maxAudio=0.f;
        for(int n=0;n<int(rate*5);++n) {
            if(n%97==0) {
                for(int p=0;p<D::PARAMS_LEN;++p){auto* q=m.paramQuantities[p];m.params[p].setValue(q->minValue+u(rng)*(q->maxValue-q->minValue));}
                for(int p=0;p<D::INPUTS_LEN;++p)connect(m,p,u(rng)*24.f-12.f);
            }
            tick(m,1);
            for(int p=0;p<D::OUTPUTS_LEN;++p){float v=m.outputs[p].getVoltage();assert(std::isfinite(v));assert(std::abs(v)<20.f);}
            assert(m.slopeValue>=0.f && m.slopeValue<=1.f);
            assert(m.contourValue>=-1e-6f && m.contourValue<=1.000001f);
            maxAudio=std::max(maxAudio,std::abs(m.outputs[D::LINEOUT_OUTPUT].getVoltage()));
        }
        // One-sample self-feedback at maximum depth remains finite/bounded.
        m.params[D::FM_DEPTH_PARAM].setValue(1.f);m.params[D::MULTIPLY_DEPTH_PARAM].setValue(1.f);m.params[D::DRONE_PARAM].setValue(1.f);
        for(int n=0;n<int(rate);++n) {
            float feedback=m.outputs[D::LINEOUT_OUTPUT].getVoltage();
            for(int input : {D::LINFM_INPUT,D::MLTPL_INPUT,D::OVRTN_INPUT,D::TIMBRE_INPUT}) connect(m,input,feedback);
            tick(m,1);
            for(int p=0;p<D::OUTPUTS_LEN;++p) {
                float v=m.outputs[p].getVoltage(); assert(std::isfinite(v) && std::abs(v)<20.f);
            }
        }
        m.params[D::DRONE_PARAM].setValue(0.f);
        connect(m,D::DYNMC_INPUT,0.f);tick(m,int(rate*2));
        assert(std::abs(m.outputs[D::LINEOUT_OUTPUT].getVoltage())<1e-4f);
        printf("%.0f Hz: finite/bounded, peak %.3f V, closes to silence\n",rate,maxAudio);
    }
    puts("DriftV2 DSP tests passed");
}
