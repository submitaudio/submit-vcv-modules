#include "plugin.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

struct CirclesKnob : SvgKnob {
	CirclesKnob() {
		minAngle = -0.83f * M_PI;
		maxAngle = 0.83f * M_PI;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobPetite.svg")));
		shadow->opacity = 0.f;
	}
};

struct CirclesSmallKnob : SvgKnob {
	CirclesSmallKnob() {
		minAngle = -0.83f * M_PI;
		maxAngle = 0.83f * M_PI;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobPetite.svg")));
		shadow->opacity = 0.f;
	}
};

struct YellowRedLight : GrayModuleLightWidget {
	YellowRedLight() {
		addBaseColor(SCHEME_YELLOW);
		addBaseColor(SCHEME_RED);
	}
};

struct Circles : Module {
	enum ParamId {
		STEP_1_PARAM,
		STEP_2_PARAM,
		STEP_3_PARAM,
		STEP_4_PARAM,
		STEP_5_PARAM,
		STEP_6_PARAM,
		STEP_7_PARAM,
		STEP_8_PARAM,
		ROOT_PARAM,
		RANGE_PARAM,
		LENGTH_PARAM,
		SUB_STEP_PARAM,
		SCALE_A_PARAM,
		SCALE_B_PARAM,
		SCALE_C_PARAM,
		SCALE_D_PARAM,
		SCALE_COUNT_PARAM,
		SCALE_BARS_PARAM,
		SUB_EDIT_SLOT_PARAM,
		SUB_BARS_PARAM,
		SUB_SHIFT_PARAM,
		SCALE_A_ENABLE_PARAM,
		SCALE_B_ENABLE_PARAM,
		SCALE_C_ENABLE_PARAM,
		SCALE_D_ENABLE_PARAM,
		FLOW_PARAM,
		SPEED_PARAM,
		DICE_PARAM,
		STEP_1_ENABLE_PARAM,
		STEP_2_ENABLE_PARAM,
		STEP_3_ENABLE_PARAM,
		STEP_4_ENABLE_PARAM,
		STEP_5_ENABLE_PARAM,
		STEP_6_ENABLE_PARAM,
		STEP_7_ENABLE_PARAM,
		STEP_8_ENABLE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CLOCK_INPUT,
		RESET_INPUT,
		TRANSPOSE_INPUT,
		STEP_1_CV_INPUT,
		STEP_2_CV_INPUT,
		STEP_3_CV_INPUT,
		STEP_4_CV_INPUT,
		STEP_5_CV_INPUT,
		STEP_6_CV_INPUT,
		STEP_7_CV_INPUT,
		STEP_8_CV_INPUT,
		FLOW_CV_INPUT,
		NEXT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		VOCT_OUTPUT,
		TRIG_OUTPUT,
		GATE_OUTPUT,
		ACCENT_OUTPUT,
		EOC_OUTPUT,
		SUB_VOCT_OUTPUT,
		SUB_GATE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		STEP_1_R_LIGHT, STEP_1_G_LIGHT, STEP_1_B_LIGHT,
		STEP_2_R_LIGHT, STEP_2_G_LIGHT, STEP_2_B_LIGHT,
		STEP_3_R_LIGHT, STEP_3_G_LIGHT, STEP_3_B_LIGHT,
		STEP_4_R_LIGHT, STEP_4_G_LIGHT, STEP_4_B_LIGHT,
		STEP_5_R_LIGHT, STEP_5_G_LIGHT, STEP_5_B_LIGHT,
		STEP_6_R_LIGHT, STEP_6_G_LIGHT, STEP_6_B_LIGHT,
		STEP_7_R_LIGHT, STEP_7_G_LIGHT, STEP_7_B_LIGHT,
		STEP_8_R_LIGHT, STEP_8_G_LIGHT, STEP_8_B_LIGHT,
		CLOCK_LIGHT,
		SUB_LIGHT,
		SCALE_A_SELECT_YELLOW_LIGHT,
		SCALE_A_SELECT_RED_LIGHT,
		SCALE_B_SELECT_YELLOW_LIGHT,
		SCALE_B_SELECT_RED_LIGHT,
		SCALE_C_SELECT_YELLOW_LIGHT,
		SCALE_C_SELECT_RED_LIGHT,
		SCALE_D_SELECT_YELLOW_LIGHT,
		SCALE_D_SELECT_RED_LIGHT,
		DICE_LIGHT,
		STEP_1_ENABLE_LIGHT,
		STEP_2_ENABLE_LIGHT,
		STEP_3_ENABLE_LIGHT,
		STEP_4_ENABLE_LIGHT,
		STEP_5_ENABLE_LIGHT,
		STEP_6_ENABLE_LIGHT,
		STEP_7_ENABLE_LIGHT,
		STEP_8_ENABLE_LIGHT,
		STEP_1_ACTIVE_RED_LIGHT,
		STEP_2_ACTIVE_RED_LIGHT,
		STEP_3_ACTIVE_RED_LIGHT,
		STEP_4_ACTIVE_RED_LIGHT,
		STEP_5_ACTIVE_RED_LIGHT,
		STEP_6_ACTIVE_RED_LIGHT,
		STEP_7_ACTIVE_RED_LIGHT,
		STEP_8_ACTIVE_RED_LIGHT,
		LIGHTS_LEN
	};

	struct Scale {
		const char* name;
		std::array<int, 12> notes;
		int count;
	};

	static constexpr std::array<const char*, 12> rootNames = {
		"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
	};

	static constexpr std::array<Scale, 8> scales = {{
		{"MINOR",          {0, 2, 3, 5, 7, 8, 10}, 7},
		{"DORIAN",         {0, 2, 3, 5, 7, 9, 10}, 7},
		{"PHRYGIAN",       {0, 1, 3, 5, 7, 8, 10}, 7},
		{"MINOR PENTA",    {0, 3, 5, 7, 10}, 5},
		{"HARMONIC MINOR", {0, 2, 3, 5, 7, 8, 11}, 7},
		{"MAJOR",          {0, 2, 4, 5, 7, 9, 11}, 7},
		{"FIFTHS",         {0, 7}, 2},
		{"OCTAVES",        {0}, 1}
	}};

	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger resetTrigger;
	dsp::SchmittTrigger nextScaleTrigger;
	dsp::PulseGenerator trigPulse;
	dsp::PulseGenerator gatePulse;
	dsp::PulseGenerator eocPulse;
	dsp::PulseGenerator subGatePulse;
	dsp::PulseGenerator clockLightPulse;
	dsp::SchmittTrigger diceLightTrigger;
	dsp::PulseGenerator diceLightPulse;

	int currentStep = -1;
	int lcdPreviousStep = -1;
	int64_t samplesSinceMainStep = 0;
	int64_t previousMainStepPeriod = 0;
	int activeScaleSlot = 0;
	int completedBarsOnScale = 0;
	int completedBarsOnSub = 0;
	int64_t totalSteps = 0;
	int64_t samplesSinceClock = 0;
	int64_t quarterPeriod = 0;
	int subdivision = 0;
	bool haveClockEdge = false;
	bool tempoValid = false;
	float currentPitch = 0.f;
	float subPitch = 0.f;
	bool currentAccent = false;
	std::array<int, 4> subSteps = {{3, 0, 0, 0}};
	int editedSubSlot = 2;
	bool subStepsInitialized = true;
	bool scaleEnableInitialized = true;
	std::array<float, 8> stepCvOffsets = {{0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}};
	std::array<bool, 4> scaleEnabled = {{true, true, true, true}};
	std::array<dsp::SchmittTrigger, 4> scaleEnableTriggers;
	std::array<bool, 8> stepEnabled = {{true, true, true, true, true, true, true, true}};
	std::array<dsp::SchmittTrigger, 8> stepEnableTriggers;
	int pendulumDirection = 1;
	uint32_t flowRandomState = 0x6d2b79f5u;
	int diceCharacter = 1;

