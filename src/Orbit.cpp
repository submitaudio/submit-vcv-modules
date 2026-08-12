#include "plugin.hpp"

#include <array>

#include <algorithm>
#include <cmath>
#include <cstdint>

// Compacte gedeelde Submit-knop voor het Orbit-grid.
struct OrbitKnob : SvgKnob {
	OrbitKnob() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobTiny.svg")));
		shadow->opacity = 0.f;
	}
};

struct Orbit : Module {
	static constexpr int HOLD_BEATS = 4;
	static constexpr int HOLD_OUTPUTS = 3;
	static constexpr int HOLD_EVENTS_PER_BEAT = 16;
	static constexpr int HOLD_CV_SAMPLES_PER_BEAT = 64;

	struct HeldBeat {
		std::array<std::array<float, HOLD_EVENTS_PER_BEAT>, HOLD_OUTPUTS> phase {};
		std::array<uint8_t, HOLD_OUTPUTS> count {};
	};

	enum ParamId {
		WINDOW_PARAM,
		POSITION_PARAM,
		MEMORY_PARAM,
		HOLD_PARAM,
		RANGE_PARAM,
		PARAMS_LEN
	};
	enum InputId { A_INPUT, B_INPUT, CLOCK_INPUT, RESET_INPUT, INPUTS_LEN };
	enum OutputId { ALIGN_OUTPUT, BETWEEN_OUTPUT, GAP_OUTPUT, TENSION_OUTPUT, DISTANCE_OUTPUT, OUTPUTS_LEN };
	enum LightId { A_LIGHT, B_LIGHT, ALIGN_LIGHT, HOLD_LIGHT, LIGHTS_LEN };

	dsp::SchmittTrigger aTrigger, bTrigger, clockTrigger, resetTrigger, holdTrigger;
	dsp::PulseGenerator alignPulse, betweenPulse, gapPulse;
	dsp::PulseGenerator aLightPulse, bLightPulse, alignLightPulse;
	uint64_t sampleCounter = 0, lastA = 0, lastB = 0, lastClock = 0;
	uint64_t periodA = 0, periodB = 0, clockPeriod = 0;
	uint64_t lastActivity = 0, lastAlignOutput = 0;
	std::array<uint64_t, 4> betweenSchedule {};
	std::array<float, 8> distanceHistory {};
	std::array<HeldBeat, HOLD_BEATS> captureBar {};
	std::array<HeldBeat, HOLD_BEATS> completedBar {};
	std::array<HeldBeat, HOLD_BEATS> frozenBar {};
	std::array<std::array<float, HOLD_CV_SAMPLES_PER_BEAT>, HOLD_BEATS> captureTension {};
	std::array<std::array<float, HOLD_CV_SAMPLES_PER_BEAT>, HOLD_BEATS> completedTension {};
	std::array<std::array<float, HOLD_CV_SAMPLES_PER_BEAT>, HOLD_BEATS> frozenTension {};
	std::array<std::array<uint64_t, HOLD_EVENTS_PER_BEAT>, HOLD_OUTPUTS> replayDue {};
	std::array<uint8_t, HOLD_OUTPUTS> replayDueCount {};
	int distanceHistoryWrite = 0, distanceHistoryCount = 0;
	int captureBeat = 0, completedBeatCount = 0, replayBeat = 0;
	uint32_t exactAlignCandidateCount = 0;
	bool haveA = false, haveB = false, haveClock = false;
	bool held = false, relationValid = false, gapArmed = false, completedBarReady = false;
	bool frozenTensionReady = false;
	float distanceTarget = 0.f, distanceValue = 0.f;
	float holdFallbackTensionVoltage = 0.f;
	float smoothingCoefficient = 0.001f;
	uint8_t controlDivider = 0;

