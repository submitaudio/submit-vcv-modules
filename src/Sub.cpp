#include "plugin.hpp"

#include <cmath>
#include <cstdint>

struct SubKnob : SvgKnob {
	SubKnob() {
		minAngle = -0.83f * M_PI;
		maxAngle = 0.83f * M_PI;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobSmall.svg")));
		shadow->opacity = 0.f;
	}
};

struct SubTuneKnob : SvgKnob {
	SubTuneKnob() {
		minAngle = -0.83f * M_PI;
		maxAngle = 0.83f * M_PI;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobMedium.svg")));
		shadow->opacity = 0.f;
	}
};

struct SubOctaveKnob : SvgKnob {
	SubOctaveKnob() {
		minAngle = -0.83f * M_PI;
		maxAngle = 0.83f * M_PI;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobTiny.svg")));
		shadow->opacity = 0.f;
	}
};

	struct Sub : Module {
	enum ParamId {
		OCTAVE_PARAM,
		TONE_PARAM,
		RESONANCE_PARAM,
		MIX_PARAM,
		ENV_AMOUNT_PARAM,
		RELEASE_PARAM,
		LEVEL_PARAM,
		TUNE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		GATE_INPUT,
		ACCENT_INPUT,
		FILTER_CV_INPUT,
		ENV_AMOUNT_CV_INPUT,
		SUB_CV_INPUT,
		DRIVE_CV_INPUT,
		DECAY_CV_INPUT,
		RESONANCE_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		ENV_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		GATE_LIGHT,
		LIGHTS_LEN
	};

	float phase = 0.f;
	float subPhase = 0.f;
	float amplitudeEnv = 0.f;
	float subAmplitudeEnv = 0.f;
	float accentEnv = 0.f;
	float filterEnv = 0.f;
	float filterAge = 1.f;
	float filterZ1 = 0.f;
	float filterZ2 = 0.f;
	float filterZ3 = 0.f;
	float filterZ4 = 0.f;
	float noiseLP = 0.f;
	float dcInput = 0.f;
	float dcOutput = 0.f;
	uint32_t noiseState = 0x9e3779b9u;
	bool gateHigh = false;

	Sub() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
#ifdef SUBMIT_LAB_BUILD
		configParam(OCTAVE_PARAM, -4.f, 4.f, -1.f, "Octave", " oct");
		configParam(TONE_PARAM, 0.f, 1.f, 0.f, "Filter");
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.47951748967170715f, "Resonance");
		configParam(MIX_PARAM, 0.f, 1.f, 1.f, "Sub layer");
		configParam(ENV_AMOUNT_PARAM, 0.f, 1.f, 0.54000002145767212f, "Filter envelope amount");
		configParam(RELEASE_PARAM, 0.03f, 4.f, 1.1440122127532959f, "Decay", " s");
		configParam(LEVEL_PARAM, 0.f, 1.f, 0.99879515171051025f, "Drive");
		configParam(TUNE_PARAM, -1.f, 1.f, 0.f, "Tune", " cents", 0.f, 100.f);
#else
		configParam(OCTAVE_PARAM, -4.f, 4.f, 1.f, "Octave", " oct");
		configParam(TONE_PARAM, 0.f, 1.f, 0.34f, "Filter");
		configParam(RESONANCE_PARAM, 0.f, 1.f, 0.16f, "Resonance");
		configParam(MIX_PARAM, 0.f, 1.f, 0.f, "Sub layer");
		configParam(ENV_AMOUNT_PARAM, 0.f, 1.f, 0.54f, "Filter envelope amount");
		configParam(RELEASE_PARAM, 0.03f, 4.f, 0.24f, "Decay", " s");
		configParam(LEVEL_PARAM, 0.f, 1.f, 0.35f, "Drive");
		configParam(TUNE_PARAM, -1.f, 1.f, 0.f, "Tune", " cents", 0.f, 100.f);