	Circles() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		const std::array<float, 8> defaultStepNotes = {{
			0.8397586941719055f, 0.5714285969734192f,
			0.5714285969734192f, 0.6428571343421936f,
			0.7857142686843872f, 0.7142857313156128f,
			0.7142857313156128f, 0.5714285969734192f
		}};
		for (int i = 0; i < 8; ++i)
			configParam(STEP_1_PARAM + i, 0.f, 1.f, defaultStepNotes[i], string::f("Step %d note", i + 1));

		configSwitch(ROOT_PARAM, 0.f, 11.f, 3.f, "Root note",
			{"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"});
		configSwitch(RANGE_PARAM, 1.f, 4.f, 2.f, "Pitch range", {"1 octave", "2 octaves", "3 octaves", "4 octaves"});
		configParam(LENGTH_PARAM, 0.03f, 1.25f, 0.15346992015838623f, "Main gate length", "%", 0.f, 100.f);
		configSwitch(SUB_STEP_PARAM, 1.f, 8.f, 1.f, "Sub source step (selected scale slot)", {"1", "2", "3", "4", "5", "6", "7", "8"});

		configSwitch(SCALE_A_PARAM, 0.f, 7.f, 1.f, "Scale A",
			{"Minor", "Dorian", "Phrygian", "Minor pentatonic", "Harmonic minor", "Major", "Fifths", "Octaves"});
		configSwitch(SCALE_B_PARAM, 0.f, 7.f, 3.f, "Scale B",
			{"Minor", "Dorian", "Phrygian", "Minor pentatonic", "Harmonic minor", "Major", "Fifths", "Octaves"});
		configSwitch(SCALE_C_PARAM, 0.f, 7.f, 6.f, "Scale C",
			{"Minor", "Dorian", "Phrygian", "Minor pentatonic", "Harmonic minor", "Major", "Fifths", "Octaves"});
		configSwitch(SCALE_D_PARAM, 0.f, 7.f, 5.f, "Scale D",
			{"Minor", "Dorian", "Phrygian", "Minor pentatonic", "Harmonic minor", "Major", "Fifths", "Octaves"});
		configSwitch(SCALE_COUNT_PARAM, 1.f, 4.f, 2.f, "Scale chain length", {"1 scale", "2 scales", "3 scales", "4 scales"});
		configSwitch(SCALE_BARS_PARAM, 1.f, 8.f, 2.f, "Bars per scale", {"1 bar", "2 bars", "3 bars", "4 bars", "5 bars", "6 bars", "7 bars", "8 bars"});
		configSwitch(SUB_EDIT_SLOT_PARAM, 0.f, 3.f, 2.f, "Edit sub source for scale", {"Scale A", "Scale B", "Scale C", "Scale D"});
		configSwitch(SUB_BARS_PARAM, 1.f, 8.f, 2.f, "Bars per sub note", {"1 bar", "2 bars", "3 bars", "4 bars", "5 bars", "6 bars", "7 bars", "8 bars"});
		configSwitch(SUB_SHIFT_PARAM, 0.f, 7.f, 0.f, "Sub launch step", {"Step 1", "Step 2", "Step 3", "Step 4", "Step 5", "Step 6", "Step 7", "Step 8"});
		configButton(SCALE_A_ENABLE_PARAM, "Include Scale A");
		configButton(SCALE_B_ENABLE_PARAM, "Include Scale B");
		configButton(SCALE_C_ENABLE_PARAM, "Include Scale C");
		configButton(SCALE_D_ENABLE_PARAM, "Include Scale D");
		configSwitch(FLOW_PARAM, 0.f, 4.f, 0.f, "Flow",
			{"Forward", "Reverse", "Pendulum", "Drunk", "Random"});
		configSwitch(SPEED_PARAM, 0.f, 3.f, 1.f, "Main sequence speed",
			{"1/4", "1/8", "1/16", "1/32"});
		configButton(DICE_PARAM, "Generate musical step notes");
		for (int i = 0; i < 8; ++i)
			configButton(STEP_1_ENABLE_PARAM + i, string::f("Step %d enable", i + 1));

		getParamQuantity(ROOT_PARAM)->snapEnabled = true;
		getParamQuantity(RANGE_PARAM)->snapEnabled = true;
		getParamQuantity(SUB_STEP_PARAM)->snapEnabled = true;
		for (int i = 0; i < 4; ++i)
			getParamQuantity(SCALE_A_PARAM + i)->snapEnabled = true;
		getParamQuantity(SCALE_COUNT_PARAM)->snapEnabled = true;
		getParamQuantity(SCALE_BARS_PARAM)->snapEnabled = true;
		getParamQuantity(SUB_EDIT_SLOT_PARAM)->snapEnabled = true;
		getParamQuantity(SUB_BARS_PARAM)->snapEnabled = true;
		getParamQuantity(SUB_SHIFT_PARAM)->snapEnabled = true;
		getParamQuantity(FLOW_PARAM)->snapEnabled = true;
		getParamQuantity(SPEED_PARAM)->snapEnabled = true;