	Orbit() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(WINDOW_PARAM, 0.f, 1.f, 0.77119f, "Relationship window", "%", 0.f, 100.f);
		configParam(POSITION_PARAM, 0.05f, 0.95f, 0.7413f, "Position between rhythms", "%", 0.f, 100.f);
		configParam(MEMORY_PARAM, 0.f, 1.f, 0.29061f, "Relationship memory", "%", 0.f, 100.f);
		configButton(HOLD_PARAM, "Hold all outputs");
		configParam(RANGE_PARAM, 0.0625f, 2.f, 1.2021f, "Tension range", " beats");
		configInput(A_INPUT, "Rhythm A");
		configInput(B_INPUT, "Rhythm B");
		configInput(CLOCK_INPUT, "Clock (1 PPQN)");
		configInput(RESET_INPUT, "Reset");
		configLight(A_LIGHT, "Rhythm A trigger");
		configLight(B_LIGHT, "Rhythm B trigger");
		configOutput(ALIGN_OUTPUT, "Aligned events");
		configOutput(BETWEEN_OUTPUT, "Events between A and B");
		configOutput(GAP_OUTPUT, "Detected rhythmic gaps");
		configOutput(TENSION_OUTPUT, "Rhythmic tension CV");
		configOutput(DISTANCE_OUTPUT, "Rhythmic distance CV");
	}

	void clearReplaySchedule() {
		for (auto& due : replayDue)
			due.fill(0);
		replayDueCount.fill(0);
	}

	void recordHeldEvent(int outputIndex) {
		if (held || outputIndex < 0 || outputIndex >= HOLD_OUTPUTS
			|| !haveClock || clockPeriod < 2 || sampleCounter < lastClock)
			return;
		HeldBeat& beat = captureBar[captureBeat];
		uint8_t& count = beat.count[outputIndex];
		if (count >= HOLD_EVENTS_PER_BEAT)
			return;
		const float phase = clamp(static_cast<float>(sampleCounter - lastClock)
			/ static_cast<float>(clockPeriod), 0.f, 0.9999f);
		beat.phase[outputIndex][count++] = phase;
	}

	float liveTensionVoltage() const {
		return relationValid ? 10.f * (1.f - distanceValue) : 0.f;
	}

	void recordTensionFrame() {
		if (held || !haveClock || clockPeriod < 2 || sampleCounter < lastClock)
			return;
		const float phase = clamp(static_cast<float>(sampleCounter - lastClock)
			/ static_cast<float>(clockPeriod), 0.f, 0.9999f);
		const int index = clamp(static_cast<int>(phase * HOLD_CV_SAMPLES_PER_BEAT),
			0, HOLD_CV_SAMPLES_PER_BEAT - 1);
		captureTension[captureBeat][index] = liveTensionVoltage();
	}

	float frozenTensionVoltage() const {
		if (!frozenTensionReady || !haveClock || clockPeriod < 2 || sampleCounter < lastClock)
			return holdFallbackTensionVoltage;
		const float phase = clamp(static_cast<float>(sampleCounter - lastClock)
			/ static_cast<float>(clockPeriod), 0.f, 0.9999f);
		const float position = phase * HOLD_CV_SAMPLES_PER_BEAT;
		const int index = clamp(static_cast<int>(position), 0, HOLD_CV_SAMPLES_PER_BEAT - 1);
		const float fraction = position - static_cast<float>(index);
		const float current = frozenTension[replayBeat][index];
		const float next = index + 1 < HOLD_CV_SAMPLES_PER_BEAT
			? frozenTension[replayBeat][index + 1]
			: frozenTension[(replayBeat + 1) % HOLD_BEATS][0];
		return current + (next - current) * fraction;
	}

	void fireAlign() {
		alignPulse.trigger(1e-3f);
		alignLightPulse.trigger(70e-3f);
		recordHeldEvent(0);
	}

	void fireBetween() {
		betweenPulse.trigger(1e-3f);
		recordHeldEvent(1);
	}

	void fireGap() {
		gapPulse.trigger(1e-3f);
		recordHeldEvent(2);
	}

	void scheduleFrozenBeat(int beatIndex, uint64_t beatStart, float firstPhase) {
		clearReplaySchedule();
		if (clockPeriod < 2)
			return;
		const HeldBeat& beat = frozenBar[clamp(beatIndex, 0, HOLD_BEATS - 1)];
		for (int outputIndex = 0; outputIndex < HOLD_OUTPUTS; ++outputIndex) {
			for (int i = 0; i < beat.count[outputIndex]; ++i) {
				const float phase = beat.phase[outputIndex][i];
				if (phase + 1e-6f < firstPhase)
					continue;
				const uint64_t due = beatStart + static_cast<uint64_t>(
					std::llround(static_cast<double>(clockPeriod) * phase));
				if (due < sampleCounter)
					continue;
				uint8_t& count = replayDueCount[outputIndex];
				if (count < HOLD_EVENTS_PER_BEAT)
					replayDue[outputIndex][count++] = due;
			}
		}
	}

	void freezeCapturedBar() {
		frozenBar = completedBarReady ? completedBar : captureBar;
		frozenTension = completedBarReady ? completedTension : captureTension;
		frozenTensionReady = completedBarReady;
		replayBeat = captureBeat;
		clearReplaySchedule();
		if (haveClock && clockPeriod > 1 && sampleCounter >= lastClock) {
			const float currentPhase = clamp(static_cast<float>(sampleCounter - lastClock)
				/ static_cast<float>(clockPeriod), 0.f, 1.f);
			scheduleFrozenBeat(replayBeat, lastClock, currentPhase);
		}
	}

	void processFrozenEvents() {
		for (int outputIndex = 0; outputIndex < HOLD_OUTPUTS; ++outputIndex) {
			for (int i = 0; i < replayDueCount[outputIndex]; ++i) {
				uint64_t& due = replayDue[outputIndex][i];
				if (due == 0 || sampleCounter < due)
					continue;
				due = 0;
				if (outputIndex == 0)
					fireAlign();
				else if (outputIndex == 1)
					fireBetween();
				else
					fireGap();
			}
		}
	}

	void advanceCaptureBeat() {
		completedBeatCount = std::min(completedBeatCount + 1, HOLD_BEATS);
		const int nextBeat = (captureBeat + 1) % HOLD_BEATS;
		if (nextBeat == 0 && completedBeatCount >= HOLD_BEATS) {
			completedBar = captureBar;
			completedTension = captureTension;
			completedBarReady = true;
		}
		captureBeat = nextBeat;
		captureBar[captureBeat] = HeldBeat {};
		captureTension[captureBeat].fill(liveTensionVoltage());
	}

	void clearTiming() {
		lastA = lastB = lastClock = 0;
		periodA = periodB = clockPeriod = 0;
		lastActivity = lastAlignOutput = 0;
		betweenSchedule.fill(0);
		distanceHistory.fill(0.f);
		for (auto& beat : captureBar) beat = HeldBeat {};
		for (auto& beat : completedBar) beat = HeldBeat {};
		for (auto& beat : frozenBar) beat = HeldBeat {};
		for (auto& beat : captureTension) beat.fill(0.f);
		for (auto& beat : completedTension) beat.fill(0.f);
		for (auto& beat : frozenTension) beat.fill(0.f);
		clearReplaySchedule();
		distanceHistoryWrite = distanceHistoryCount = 0;
		captureBeat = completedBeatCount = replayBeat = 0;
		exactAlignCandidateCount = 0;
		haveA = haveB = haveClock = relationValid = gapArmed = completedBarReady = false;
		frozenTensionReady = false;
		distanceTarget = distanceValue = 0.f;
		holdFallbackTensionVoltage = 0.f;
		alignPulse.reset();
		betweenPulse.reset();
		gapPulse.reset();
	}

	void silenceTriggerActivity() {
		betweenSchedule.fill(0);
		clearReplaySchedule();
		gapArmed = false;
		alignPulse.reset();
		betweenPulse.reset();
		gapPulse.reset();
		aLightPulse.reset();
		bLightPulse.reset();
		alignLightPulse.reset();
	}

	void restartTimingAfterHold() {
		lastA = lastB = lastClock = 0;
		periodA = periodB = clockPeriod = 0;
		lastActivity = lastAlignOutput = 0;
		haveA = haveB = haveClock = gapArmed = false;
		for (auto& beat : captureBar) beat = HeldBeat {};
		for (auto& beat : captureTension) beat.fill(liveTensionVoltage());
		captureBeat = completedBeatCount = 0;
		silenceTriggerActivity();
	}

	uint64_t referencePeriod(float sampleRate) const {
		if (clockPeriod > 1) return clockPeriod;
		if (periodA > 1 && periodB > 1) return (periodA + periodB) / 2;
		if (periodA > 1) return periodA;
		if (periodB > 1) return periodB;
		return static_cast<uint64_t>(sampleRate * 0.5f);
	}

	void updateDistance(float sampleRate) {
		if (held || !haveA || !haveB) return;
		const uint64_t delta = lastA > lastB ? lastA - lastB : lastB - lastA;
		const float ref = static_cast<float>(std::max<uint64_t>(referencePeriod(sampleRate), 1))
			* params[RANGE_PARAM].getValue();
		const float rawDistance = clamp(static_cast<float>(delta) / ref, 0.f, 1.f);
		distanceHistory[distanceHistoryWrite] = rawDistance;
		distanceHistoryWrite = (distanceHistoryWrite + 1) % static_cast<int>(distanceHistory.size());
		distanceHistoryCount = std::min(distanceHistoryCount + 1,
			static_cast<int>(distanceHistory.size()));
		const int wantedHistory = 1 + static_cast<int>(std::round(
			params[MEMORY_PARAM].getValue() * static_cast<float>(distanceHistory.size() - 1)));
		const int historyLength = std::min(wantedHistory, distanceHistoryCount);
		float historySum = 0.f;
		for (int i = 0; i < historyLength; ++i) {
			const int index = (distanceHistoryWrite - 1 - i
				+ static_cast<int>(distanceHistory.size())) % static_cast<int>(distanceHistory.size());
			historySum += distanceHistory[index];
		}
		distanceTarget = historySum / static_cast<float>(std::max(historyLength, 1));
		relationValid = true;
	}

	void scheduleBetweenFrom(uint64_t now, uint64_t oppositeLast, uint64_t oppositePeriod) {
		if (oppositePeriod < 2) return;
		uint64_t predicted = oppositeLast;
		while (predicted <= now) predicted += oppositePeriod;
		const uint64_t candidate = now + static_cast<uint64_t>((predicted - now) * params[POSITION_PARAM].getValue());
		if (candidate <= now + 1) return;
		for (uint64_t scheduled : betweenSchedule) {
			if (scheduled > 0 && (scheduled > candidate ? scheduled - candidate : candidate - scheduled) <= 1)
				return;
		}
		for (uint64_t& scheduled : betweenSchedule) {
			if (scheduled == 0) {
				scheduled = candidate;
				return;
			}
		}
		auto latest = std::max_element(betweenSchedule.begin(), betweenSchedule.end());
		if (latest != betweenSchedule.end() && candidate < *latest) {
			*latest = candidate;
		}
	}

	void processRhythmEdge(bool isA, const ProcessArgs& args) {
		const uint64_t now = sampleCounter;
		uint64_t& last = isA ? lastA : lastB;
		uint64_t& period = isA ? periodA : periodB;
		bool& have = isA ? haveA : haveB;
		const uint64_t oppositeLast = isA ? lastB : lastA;
		const uint64_t oppositePeriod = isA ? periodB : periodA;
		const bool haveOpposite = isA ? haveB : haveA;
		if (have) {
			const uint64_t measured = now - last;
			if (measured > 1) period = period > 1 ? (period * 3 + measured) / 4 : measured;
		}
		last = now;
		have = true;
		lastActivity = now;
		gapArmed = true;
		if (isA) aLightPulse.trigger(40e-3f); else bLightPulse.trigger(40e-3f);
		if (haveOpposite) {
			const uint64_t delta = now > oppositeLast ? now - oppositeLast : oppositeLast - now;
			const uint64_t ref = std::max<uint64_t>(referencePeriod(args.sampleRate), 1);
			const float windowAmount = clamp(params[WINDOW_PARAM].getValue(), 0.f, 1.f);
			const float windowBeats = 0.125f + std::sqrt(windowAmount) * 0.875f;
			const uint64_t window = static_cast<uint64_t>(static_cast<float>(ref) * windowBeats);
			if (delta <= std::max<uint64_t>(window, 1)) {
				const bool exactAlignment = delta <= std::max<uint64_t>(ref / 100, 1);
				bool passExactThinning = true;
				if (exactAlignment) {
					++exactAlignCandidateCount;
					passExactThinning = (exactAlignCandidateCount & 1u) != 0u;
				}
				const uint64_t minimumSpacing = std::max<uint64_t>(ref / 16, 1);
				if (passExactThinning
					&& (lastAlignOutput == 0 || now - lastAlignOutput >= minimumSpacing)) {
					fireAlign();
					lastAlignOutput = now;
				}
			}
		}
		scheduleBetweenFrom(now, oppositeLast, oppositePeriod);
		updateDistance(args.sampleRate);
	}

	void process(const ProcessArgs& args) override {
		++sampleCounter;
		if (holdTrigger.process(params[HOLD_PARAM].getValue())) {
			if (!held) {
				holdFallbackTensionVoltage = outputs[TENSION_OUTPUT].getVoltage();
				held = true;
				silenceTriggerActivity();
				freezeCapturedBar();
			}
			else {
				held = false;
				restartTimingAfterHold();
			}
		}
		if (resetTrigger.process(inputs[RESET_INPUT].getVoltage())) {
			clearTiming();
			held = false;
		}
		const bool clockRise = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage());
		const bool aRise = aTrigger.process(inputs[A_INPUT].getVoltage());
		const bool bRise = bTrigger.process(inputs[B_INPUT].getVoltage());
		if (!held) {
			if (clockRise) {
				const uint64_t now = sampleCounter;
				if (haveClock) {
					const uint64_t measured = now - lastClock;
					if (measured > 1) clockPeriod = clockPeriod > 1 ? (clockPeriod * 3 + measured) / 4 : measured;
					if (lastActivity < lastClock) fireGap();
					advanceCaptureBeat();
				}
				else {
					captureBeat = completedBeatCount = 0;
					captureBar[captureBeat] = HeldBeat {};
					captureTension[captureBeat].fill(liveTensionVoltage());
				}
				lastClock = now;
				haveClock = true;
			}
			if (aRise) processRhythmEdge(true, args);
			if (bRise) processRhythmEdge(false, args);
			bool shouldFireBetween = false;
			for (uint64_t& scheduled : betweenSchedule) {
				if (scheduled > 0 && sampleCounter >= scheduled) {
					scheduled = 0;
					shouldFireBetween = true;
				}
			}
			if (shouldFireBetween)
				fireBetween();
			if (gapArmed && haveClock && clockPeriod > 1 && lastActivity > 0) {
				const float windowAmount = clamp(params[WINDOW_PARAM].getValue(), 0.f, 1.f);
				const float gapBeats = 0.5f - windowAmount * 0.375f;
				const uint64_t gapDelay = std::max<uint64_t>(
					static_cast<uint64_t>(static_cast<float>(clockPeriod) * gapBeats), 1);
				if (sampleCounter - lastActivity >= gapDelay) {
					fireGap();
					gapArmed = false;
				}
			}
			if (++controlDivider == 0) {
				smoothingCoefficient = 1.f - std::exp(-args.sampleTime / 0.02f);
			}
			distanceValue += (distanceTarget - distanceValue) * smoothingCoefficient;
			recordTensionFrame();
		}
		else {
			if (clockRise) {
				const uint64_t now = sampleCounter;
				if (haveClock) {
					const uint64_t measured = now - lastClock;
					if (measured > 1)
						clockPeriod = clockPeriod > 1 ? (clockPeriod * 3 + measured) / 4 : measured;
				}
				lastClock = now;
				haveClock = true;
				replayBeat = (replayBeat + 1) % HOLD_BEATS;
				scheduleFrozenBeat(replayBeat, now, 0.f);
			}
			processFrozenEvents();
		}
		outputs[ALIGN_OUTPUT].setVoltage(alignPulse.process(args.sampleTime) ? 10.f : 0.f);
		outputs[BETWEEN_OUTPUT].setVoltage(betweenPulse.process(args.sampleTime) ? 10.f : 0.f);
		outputs[GAP_OUTPUT].setVoltage(gapPulse.process(args.sampleTime) ? 10.f : 0.f);
		const float tensionVoltage = held ? frozenTensionVoltage() : liveTensionVoltage();
		outputs[DISTANCE_OUTPUT].setVoltage(held ? clamp(10.f - tensionVoltage, 0.f, 10.f)
			: (relationValid ? 10.f * distanceValue : 0.f));
		outputs[TENSION_OUTPUT].setVoltage(tensionVoltage);
		lights[A_LIGHT].setSmoothBrightness(aLightPulse.process(args.sampleTime) ? 1.f : 0.f, args.sampleTime);
		lights[B_LIGHT].setSmoothBrightness(bLightPulse.process(args.sampleTime) ? 1.f : 0.f, args.sampleTime);
		lights[ALIGN_LIGHT].setSmoothBrightness(alignLightPulse.process(args.sampleTime) ? 1.f : 0.f, args.sampleTime);
		lights[HOLD_LIGHT].setBrightness(held ? 1.f : 0.f);
	}
};