#endif
		getParamQuantity(OCTAVE_PARAM)->snapEnabled = true;

		configInput(VOCT_INPUT, "V/OCT");
		configInput(GATE_INPUT, "Gate");
		configInput(ACCENT_INPUT, "Accent");
		configInput(FILTER_CV_INPUT, "Filter CV");
		configInput(ENV_AMOUNT_CV_INPUT, "Envelope amount CV");
		configInput(SUB_CV_INPUT, "Sub mix CV");
		configInput(DRIVE_CV_INPUT, "Drive CV");
		configInput(DECAY_CV_INPUT, "Decay CV");
		configInput(RESONANCE_CV_INPUT, "Resonance CV");
		configOutput(OUT_OUTPUT, "Audio");
		configOutput(ENV_OUTPUT, "Envelope");
	}

	static float polyBlep(float t, float dt) {
		if (t < dt) {
			t /= dt;
			return t + t - t * t - 1.f;
		}
		if (t > 1.f - dt) {
			t = (t - 1.f) / dt;
			return t * t + t + t + 1.f;
		}
		return 0.f;
	}

	static float smooth(float value, float target, float seconds, const ProcessArgs& args) {
		const float coefficient = 1.f - std::exp(-args.sampleTime / std::max(seconds, 1e-5f));
		return value + (target - value) * coefficient;
	}

	float nextNoise() {
		noiseState = noiseState * 1664525u + 1013904223u;
		return ((noiseState >> 8) & 0x00ffffff) / 8388607.5f - 1.f;
	}

	void process(const ProcessArgs& args) override {
		const bool nextGate = inputs[GATE_INPUT].getVoltage() > (gateHigh ? 0.1f : 1.f);
		const bool gateRising = nextGate && !gateHigh;
		gateHigh = nextGate;
		if (gateRising)
			filterAge = 0.f;

		const float accentInput = inputs[ACCENT_INPUT].isConnected()
			? clamp(inputs[ACCENT_INPUT].getVoltage() / 10.f, 0.f, 1.f)
			: 0.f;
		// Circles can send very short accents while the slower sub gate is
		// already high. Shape those edges into a musical contour so cutoff
		// modulation cannot jump abruptly in the middle of a sustained note.
		const float accentTime = accentInput > accentEnv ? 0.008f : 0.040f;
		accentEnv = smooth(accentEnv, accentInput, accentTime, args);
		const float accent = accentEnv;
		const float octave = std::round(params[OCTAVE_PARAM].getValue());
		const float basePitch = inputs[VOCT_INPUT].getVoltage() + octave;
		const float tune = params[TUNE_PARAM].getValue() / 12.f;
		const float frequency = clamp(261.6256f * std::pow(2.f, basePitch + tune), 12.f, 5000.f);
		// The lower layer follows V/OCT and OCTAVE, but deliberately ignores
		// TUNE so the upper layer can be detuned around a stable sub pitch.
		const float subFrequency = clamp(261.6256f * std::pow(2.f, basePitch - 1.f), 5.f, 2500.f);
		const float dt = clamp(frequency * args.sampleTime, 0.f, 0.45f);

		const float decay = clamp(
			params[RELEASE_PARAM].getValue()
				+ inputs[DECAY_CV_INPUT].getVoltage() / 10.f * (4.f - 0.03f),
			0.03f,
			4.f
		);

		if (gateHigh)
			amplitudeEnv = smooth(amplitudeEnv, 1.f, 0.0035f, args);
		else
			amplitudeEnv = smooth(amplitudeEnv, 0.f, decay, args);
		if (gateHigh)
			subAmplitudeEnv = smooth(subAmplitudeEnv, 1.f, 0.007f, args);
		else
			subAmplitudeEnv = smooth(subAmplitudeEnv, 0.f, decay, args);

		filterAge += args.sampleTime;
		const float length = decay;
		// Keep the short default contour, then open up into long evolving
		// filter motion in the upper half of the control. Let the contour keep
		// following that decay after gate-off instead of forcing it to zero;
		// otherwise the filter and FM colour change abruptly while the long
		// amplitude tail is still clearly audible.
		const float filterDecay = 0.045f + 0.20f * length + 0.50f * std::max(0.f, length - 0.40f);
		const float filterTarget = std::exp(-filterAge / filterDecay);
		filterEnv = smooth(filterEnv, filterTarget, 0.0025f, args);

		phase += frequency * args.sampleTime;
		subPhase += subFrequency * args.sampleTime;
		while (phase >= 1.f) phase -= 1.f;
		while (subPhase >= 1.f) subPhase -= 1.f;

		const float saw = 2.f * phase - 1.f - polyBlep(phase, dt);
		const float rawNoise = nextNoise();
		noiseLP += 0.10f * (rawNoise - noiseLP);
		const float attackNoise = (rawNoise - noiseLP) * 0.008f * filterEnv;
		// Upper layer only: keep the lively analog-style main oscillator and
		// remove the old octave-down sine. The dedicated lower sub layer will
		// be designed and mixed separately with the reserved SUB control.
		const float analogSound = (saw + attackNoise) * 0.72f;

		const float envAmount = clamp(
			params[ENV_AMOUNT_PARAM].getValue()
				+ inputs[ENV_AMOUNT_CV_INPUT].getVoltage() / 10.f
				+ accent * 0.30f,
			0.f,
			1.f
		);
		const float filterControl = clamp(
			params[TONE_PARAM].getValue() + inputs[FILTER_CV_INPUT].getVoltage() / 10.f,
			0.f,
			1.f
		);
		float cutoff = 42.f * std::pow(78.f, filterControl);
		cutoff += filterEnv * envAmount * 2600.f;
		cutoff = clamp(cutoff, 30.f, 8000.f);
		const float filterCoefficient = 1.f - std::exp(-2.f * M_PI * cutoff * args.sampleTime);
		const float resonanceControl = clamp(
			params[RESONANCE_PARAM].getValue() + inputs[RESONANCE_CV_INPUT].getVoltage() / 10.f,
			0.f,
			1.f
		);
		const float resonance = resonanceControl * 3.35f;

		const float ladderInput = std::tanh((analogSound - resonance * filterZ4) * 1.8f);
		filterZ1 += filterCoefficient * (ladderInput - filterZ1);
		filterZ2 += filterCoefficient * (filterZ1 - filterZ2);
		filterZ3 += filterCoefficient * (filterZ2 - filterZ3);
		filterZ4 += filterCoefficient * (filterZ3 - filterZ4);

		// Warm output-stage drive: increasing DRIVE adds soft, slightly
		// asymmetric saturation instead of acting as a volume control.
		// Fixed makeup compensation keeps the perceived level broadly stable.
		const float drive = clamp(
			params[LEVEL_PARAM].getValue() + inputs[DRIVE_CV_INPUT].getVoltage() / 10.f,
			0.f,
			1.f
		);
		const float driveGain = 1.f + 1.5f * drive * drive;
		const float bias = 0.075f * drive;
		const float driven = std::tanh((filterZ4 * 2.2f + bias) * driveGain)
			- std::tanh(bias * driveGain);
		const float driveMakeup = 1.f / (1.f + 0.42f * drive);
		const float upperSound = driven * driveMakeup * amplitudeEnv * 0.82f;

		const float subMix = clamp(
			params[MIX_PARAM].getValue() + inputs[SUB_CV_INPUT].getVoltage() / 10.f,
			0.f,
			1.f
		);
		const float subRadians = 2.f * M_PI * subPhase;
		// Gentle phase modulation gives the sub a defined pitch without the
		// buzz of a saw. A little extra motion at the start settles into a
		// round sustained tone.
		const float subFmIndex = 0.10f + 0.16f * filterEnv;
		const float subFundamental = std::sin(subRadians + subFmIndex * std::sin(2.f * subRadians));
		const float subSecond = std::sin(2.f * subRadians);
		const float subThird = std::sin(3.f * subRadians);
		// Below the normal audible bass range, reduce speaker-pumping energy
		// in the fundamental while retaining harmonics that communicate pitch.
		const float fundamentalWeight = clamp((subFrequency - 12.f) / 18.f, 0.35f, 1.f);
		const float subRaw = fundamentalWeight * subFundamental + 0.16f * subSecond + 0.045f * subThird;
		const float subWarm = std::tanh(subRaw * 1.25f) / std::tanh(1.25f);
		// Keep the fundamental layer level stable when a fast main-sequence
		// accent arrives during a longer sub note.
		const float subSound = subWarm * subAmplitudeEnv * 0.62f;
		// SUB=0 is bit-for-bit the upper layer alone. At higher settings a
		// small upper-layer trim prevents the combined signal jumping in level.
		const float combinedSound = upperSound * (1.f - 0.10f * subMix) + subSound * subMix;
		const float dcBlocked = combinedSound - dcInput + 0.9955f * dcOutput;
		dcInput = combinedSound;
		dcOutput = dcBlocked;
		outputs[OUT_OUTPUT].setVoltage(5.f * dcBlocked);
		outputs[ENV_OUTPUT].setVoltage(10.f * amplitudeEnv);

		lights[GATE_LIGHT].setBrightness(amplitudeEnv);
	}
};