		configInput(CLOCK_INPUT, "Clock (1 PPQN)");
		configInput(RESET_INPUT, "Reset");
		configInput(TRANSPOSE_INPUT, "Quantized transpose");
		for (int i = 0; i < 8; ++i)
			configInput(STEP_1_CV_INPUT + i, string::f("Step %d CV", i + 1));
		configInput(FLOW_CV_INPUT, "Flow CV");
		configInput(NEXT_INPUT, "Advance to next enabled scale");
		configOutput(VOCT_OUTPUT, "Quantized V/OCT");
		configOutput(TRIG_OUTPUT, "Step trigger");
		configOutput(GATE_OUTPUT, "Variable-length gate");
		configOutput(ACCENT_OUTPUT, "Accent");
		configOutput(EOC_OUTPUT, "End of cycle");
		configOutput(SUB_VOCT_OUTPUT, "Slow sub V/OCT");
		configOutput(SUB_GATE_OUTPUT, "Slow sub gate");
	}

	int getRoot() {
		return clamp((int) std::round(params[ROOT_PARAM].getValue()), 0, 11);
	}

	int getRange() {
		return clamp((int) std::round(params[RANGE_PARAM].getValue()), 1, 4);
	}

	int getScaleCount() {
		return clamp((int) std::round(params[SCALE_COUNT_PARAM].getValue()), 1, 4);
	}

	bool isScaleEnabled(int slot) {
		return scaleEnabled[clamp(slot, 0, 3)];
	}

	int getFirstEnabledScale() {
		for (int i = 0; i < 4; ++i) {
			if (isScaleEnabled(i))
				return i;
		}
		return 0;
	}

	int getNextEnabledScale(int current) {
		for (int offset = 1; offset <= 4; ++offset) {
			const int candidate = (current + offset) % 4;
			if (isScaleEnabled(candidate))
				return candidate;
		}
		return getFirstEnabledScale();
	}

	void syncScaleEnableButtons() {
		if (!scaleEnableInitialized) {
			// Existing V2 patches used the first N slots. Carry that selection
			// forward the first time they are opened with the button workflow.
			const int legacyCount = getScaleCount();
			for (int i = 0; i < 4; ++i)
				scaleEnabled[i] = i < legacyCount;
			scaleEnableInitialized = true;
		}

		for (int i = 0; i < 4; ++i) {
			if (scaleEnableTriggers[i].process(params[SCALE_A_ENABLE_PARAM + i].getValue())) {
				const bool isOnlyEnabled = scaleEnabled[i] && [&]() {
					for (int j = 0; j < 4; ++j) {
						if (j != i && scaleEnabled[j])
							return false;
					}
					return true;
				}();
				if (!isOnlyEnabled)
					scaleEnabled[i] = !scaleEnabled[i];
			}
		}

		bool anyEnabled = false;
		for (int i = 0; i < 4; ++i)
			anyEnabled |= isScaleEnabled(i);
		if (!anyEnabled)
			scaleEnabled[0] = true;
	}

	void syncStepEnableButtons() {
		for (int i = 0; i < 8; ++i) {
			if (stepEnableTriggers[i].process(params[STEP_1_ENABLE_PARAM + i].getValue()))
				stepEnabled[i] = !stepEnabled[i];
		}
	}

	int getScaleBars() {
		return clamp((int) std::round(params[SCALE_BARS_PARAM].getValue()), 1, 8);
	}

	int getSubBars() {
		return clamp((int) std::round(params[SUB_BARS_PARAM].getValue()), 1, 8);
	}

	int getSubShift() {
		return clamp((int) std::round(params[SUB_SHIFT_PARAM].getValue()), 0, 7);
	}

	int getFlow() {
		if (inputs[FLOW_CV_INPUT].isConnected())
			return clamp((int) std::round(inputs[FLOW_CV_INPUT].getVoltage() * 0.4f), 0, 4);
		return clamp((int) std::round(params[FLOW_PARAM].getValue()), 0, 4);
	}

	int getSpeed() {
		return clamp((int) std::round(params[SPEED_PARAM].getValue()), 0, 3);
	}

	uint32_t nextFlowRandom() {
		flowRandomState ^= flowRandomState << 13;
		flowRandomState ^= flowRandomState >> 17;
		flowRandomState ^= flowRandomState << 5;
		return flowRandomState;
	}

	int getNextSequenceStep() {
		if (currentStep < 0) {
			pendulumDirection = 1;
			return getFlow() == 1 ? 7 : 0;
		}

		switch (getFlow()) {
			case 1: // Reverse
				return (currentStep + 7) % 8;
			case 2: { // Pendulum
				int next = currentStep + pendulumDirection;
				if (next > 7) {
					pendulumDirection = -1;
					next = 6;
				}
				else if (next < 0) {
					pendulumDirection = 1;
					next = 1;
				}
				return next;
			}
			case 3: // Drunk: move one neighbouring step, bouncing at the ends
				if (currentStep == 0)
					return 1;
				if (currentStep == 7)
					return 6;
				return currentStep + ((nextFlowRandom() & 1u) ? 1 : -1);
			case 4: { // Random: choose any other step, never repeat immediately
				int next = (int) (nextFlowRandom() % 7u);
				if (next >= currentStep)
					++next;
				return next;
			}
			default: // Forward
				return (currentStep + 1) % 8;
		}
	}

	float getMainStepSeconds(const ProcessArgs& args) {
		const float quarterSeconds = quarterPeriod > 0 ? quarterPeriod * args.sampleTime : 0.2f;
		switch (getSpeed()) {
			case 0: return quarterSeconds;
			case 1: return quarterSeconds * 0.5f;
			case 3: return quarterSeconds * 0.125f;
			default: return quarterSeconds * 0.25f;
		}
	}

	int getMainAdvancesOnGrid() {
		switch (getSpeed()) {
			case 0: return totalSteps % 4 == 0 ? 1 : 0;
			case 1: return totalSteps % 2 == 0 ? 1 : 0;
			default: return 1;
		}
	}

	int getScaleAtSlot(int slot) {
		return clamp((int) std::round(params[SCALE_A_PARAM + clamp(slot, 0, 3)].getValue()), 0, 7);
	}

	int getActiveScaleIndex() {
		return getScaleAtSlot(activeScaleSlot);
	}

	int getSubStep() {
		return clamp((int) std::round(params[SUB_STEP_PARAM].getValue()) - 1, 0, 7);
	}

	int getSubEditSlot() {
		return clamp((int) std::round(params[SUB_EDIT_SLOT_PARAM].getValue()), 0, 3);
	}

	int getSubStepForSlot(int slot) {
		return subSteps[clamp(slot, 0, 3)];
	}

	void syncSubStepEditor() {
		if (!subStepsInitialized) {
			// V2 patches created before per-scale sub sources used one shared
			// source step. Preserve that behaviour when they are first loaded.
			subSteps.fill(getSubStep());
			subStepsInitialized = true;
		}

		const int requestedSlot = getSubEditSlot();
		if (requestedSlot != editedSubSlot) {
			editedSubSlot = requestedSlot;
			params[SUB_STEP_PARAM].setValue((float) (subSteps[editedSubSlot] + 1));
		}
		subSteps[editedSubSlot] = getSubStep();
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_t* subStepsJ = json_array();
		for (int step : subSteps)
			json_array_append_new(subStepsJ, json_integer(step));
		json_object_set_new(rootJ, "subSteps", subStepsJ);
		json_t* scaleEnabledJ = json_array();
		for (int i = 0; i < 4; ++i)
			json_array_append_new(scaleEnabledJ, json_integer(scaleEnabled[i] ? 1 : 0));
		json_object_set_new(rootJ, "scaleEnabled", scaleEnabledJ);
		json_t* stepEnabledJ = json_array();
		for (int i = 0; i < 8; ++i)
			json_array_append_new(stepEnabledJ, json_integer(stepEnabled[i] ? 1 : 0));
		json_object_set_new(rootJ, "stepEnabled", stepEnabledJ);
		json_object_set_new(rootJ, "diceCharacter", json_integer(diceCharacter));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* subStepsJ = json_object_get(rootJ, "subSteps");
		if (json_is_array(subStepsJ)) {
			for (int i = 0; i < 4; ++i) {
				json_t* stepJ = json_array_get(subStepsJ, i);
				if (json_is_integer(stepJ))
					subSteps[i] = clamp((int) json_integer_value(stepJ), 0, 7);
			}
			editedSubSlot = getSubEditSlot();
			subStepsInitialized = true;
			params[SUB_STEP_PARAM].setValue((float) (subSteps[editedSubSlot] + 1));
		}
		else {
			// Older V2 patches stored one shared SUB STEP parameter. Let the
			// existing migration path copy that saved value into all four slots.
			subStepsInitialized = false;
		}

		json_t* scaleEnabledJ = json_object_get(rootJ, "scaleEnabled");
		if (json_is_array(scaleEnabledJ)) {
			for (int i = 0; i < 4; ++i) {
				json_t* enabledJ = json_array_get(scaleEnabledJ, i);
				if (json_is_integer(enabledJ))
					scaleEnabled[i] = json_integer_value(enabledJ) != 0;
			}
			scaleEnableInitialized = true;
		}
		else {
			// Older V2 patches used the hidden scale-count parameter before the
			// four illuminated enable buttons stored their own state.
			scaleEnableInitialized = false;
		}

		json_t* stepEnabledJ = json_object_get(rootJ, "stepEnabled");
		if (json_is_array(stepEnabledJ)) {
			for (int i = 0; i < 8; ++i) {
				json_t* enabledJ = json_array_get(stepEnabledJ, i);
				if (json_is_integer(enabledJ))
					stepEnabled[i] = json_integer_value(enabledJ) != 0;
			}
		}

		json_t* diceCharacterJ = json_object_get(rootJ, "diceCharacter");
		if (json_is_integer(diceCharacterJ))
			diceCharacter = clamp((int) json_integer_value(diceCharacterJ), 0, 2);
	}

	void onReset() override {
		subSteps = {{3, 0, 0, 0}};
		editedSubSlot = 2;
		subStepsInitialized = true;
		scaleEnableInitialized = true;
		scaleEnabled = {{true, true, true, true}};
		stepEnabled.fill(true);
		pendulumDirection = 1;
		flowRandomState = 0x6d2b79f5u;
		diceCharacter = 1;
	}

	void rollDice() {
		const Scale& scale = scales[getActiveScaleIndex()];
		const int maximumIndex = scale.count * getRange();
		if (maximumIndex <= 0)
			return;

		auto randomInt = [](int maximum) {
			return maximum > 0 ? (int) (random::u32() % (uint32_t) maximum) : 0;
		};
		auto clampIndex = [maximumIndex](int index) {
			return clamp(index, 0, maximumIndex);
		};
		auto smallMove = [&]() {
			const int choice = randomInt(10);
			if (choice < 3)
				return 0;
			if (choice < 6)
				return 1;
			if (choice < 9)
				return -1;
			return randomInt(2) ? 2 : -2;
		};

		std::array<int, 8> noteIndices;
		if (diceCharacter == 0) {
			for (int i = 0; i < 8; ++i) {
				const int current = (int) std::round(params[STEP_1_PARAM + i].getValue() * maximumIndex);
				const int move = randomInt(10) < 6 ? 0 : (randomInt(2) ? 1 : -1);
				noteIndices[i] = clampIndex(current + move);
			}
		}
		else {
			const int anchorOctave = getRange() / 2;
			const int rootAnchor = clampIndex(anchorOctave * scale.count);
			int fifthDegree = 0;
			int fifthDistance = 128;
			for (int degree = 0; degree < scale.count; ++degree) {
				const int distance = std::abs(scale.notes[degree] - 7);
				if (distance < fifthDistance) {
					fifthDistance = distance;
					fifthDegree = degree;
				}
			}

			noteIndices[0] = rootAnchor;
			noteIndices[4] = clampIndex(rootAnchor + fifthDegree);
			if (diceCharacter == 1) {
				for (int i = 1; i < 4; ++i)
					noteIndices[i] = clampIndex(noteIndices[i - 1] + smallMove());
				for (int i = 5; i < 7; ++i)
					noteIndices[i] = clampIndex(noteIndices[i - 1] + smallMove());
				noteIndices[7] = clampIndex(rootAnchor + (randomInt(3) - 1));
			}
			else {
				for (int i = 1; i < 4; ++i) {
					const int leap = randomInt(9) - 4;
					noteIndices[i] = clampIndex(noteIndices[i - 1] + leap);
				}
				for (int i = 5; i < 7; ++i) {
					const int leap = randomInt(9) - 4;
					noteIndices[i] = clampIndex(noteIndices[i - 1] + leap);
				}
				noteIndices[7] = clampIndex(rootAnchor + randomInt(5) - 2);
			}
		}

		auto* action = new history::ComplexAction;
		action->name = "generate Circles melody";
		for (int i = 0; i < 8; ++i) {
			const float oldValue = params[STEP_1_PARAM + i].getValue();
			const float newValue = noteIndices[i] / (float) maximumIndex;
			if (oldValue == newValue)
				continue;
			auto* change = new history::ParamChange;
			change->moduleId = id;
			change->paramId = STEP_1_PARAM + i;
			change->oldValue = oldValue;
			change->newValue = newValue;
			action->push(change);
			APP->engine->setParamValue(this, STEP_1_PARAM + i, newValue);
		}
		if (action->isEmpty())
			delete action;
		else
			APP->history->push(action);
	}

	int getNoteSemitones(int step, int scaleIndex) {
		const Scale& scale = scales[clamp(scaleIndex, 0, 7)];
		const int totalNotes = scale.count * getRange() + 1;
		const int stepIndex = clamp(step, 0, 7);
		const float normalized = clamp(params[STEP_1_PARAM + stepIndex].getValue() + stepCvOffsets[stepIndex], 0.f, 1.f);
		const int index = clamp((int) std::round(normalized * (totalNotes - 1)), 0, totalNotes - 1);
		const int octave = index / scale.count;
		const int degree = index % scale.count;
		return getRoot() + scale.notes[degree] + 12 * (octave - 1);
	}

	float getPitch(int step, int scaleIndex) {
		const float transpose = std::round(inputs[TRANSPOSE_INPUT].getVoltage() * 12.f) / 12.f;
		return getNoteSemitones(step, scaleIndex) / 12.f + transpose;
	}

	const char* getCurrentNoteName() {
		if (currentStep < 0)
			return "--";
		int semitone = getNoteSemitones(currentStep, getActiveScaleIndex()) % 12;
		if (semitone < 0)
			semitone += 12;
		return rootNames[semitone];
	}

	void sampleStepCV(int step) {
		const int stepIndex = clamp(step, 0, 7);
		Input& input = inputs[STEP_1_CV_INPUT + stepIndex];
		stepCvOffsets[stepIndex] = input.isConnected()
			? clamp(input.getVoltage() / 10.f, -1.f, 1.f)
			: 0.f;
	}

	void resetSequence() {
		currentStep = -1;
		lcdPreviousStep = -1;
		samplesSinceMainStep = 0;
		previousMainStepPeriod = 0;
		activeScaleSlot = getFirstEnabledScale();
		completedBarsOnScale = 0;
		completedBarsOnSub = 0;
		totalSteps = 0;
		subdivision = 0;
		samplesSinceClock = 0;
		quarterPeriod = 0;
		haveClockEdge = false;
		tempoValid = false;
		currentPitch = 0.f;
		subPitch = 0.f;
		currentAccent = false;
		pendulumDirection = 1;
		flowRandomState = 0x6d2b79f5u;
		stepCvOffsets.fill(0.f);
		trigPulse.reset();
		gatePulse.reset();
		eocPulse.reset();
		subGatePulse.reset();
	}

	void advanceScaleAtBarBoundary() {
		if (totalSteps == 0 || totalSteps % 16 != 0)
			return;
		if (!isScaleEnabled(activeScaleSlot)) {
			activeScaleSlot = getNextEnabledScale(activeScaleSlot);
			completedBarsOnScale = 0;
			return;
		}
		++completedBarsOnScale;
		if (completedBarsOnScale >= getScaleBars()) {
			activeScaleSlot = getNextEnabledScale(activeScaleSlot);
			completedBarsOnScale = 0;
		}
	}

	void triggerSub(const ProcessArgs& args) {
		// Every scale slot has its own stored source step. The pitch is latched
		// here, on the bar boundary, so it cannot move during the slow sub gate.
		const int subStep = getSubStepForSlot(activeScaleSlot);
		sampleStepCV(subStep);
		subPitch = getPitch(subStep, getActiveScaleIndex()) - 2.f;
		const float barSeconds = quarterPeriod > 0 ? quarterPeriod * args.sampleTime * 4.f : 1.5f;
		subGatePulse.trigger(std::max(barSeconds * getSubBars() * 0.75f, 1e-3f));
	}

	void advanceMainStep(const ProcessArgs& args) {
		lcdPreviousStep = currentStep;
		if (samplesSinceMainStep > 16)
			previousMainStepPeriod = samplesSinceMainStep;
		samplesSinceMainStep = 0;
		currentStep = getNextSequenceStep();
		sampleStepCV(currentStep);
		currentPitch = getPitch(currentStep, getActiveScaleIndex());
		const bool enabled = stepEnabled[currentStep];
		currentAccent = enabled && (currentStep == 0 || currentStep == 4);
		if (enabled) {
			trigPulse.trigger(1e-3f);
			gatePulse.trigger(std::max(getMainStepSeconds(args) * params[LENGTH_PARAM].getValue(), 1e-3f));
		}
		else {
			trigPulse.reset();
			gatePulse.reset();
		}
		if (currentStep == 0)
			eocPulse.trigger(1e-3f);
	}

	void advanceGrid(const ProcessArgs& args) {
		advanceScaleAtBarBoundary();
		const bool subLaunchStep = (totalSteps % 16) == getSubShift();
		for (int i = 0; i < getMainAdvancesOnGrid(); ++i)
			advanceMainStep(args);
		if (subLaunchStep) {
			if (completedBarsOnSub == 0)
				triggerSub(args);
			++completedBarsOnSub;
			if (completedBarsOnSub >= getSubBars())
				completedBarsOnSub = 0;
		}
		++totalSteps;
	}

	void process(const ProcessArgs& args) override {
		syncScaleEnableButtons();
		syncStepEnableButtons();
		syncSubStepEditor();
		if (resetTrigger.process(inputs[RESET_INPUT].getVoltage()))
			resetSequence();
		if (nextScaleTrigger.process(inputs[NEXT_INPUT].getVoltage())) {
			activeScaleSlot = getNextEnabledScale(activeScaleSlot);
			completedBarsOnScale = 0;
		}
		if (diceLightTrigger.process(params[DICE_PARAM].getValue()))
			diceLightPulse.trigger(160e-3f);

		++samplesSinceClock;
		++samplesSinceMainStep;
		if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage())) {
			clockLightPulse.trigger(30e-3f);
			if (haveClockEdge && samplesSinceClock > 16) {
				quarterPeriod = samplesSinceClock;
				if (!tempoValid) {
					currentStep = 3;
					tempoValid = true;
				}
			}
			haveClockEdge = true;
			samplesSinceClock = 0;
			subdivision = 0;
			advanceGrid(args);
		}

		if (tempoValid && quarterPeriod > 0 && subdivision < 7) {
			const int64_t nextSample = (quarterPeriod * (subdivision + 1)) / 8;
			if (samplesSinceClock >= nextSample) {
				++subdivision;
				if (subdivision % 2 == 0)
					advanceGrid(args);
				else if (getSpeed() == 3)
					advanceMainStep(args);
			}
		}

		if (currentStep >= 0)
			currentPitch = getPitch(currentStep, getActiveScaleIndex());

		const bool trigHigh = trigPulse.process(args.sampleTime);
		const bool gateHigh = gatePulse.process(args.sampleTime);
		const bool eocHigh = eocPulse.process(args.sampleTime);
		const bool subGateHigh = subGatePulse.process(args.sampleTime);
		outputs[VOCT_OUTPUT].setVoltage(currentPitch);
		outputs[TRIG_OUTPUT].setVoltage(trigHigh ? 10.f : 0.f);
		outputs[GATE_OUTPUT].setVoltage(gateHigh ? 10.f : 0.f);
		outputs[ACCENT_OUTPUT].setVoltage(gateHigh && currentAccent ? 10.f : 0.f);
		outputs[EOC_OUTPUT].setVoltage(eocHigh ? 10.f : 0.f);
		outputs[SUB_VOCT_OUTPUT].setVoltage(subPitch);
		outputs[SUB_GATE_OUTPUT].setVoltage(subGateHigh ? 10.f : 0.f);
		lights[CLOCK_LIGHT].setBrightness(clockLightPulse.process(args.sampleTime) ? 1.f : 0.f);
		lights[SUB_LIGHT].setBrightness(subGateHigh ? 1.f : 0.f);
		lights[DICE_LIGHT].setBrightness(diceLightPulse.process(args.sampleTime) ? 1.f : 0.f);
		for (int i = 0; i < 4; ++i) {
			const bool enabled = isScaleEnabled(i);
			const bool active = enabled && i == activeScaleSlot;
			lights[SCALE_A_SELECT_YELLOW_LIGHT + i * 2].setBrightness(enabled && !active ? 0.8f : 0.f);
			lights[SCALE_A_SELECT_RED_LIGHT + i * 2].setBrightness(active ? 1.f : 0.f);
		}

		for (int i = 0; i < 8; ++i) {
			const bool active = i == currentStep;
			const bool accent = (i == 0 || i == 4);
			const bool enabled = stepEnabled[i];
			lights[STEP_1_R_LIGHT + i * 3].setBrightness(active ? 1.f : (accent && enabled ? 0.15f : 0.03f));
			lights[STEP_1_G_LIGHT + i * 3].setBrightness(active && enabled ? 0.85f : (enabled ? 0.12f : 0.f));
			lights[STEP_1_B_LIGHT + i * 3].setBrightness(active ? 0.18f : 0.f);
			lights[STEP_1_ENABLE_LIGHT + i].setBrightness(enabled && !active ? 0.9f : 0.f);
			lights[STEP_1_ACTIVE_RED_LIGHT + i].setBrightness(active ? 1.f : 0.f);
		}
	}
};