struct OrbitPanelBackground : TransparentWidget {
	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
		nvgFillColor(args.vg, nvgRGB(0x26, 0x26, 0x26));
		nvgFill(args.vg);
	}
};

struct OrbitPanelBorder : TransparentWidget {
	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0.5f, 0.5f, box.size.x - 1.f, box.size.y - 1.f);
		nvgStrokeColor(args.vg, nvgRGB(0x55, 0x55, 0x55));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);
	}
};

struct OrbitWidget : SubmitModuleWidget {
	OrbitWidget(Orbit* module) {
		setModule(module);
		auto* panel = createPanel(asset::plugin(pluginInstance, "res/Orbit.svg"));
		setPanel(panel);
		panel->panelBorder->hide();

		// De 68.346 x 380 Illustrator-layout blijft 1:1 en wordt, net als
		// Flip en Sync, gecentreerd in het 75 px brede 5HP-frame.
		box.size.x = 75.f;
		panel->box.pos.x = 3.327f;
		auto* background = createWidget<OrbitPanelBackground>(Vec(0.f, 0.f));
		background->box.size = Vec(75.f, RACK_GRID_HEIGHT);
		addChildBottom(background);
		auto* border = createWidget<OrbitPanelBorder>(Vec(0.f, 0.f));
		border->box.size = Vec(75.f, RACK_GRID_HEIGHT);
		addChild(border);

		// Exacte 1:1-posities uit Panel-design-Orbit-components.svg, plus
		// uitsluitend de 3.327 px horizontale centreerruimte.
		addParam(createParamCentered<OrbitKnob>(Vec(22.193f, 90.539f), module, Orbit::POSITION_PARAM));
		addParam(createParamCentered<OrbitKnob>(Vec(53.59f, 90.539f), module, Orbit::WINDOW_PARAM));
		addParam(createParamCentered<OrbitKnob>(Vec(22.193f, 133.659f), module, Orbit::MEMORY_PARAM));
		addParam(createParamCentered<OrbitKnob>(Vec(53.59f, 133.659f), module, Orbit::RANGE_PARAM));

		addParam(createParamCentered<LEDButton>(Vec(37.679f, 173.225f), module, Orbit::HOLD_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(37.679f, 173.225f), module, Orbit::HOLD_LIGHT));

		addInput(createInputCentered<PJ301MPort>(Vec(21.594f, 219.189f), module, Orbit::A_INPUT));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(14.500f, 201.300f), module, Orbit::A_LIGHT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(53.998f, 219.189f), module, Orbit::ALIGN_OUTPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(21.594f, 259.152f), module, Orbit::B_INPUT));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(14.500f, 241.300f), module, Orbit::B_LIGHT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(53.998f, 259.152f), module, Orbit::BETWEEN_OUTPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(21.594f, 302.037f), module, Orbit::CLOCK_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(53.998f, 302.037f), module, Orbit::GAP_OUTPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(21.594f, 342.001f), module, Orbit::RESET_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(53.998f, 342.001f), module, Orbit::TENSION_OUTPUT));
		// DISTANCE_OUTPUT blijft bewust intern bestaan voor oude beta-patches,
		// maar is redundant aan TENSION en heeft geen jack op het compacte paneel.
	}
};

Model* modelOrbit = createModel<Orbit, OrbitWidget>("Orbit");