struct SubWidget : SubmitModuleWidget {
	SubWidget(Sub* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Sub.svg")));

#ifdef SUBMIT_LAB_BUILD
		auto* betaBadge = new SvgWidget;
		betaBadge->setSvg(Svg::load(asset::plugin(pluginInstance, "res/beta-submit-lab.svg")));
		betaBadge->box.pos = Vec((box.size.x - betaBadge->box.size.x) * 0.5f, 364.f);
		addChild(betaBadge);
#endif

		addParam(createParamCentered<SubOctaveKnob>(Vec(30.973f, 93.577f), module, Sub::OCTAVE_PARAM));
		addParam(createParamCentered<SubTuneKnob>(Vec(89.678f, 93.394f), module, Sub::TUNE_PARAM));
		addParam(createParamCentered<SubKnob>(Vec(64.472f, 168.957f), module, Sub::TONE_PARAM));
		addParam(createParamCentered<SubKnob>(Vec(114.675f, 168.957f), module, Sub::RESONANCE_PARAM));
		addParam(createParamCentered<SubKnob>(Vec(64.472f, 228.913f), module, Sub::ENV_AMOUNT_PARAM));
		addParam(createParamCentered<SubKnob>(Vec(114.425f, 228.913f), module, Sub::RELEASE_PARAM));
		addParam(createParamCentered<SubKnob>(Vec(64.472f, 288.949f), module, Sub::MIX_PARAM));
		addParam(createParamCentered<SubKnob>(Vec(114.425f, 288.699f), module, Sub::LEVEL_PARAM));

		addInput(createInputCentered<PJ301MPort>(Vec(30.283f, 157.474f), module, Sub::FILTER_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(149.474f, 157.622f), module, Sub::RESONANCE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(30.283f, 217.524f), module, Sub::ENV_AMOUNT_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(149.474f, 217.407f), module, Sub::DECAY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(30.283f, 277.309f), module, Sub::SUB_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(149.474f, 277.566f), module, Sub::DRIVE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(30.283f, 341.646f), module, Sub::VOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(64.451f, 341.646f), module, Sub::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(98.462f, 341.646f), module, Sub::ACCENT_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(149.474f, 341.646f), module, Sub::OUT_OUTPUT));
	}
};

Model* modelSub = createModel<Sub, SubWidget>("Sub");