constexpr std::array<const char*, 12> Circles::rootNames;
constexpr std::array<Circles::Scale, 8> Circles::scales;

struct CirclesDiceButton : LEDButton {
	Circles* circlesModule = nullptr;

	void onButton(const event::Button& e) override {
		LEDButton::onButton(e);
		if (circlesModule && e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS)
			circlesModule->rollDice();
	}
};

struct CirclesDisplay : TransparentWidget {
	Circles* module = nullptr;
	std::shared_ptr<Font> font;
	std::shared_ptr<Svg> lcdRingSvg;
	NSVGshape* lcdRingShape = nullptr;
	std::vector<NSVGpath*> lcdLedPaths;
	std::array<float, Circles::PARAMS_LEN> observedParams{};
	bool observedParamsInitialized = false;
	int displayedParam = -1;
	double parameterDisplayUntil = 0.0;

	bool isDisplayedKnob(int paramId) const {
		return (paramId >= Circles::STEP_1_PARAM && paramId <= Circles::SUB_SHIFT_PARAM)
			|| paramId == Circles::FLOW_PARAM
			|| paramId == Circles::SPEED_PARAM;
	}

	void step() override {
		TransparentWidget::step();
		if (!module)
			return;

		if (!observedParamsInitialized) {
			for (int i = 0; i < Circles::PARAMS_LEN; ++i)
				observedParams[i] = module->params[i].getValue();
			observedParamsInitialized = true;
			return;
		}

		const bool diceActive = module->params[Circles::DICE_PARAM].getValue() > 0.5f;
		for (int i = 0; i < Circles::PARAMS_LEN; ++i) {
			const float value = module->params[i].getValue();
			if (!diceActive && isDisplayedKnob(i) && std::fabs(value - observedParams[i]) > 1e-5f) {
				displayedParam = i;
				parameterDisplayUntil = system::getTime() + 1.35;
			}
			observedParams[i] = value;
		}
	}

	void getParameterText(int paramId, std::string& label, std::string& value) {
		if (!module)
			return;
		if (paramId >= Circles::STEP_1_PARAM && paramId <= Circles::STEP_8_PARAM) {
			const int stepIndex = paramId - Circles::STEP_1_PARAM;
			int semitone = module->getNoteSemitones(stepIndex, module->getActiveScaleIndex()) % 12;
			if (semitone < 0)
				semitone += 12;
			label = string::f("STEP %d", stepIndex + 1);
			value = Circles::rootNames[semitone];
			return;
		}

		switch (paramId) {
			case Circles::ROOT_PARAM:
				label = "ROOT";
				value = Circles::rootNames[module->getRoot()];
				break;
			case Circles::RANGE_PARAM:
				label = "RANGE";
				value = string::f("%d OCTAVES", module->getRange());
				break;
			case Circles::LENGTH_PARAM:
				label = "GATE";
				value = string::f("%d%%", (int) std::round(module->params[paramId].getValue() * 100.f));
				break;
			case Circles::SUB_STEP_PARAM:
				label = "SUB STEP";
				value = string::f("STEP %d", (int) std::round(module->params[paramId].getValue()));
				break;
			case Circles::SCALE_A_PARAM:
			case Circles::SCALE_B_PARAM:
			case Circles::SCALE_C_PARAM:
			case Circles::SCALE_D_PARAM: {
				const int slot = paramId - Circles::SCALE_A_PARAM;
				const int scale = clamp((int) std::round(module->params[paramId].getValue()), 0, 7);
				label = string::f("SCALE %c", 'A' + slot);
				value = Circles::scales[scale].name;
				break;
			}
			case Circles::SCALE_BARS_PARAM:
				label = "SCALE BARS";
				value = string::f("%d BARS", (int) std::round(module->params[paramId].getValue()));
				break;
			case Circles::SUB_EDIT_SLOT_PARAM:
				label = "EDIT SUB";
				value = string::f("SCALE %c", 'A' + clamp((int) std::round(module->params[paramId].getValue()), 0, 3));
				break;
			case Circles::SUB_BARS_PARAM:
				label = "SUB BARS";
				value = string::f("%d BARS", (int) std::round(module->params[paramId].getValue()));
				break;
			case Circles::SUB_SHIFT_PARAM:
				label = "SUB SHIFT";
				value = string::f("STEP %d", (int) std::round(module->params[paramId].getValue()) + 1);
				break;
			case Circles::FLOW_PARAM: {
				static const std::array<const char*, 5> names = {{"FORWARD", "REVERSE", "PENDULUM", "DRUNK", "RANDOM"}};
				label = "FLOW";
				value = names[module->getFlow()];
				break;
			}
			case Circles::SPEED_PARAM: {
				static const std::array<const char*, 4> names = {{"1/4", "1/8", "1/16", "1/32"}};
				label = "SPEED";
				value = names[module->getSpeed()];
				break;
			}
			default:
				label.clear();
				value.clear();
				break;
		}
	}

	void loadLcdRing() {
		if (lcdRingSvg)
			return;

		// This SVG must be unique per widget because its NanoSVG path list is
		// temporarily isolated below while the individual LEDs are drawn.
		lcdRingSvg = std::make_shared<Svg>();
		lcdRingSvg->loadFile(asset::plugin(pluginInstance, "res/CirclesComponents.svg"));
		if (!lcdRingSvg->handle)
			return;

		for (NSVGshape* shape = lcdRingSvg->handle->shapes; shape; shape = shape->next) {
			if (std::strcmp(shape->id, "Led_ring") == 0) {
				lcdRingShape = shape;
				break;
			}
		}
		if (!lcdRingShape)
			return;

		for (NSVGpath* path = lcdRingShape->paths; path; path = path->next)
			lcdLedPaths.push_back(path);

		const Vec ringCenter(144.240f, 148.555f);
		std::sort(lcdLedPaths.begin(), lcdLedPaths.end(), [&](const NSVGpath* a, const NSVGpath* b) {
			auto clockwiseAngle = [&](const NSVGpath* path) {
				const float x = (path->bounds[0] + path->bounds[2]) * 0.5f - ringCenter.x;
				const float y = (path->bounds[1] + path->bounds[3]) * 0.5f - ringCenter.y;
				float angle = std::atan2(x, -y);
				return angle < 0.f ? angle + 2.f * M_PI : angle;
			};
			return clockwiseAngle(a) < clockwiseAngle(b);
		});
	}

	void drawLcdRing(const DrawArgs& args) {
		loadLcdRing();
		if (!lcdRingSvg || !lcdRingSvg->handle || !lcdRingShape || lcdLedPaths.empty())
			return;

		NSVGimage* image = lcdRingSvg->handle;
		NSVGshape* savedFirstShape = image->shapes;
		NSVGshape* savedNextShape = lcdRingShape->next;
		NSVGpath* savedFirstPath = lcdRingShape->paths;
		const NSVGpaint savedFill = lcdRingShape->fill;
		const NSVGpaint savedStroke = lcdRingShape->stroke;
		const float savedOpacity = lcdRingShape->opacity;

		image->shapes = lcdRingShape;
		lcdRingShape->next = nullptr;
		lcdRingShape->stroke.type = NSVG_PAINT_NONE;
		lcdRingShape->fill.type = NSVG_PAINT_COLOR;

		const int ledCount = static_cast<int>(lcdLedPaths.size());
		float ringPosition = -1.f;
		const bool hasRingPosition = module && module->currentStep >= 0;
		if (hasRingPosition) {
			float displayStep = static_cast<float>(module->currentStep);
			if (module->lcdPreviousStep >= 0 && module->previousMainStepPeriod > 0) {
				int travelDirection = module->currentStep - module->lcdPreviousStep;
				if (travelDirection > 4)
					travelDirection -= 8;
				else if (travelDirection < -4)
					travelDirection += 8;
				const float progress = clamp(
					module->samplesSinceMainStep / static_cast<float>(module->previousMainStepPeriod),
					0.f, 1.f);
				// Start exactly at the active red step, then travel continuously
				// toward the next expected position for the entire step period.
				displayStep = module->currentStep + travelDirection * progress;
			}
			ringPosition = displayStep * ledCount / 8.f;
		}
		const float firstLitPosition = ringPosition - 2.f;
		const int firstBaseSegment = static_cast<int>(std::floor(firstLitPosition));
		const float crossfade = firstLitPosition - firstBaseSegment;
		for (int segment = 0; segment < ledCount; ++segment) {
			NSVGpath* path = lcdLedPaths[segment];
			NSVGpath* savedNextPath = path->next;
			lcdRingShape->paths = path;
			path->next = nullptr;

			float brightness = 0.f;
			if (hasRingPosition) {
				const int base = (firstBaseSegment % ledCount + ledCount) % ledCount;
				const int nextBase = (base + 1) % ledCount;
				const int distanceFromBase = (segment - base + ledCount) % ledCount;
				const int distanceFromNextBase = (segment - nextBase + ledCount) % ledCount;
				if (distanceFromBase < 4)
					brightness += 1.f - crossfade;
				if (distanceFromNextBase < 4)
					brightness += crossfade;
			}
			const unsigned int red = static_cast<unsigned int>(std::round(89.f + 166.f * brightness));
			const unsigned int green = static_cast<unsigned int>(std::round(89.f + 166.f * brightness));
			const unsigned int blue = static_cast<unsigned int>(std::round(57.f * (1.f - brightness)));
			lcdRingShape->fill.color = 0xff000000u | (blue << 16) | (green << 8) | red;
			lcdRingShape->opacity = 1.f;

			nvgSave(args.vg);
			nvgTranslate(args.vg, -89.240f, -93.555f);
			lcdRingSvg->draw(args.vg);
			nvgRestore(args.vg);
			path->next = savedNextPath;
		}

		lcdRingShape->paths = savedFirstPath;
		lcdRingShape->fill = savedFill;
		lcdRingShape->stroke = savedStroke;
		lcdRingShape->opacity = savedOpacity;
		lcdRingShape->next = savedNextShape;
		image->shapes = savedFirstShape;
	}

	void draw(const DrawArgs& args) override {
		if (!font)
			font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font)
			return;

		const Vec center = box.size.div(2.f);
		drawLcdRing(args);

		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		const NVGcolor white = nvgRGB(255, 255, 255);
		const NVGcolor yellow = nvgRGB(255, 255, 0);
		const NVGcolor dim = nvgRGB(89, 89, 57);
		auto drawTwoTone = [&](float y, float size, const std::string& first, NVGcolor firstColor,
			const std::string& second, NVGcolor secondColor) {
			nvgFontSize(args.vg, size);
			float firstBounds[4];
			float secondBounds[4];
			nvgTextBounds(args.vg, 0.f, 0.f, first.c_str(), nullptr, firstBounds);
			nvgTextBounds(args.vg, 0.f, 0.f, second.c_str(), nullptr, secondBounds);
			const float gap = size * 0.65f;
			const float firstWidth = firstBounds[2] - firstBounds[0];
			const float secondWidth = secondBounds[2] - secondBounds[0];
			const float start = center.x - (firstWidth + gap + secondWidth) * 0.5f;
			nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
			nvgFillColor(args.vg, firstColor);
			nvgText(args.vg, start, y, first.c_str(), nullptr);
			nvgFillColor(args.vg, secondColor);
			nvgText(args.vg, start + firstWidth + gap, y, second.c_str(), nullptr);
			nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		};

		if (!module) {
			nvgBeginPath(args.vg);
			nvgCircle(args.vg, 54.847f, 21.936f, 5.883f);
			nvgCircle(args.vg, 54.847f, 53.489f, 5.883f);
			nvgFillColor(args.vg, dim);
			nvgFill(args.vg);
			nvgFillColor(args.vg, yellow);
			nvgFontSize(args.vg, 11.f);
			nvgText(args.vg, 54.847f, 21.936f, "1", nullptr);
			nvgText(args.vg, 54.847f, 53.489f, "A", nullptr);
			drawTwoTone(35.2f, 10.f, "NOTE", white, "C", yellow);
			drawTwoTone(67.2f, 10.f, "C", yellow, "MINOR", white);
			nvgFillColor(args.vg, yellow);
			nvgFontSize(args.vg, 10.f);
			nvgText(args.vg, center.x, 83.3f, "--- BPM", nullptr);
			return;
		}

		const bool showParameter = displayedParam >= 0 && system::getTime() < parameterDisplayUntil;
		if (showParameter) {
			std::string label;
			std::string value;
			getParameterText(displayedParam, label, value);
			if (!label.empty()) {
				nvgBeginPath(args.vg);
				nvgCircle(args.vg, center.x, center.y, 34.06f);
				nvgStrokeColor(args.vg, dim);
				nvgStrokeWidth(args.vg, 4.f);
				nvgStroke(args.vg);
				nvgFillColor(args.vg, white);
				nvgFontSize(args.vg, 10.f);
				nvgText(args.vg, center.x, 46.7f, label.c_str(), nullptr);
				nvgFillColor(args.vg, yellow);
				nvgText(args.vg, center.x, 58.0f, value.c_str(), nullptr);
				return;
			}
		}

		nvgBeginPath(args.vg);
		nvgCircle(args.vg, 54.847f, 21.936f, 5.883f);
		nvgCircle(args.vg, 54.847f, 53.489f, 5.883f);
		nvgFillColor(args.vg, dim);
		nvgFill(args.vg);

		nvgFillColor(args.vg, yellow);
		nvgFontSize(args.vg, 11.f);
		const std::string stepText = module->currentStep >= 0 ? string::f("%d", module->currentStep + 1) : "-";
		nvgText(args.vg, 54.847f, 21.936f, stepText.c_str(), nullptr);
		const std::string chainText = string::f("%c", 'A' + module->activeScaleSlot);
		nvgText(args.vg, 54.847f, 53.489f, chainText.c_str(), nullptr);

		drawTwoTone(35.2f, 10.f, "NOTE", white, module->getCurrentNoteName(), yellow);
		const int activeScale = module->getActiveScaleIndex();
		drawTwoTone(67.2f, 10.f, Circles::rootNames[module->getRoot()], yellow,
			Circles::scales[activeScale].name, white);

		std::string bpmText = "--- BPM";
		if (module->tempoValid && module->quarterPeriod > 0) {
			const float bpm = 60.f * APP->engine->getSampleRate() / module->quarterPeriod;
			bpmText = string::f("%d BPM", (int) std::round(bpm));
		}
		nvgFillColor(args.vg, yellow);
		nvgFontSize(args.vg, 10.f);
		nvgText(args.vg, center.x, 83.3f, bpmText.c_str(), nullptr);
	}
};

struct CirclesPanelLabels : TransparentWidget {
	std::shared_ptr<Font> font;

	void draw(const DrawArgs& args) override {
		if (!font)
			font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font)
			return;

		nvgFontFaceId(args.vg, font->handle);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		auto label = [&](float x, float y, float size, const char* text, NVGcolor color) {
			nvgFontSize(args.vg, size);
			nvgFillColor(args.vg, color);
			nvgText(args.vg, x, y, text, nullptr);
		};

		const NVGcolor textColor = nvgRGB(220, 220, 215);
		label(240.f, 20.f, 17.f, "CIRCLES V2", textColor);
		label(365.f, 56.f, 10.f, "SCALE CHAIN", textColor);
		label(308.f, 116.f, 9.f, "SCALE A", textColor);
		label(422.f, 116.f, 9.f, "SCALE B", textColor);
		label(308.f, 186.f, 9.f, "SCALE C", textColor);
		label(422.f, 186.f, 9.f, "SCALE D", textColor);
		label(280.f, 58.f, 8.f, "A ON", textColor);
		label(394.f, 58.f, 8.f, "B ON", textColor);
		label(280.f, 128.f, 8.f, "C ON", textColor);
		label(394.f, 128.f, 8.f, "D ON", textColor);
		label(308.f, 256.f, 9.f, "SUB BARS", textColor);
		label(365.f, 256.f, 9.f, "EDIT", textColor);
		label(422.f, 256.f, 9.f, "BARS", textColor);
		label(288.f, 313.f, 8.f, "DICE", textColor);
		label(327.f, 313.f, 8.f, "SPEED", textColor);
		label(376.f, 313.f, 9.f, "FLOW", textColor);
		label(427.f, 313.f, 8.f, "FLOW CV", textColor);
		label(45.f, 324.f, 9.f, "ROOT", textColor);
		label(102.f, 324.f, 9.f, "RANGE", textColor);
		label(159.f, 324.f, 9.f, "GATE", textColor);
		label(216.f, 324.f, 9.f, "SUB STEP", textColor);
		label(257.f, 324.f, 8.f, "SUB SHIFT", textColor);

		const NVGcolor portColor = nvgRGB(185, 198, 0);
		label(30.f, 370.f, 8.f, "CLOCK", portColor);
		label(72.f, 370.f, 8.f, "RESET", portColor);
		label(114.f, 370.f, 8.f, "TRANSPOSE", portColor);
		label(177.f, 370.f, 8.f, "V/OCT", portColor);
		label(220.f, 370.f, 8.f, "TRIG", portColor);
		label(263.f, 370.f, 8.f, "GATE", portColor);
		label(306.f, 370.f, 8.f, "ACCENT", portColor);
		label(349.f, 370.f, 8.f, "EOC", portColor);
		label(402.f, 370.f, 8.f, "SUB V/OCT", portColor);
		label(447.f, 370.f, 8.f, "SUB GATE", portColor);
	}
};

struct CirclesWidget : SubmitModuleWidget {
	CirclesWidget(Circles* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Circles.svg")));

#ifdef SUBMIT_LAB_BUILD
		auto* betaBadge = new SvgWidget;
		betaBadge->setSvg(Svg::load(asset::plugin(pluginInstance, "res/beta-submit-lab.svg")));
		betaBadge->box.pos = Vec((box.size.x - betaBadge->box.size.x) * 0.5f, 364.f);
		addChild(betaBadge);
#endif

		const std::array<Vec, 8> notePositions = {{
			Vec(144.240f, 72.620f), Vec(198.182f, 94.672f),
			Vec(220.391f, 148.771f), Vec(198.182f, 202.366f),
			Vec(144.240f, 224.706f), Vec(90.297f, 202.366f),
			Vec(68.088f, 148.771f), Vec(90.297f, 94.672f)
		}};
		const std::array<Vec, 8> enablePositions = {{
			Vec(144.291f, 36.816f), Vec(222.784f, 69.655f),
			Vec(255.317f, 148.771f), Vec(222.784f, 227.426f),
			Vec(144.115f, 259.587f), Vec(64.832f, 227.390f),
			Vec(33.001f, 148.359f), Vec(65.082f, 69.655f)
		}};
		const std::array<Vec, 8> cvPositions = {{
			Vec(115.526f, 40.953f), Vec(199.955f, 51.982f),
			Vec(252.111f, 119.667f), Vec(240.875f, 204.446f),
			Vec(173.127f, 256.236f), Vec(88.524f, 245.449f),
			Vec(36.779f, 177.714f), Vec(47.671f, 93.071f)
		}};
		for (int i = 0; i < 8; ++i) {
			addParam(createParamCentered<CirclesKnob>(notePositions[i], module, Circles::STEP_1_PARAM + i));
			addParam(createParamCentered<LEDButton>(enablePositions[i], module, Circles::STEP_1_ENABLE_PARAM + i));
			addChild(createLightCentered<MediumLight<YellowLight>>(enablePositions[i], module, Circles::STEP_1_ENABLE_LIGHT + i));
			addChild(createLightCentered<MediumLight<RedLight>>(enablePositions[i], module, Circles::STEP_1_ACTIVE_RED_LIGHT + i));
			addInput(createInputCentered<PJ301MPort>(cvPositions[i], module, Circles::STEP_1_CV_INPUT + i));
		}

		auto* display = new CirclesDisplay;
		display->module = module;
		display->box.pos = Vec(89.240f, 93.555f);
		display->box.size = Vec(110.f, 110.f);
		addChild(display);

		addParam(createParamCentered<CirclesSmallKnob>(Vec(307.813f, 66.322f), module, Circles::ROOT_PARAM));
		addParam(createParamCentered<CirclesSmallKnob>(Vec(298.916f, 115.947f), module, Circles::RANGE_PARAM));
		addParam(createParamCentered<CirclesSmallKnob>(Vec(308.705f, 166.808f), module, Circles::LENGTH_PARAM));
		addParam(createParamCentered<CirclesSmallKnob>(Vec(35.633f, 256.890f), module, Circles::FLOW_PARAM));
		addInput(createInputCentered<PJ301MPort>(Vec(64.183f, 276.775f), module, Circles::FLOW_CV_INPUT));

		const std::array<Vec, 4> scalePositions = {{
			Vec(409.787f, 68.264f), Vec(381.346f, 97.048f),
			Vec(380.734f, 137.170f), Vec(409.787f, 165.774f)
		}};
		const std::array<Vec, 4> scaleEnablePositions = {{
			Vec(395.996f, 34.782f), Vec(348.490f, 82.687f),
			Vec(348.240f, 149.885f), Vec(396.120f, 197.515f)
		}};
		for (int i = 0; i < 4; ++i) {
			addParam(createParamCentered<CirclesSmallKnob>(scalePositions[i], module, Circles::SCALE_A_PARAM + i));
			addParam(createParamCentered<LEDButton>(scaleEnablePositions[i], module, Circles::SCALE_A_ENABLE_PARAM + i));
			addChild(createLightCentered<MediumLight<YellowRedLight>>(scaleEnablePositions[i], module, Circles::SCALE_A_SELECT_YELLOW_LIGHT + i * 2));
		}
		addInput(createInputCentered<PJ301MPort>(Vec(429.383f, 117.149f), module, Circles::NEXT_INPUT));
		addParam(createParamCentered<CirclesSmallKnob>(Vec(336.799f, 209.520f), module, Circles::SCALE_BARS_PARAM));
		addParam(createParamCentered<CirclesSmallKnob>(Vec(380.397f, 237.186f), module, Circles::SPEED_PARAM));

		addParam(createParamCentered<CirclesSmallKnob>(Vec(222.706f, 288.780f), module, Circles::SUB_EDIT_SLOT_PARAM));
		addParam(createParamCentered<CirclesSmallKnob>(Vec(279.514f, 265.698f), module, Circles::SUB_STEP_PARAM));
		addParam(createParamCentered<CirclesSmallKnob>(Vec(340.773f, 265.698f), module, Circles::SUB_BARS_PARAM));
		addParam(createParamCentered<CirclesSmallKnob>(Vec(397.935f, 289.086f), module, Circles::SUB_SHIFT_PARAM));

		auto* diceButton = createParamCentered<CirclesDiceButton>(Vec(38.429f, 40.179f), module, Circles::DICE_PARAM);
		diceButton->circlesModule = module;
		addParam(diceButton);
		addChild(createLightCentered<MediumLight<YellowLight>>(Vec(38.429f, 40.179f), module, Circles::DICE_LIGHT));

		addInput(createInputCentered<PJ301MPort>(Vec(34.928f, 342.001f), module, Circles::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(71.921f, 342.001f), module, Circles::RESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(108.913f, 342.001f), module, Circles::TRANSPOSE_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(215.462f, 342.001f), module, Circles::VOCT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(249.147f, 342.001f), module, Circles::TRIG_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(282.832f, 342.001f), module, Circles::GATE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(316.517f, 342.001f), module, Circles::ACCENT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(380.061f, 342.001f), module, Circles::SUB_VOCT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(412.827f, 341.935f), module, Circles::SUB_GATE_OUTPUT));

	}

	void appendContextMenu(Menu* menu) override {
		auto* circlesModule = dynamic_cast<Circles*>(module);
		if (circlesModule) {
			menu->addChild(new MenuSeparator);
			menu->addChild(createIndexSubmenuItem("Dice character",
				{"Subtle", "Musical", "Wild"},
				[circlesModule]() {
					return (size_t) circlesModule->diceCharacter;
				},
				[circlesModule](size_t character) {
					circlesModule->diceCharacter = clamp((int) character, 0, 2);
				}));
		}
		SubmitModuleWidget::appendContextMenu(menu);
	}
};

Model* modelCircles = createModel<Circles, CirclesWidget>("Circles");
