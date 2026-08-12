#include "plugin.hpp"
#include "FlipEngine.hpp"

#include <cmath>

// Gedeelde Submit-knop met dezelfde draaihoek als de andere modules.
struct FlipLengthKnob : SvgKnob {
	FlipLengthKnob() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobSmall.svg")));
		shadow->opacity = 0.f;
	}
};

struct Flip : Module {
	enum ParamId {
		GATE_BUTTON_PARAM,
		DRY_WET_PARAM,
		FREEZE_PARAM,
		LENGTH_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		AUDIO_L_INPUT,
		CLOCK_INPUT,
		GATE_INPUT,
		AUDIO_R_INPUT,
		FREEZE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		AUDIO_L_OUTPUT,
		AUDIO_R_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		FLIP_LIGHT,
		FREEZE_LIGHT,
		LIGHTS_LEN
	};

	static constexpr float MAX_CAPTURE_SECONDS = 8.f;
	static constexpr float MAX_FREEZE_SECONDS = 8.f;

	FlipEngine engine;
	FlipGridClock gridClock;
	FlipGridClock alignmentGridClock;
	FlipEngine freezeEngine;
	FlipGridClock freezeGridClock;
	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger gateTrigger;
	bool externalSequenceActive = false;
	bool flipButtonActive = false;
	bool pendingFlipButtonState = false;
	bool freezeActive = false;
	bool pendingFreezeState = false;
	int flipAlignmentTestMode = 0;
	int freezeAlignmentTestMode = 1;
	int flipChangeDelayBoundaries = 0;
	int freezeChangeDelayBoundaries = 0;
	int activeLengthIndex = 0;
	int pendingLengthIndex = 0;
	float smoothedWetMix = 0.f;
	float smoothedFreezeMix = 0.f;
	float lastFrozenWetLeft = 0.f;
	float lastFrozenWetRight = 0.f;
	bool haveFrozenWet = false;
	float mixSlewPerSample = 1.f;

	Flip() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(GATE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Flip", {"Off", "On"});
		// Retained without a widget so older patches keep their parameter IDs.
		configParam(DRY_WET_PARAM, 0.f, 1.f, 1.f, "Legacy Dry/Wet");
		configSwitch(FREEZE_PARAM, 0.f, 1.f, 0.f, "Freeze", {"Off", "On"});
		configSwitch(LENGTH_PARAM, 0.f, 2.f, 0.f, "Length", {"1/2", "1/4", "1/8"});
		configInput(AUDIO_L_INPUT, "Audio In L");
		configInput(AUDIO_R_INPUT, "Audio In R");
		configInput(CLOCK_INPUT, "Clock In (1 PPQN)");
		configInput(GATE_INPUT, "Gate In");
		configInput(FREEZE_INPUT, "Freeze Gate");
		configOutput(AUDIO_L_OUTPUT, "Audio Out L");
		configOutput(AUDIO_R_OUTPUT, "Audio Out R");
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		const std::size_t capacity = static_cast<std::size_t>(
			std::ceil(e.sampleRate * MAX_CAPTURE_SECONDS));
		const std::size_t freezeCapacity = static_cast<std::size_t>(
			std::ceil(e.sampleRate * MAX_FREEZE_SECONDS));
		engine.setCapacity(capacity);
		freezeEngine.setCapacity(freezeCapacity);
		engine.setDeClickSamples(static_cast<std::size_t>(std::ceil(e.sampleRate * 0.002f)));
		freezeEngine.setDeClickSamples(
			static_cast<std::size_t>(std::ceil(e.sampleRate * 0.002f)));
		mixSlewPerSample = 1.f / std::max(1.f, e.sampleRate * 0.004f);
		clockTrigger.reset();
		gateTrigger.reset();
		gridClock.reset();
		alignmentGridClock.reset();
		freezeGridClock.reset();
		externalSequenceActive = false;
		flipButtonActive = false;
		pendingFlipButtonState = false;
		freezeActive = false;
		pendingFreezeState = false;
		flipChangeDelayBoundaries = 0;
		freezeChangeDelayBoundaries = 0;
		activeLengthIndex = 0;
		pendingLengthIndex = 0;
		smoothedWetMix = 0.f;
		smoothedFreezeMix = 0.f;
		lastFrozenWetLeft = 0.f;
		lastFrozenWetRight = 0.f;
		haveFrozenWet = false;
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		engine.reset();
		freezeEngine.reset();
		clockTrigger.reset();
		gateTrigger.reset();
		gridClock.reset();
		alignmentGridClock.reset();
		freezeGridClock.reset();
		externalSequenceActive = false;
		flipButtonActive = false;
		pendingFlipButtonState = false;
		freezeActive = false;
		pendingFreezeState = false;
		flipChangeDelayBoundaries = 0;
		freezeChangeDelayBoundaries = 0;
		activeLengthIndex = 0;
		pendingLengthIndex = 0;
		smoothedWetMix = 0.f;
		smoothedFreezeMix = 0.f;
		lastFrozenWetLeft = 0.f;
		lastFrozenWetRight = 0.f;
		haveFrozenWet = false;
	}

	static void updateQuantizedState(bool requestedState, int alignmentTestMode,
		bool& activeState, int& delayedBoundaries) {
		if (alignmentTestMode == 0) {
			activeState = requestedState;
			delayedBoundaries = 0;
		}
		else if (requestedState == activeState) {
			delayedBoundaries = 0;
		}
		else if (delayedBoundaries > 0) {
			activeState = requestedState;
			delayedBoundaries = 0;
		}
		else {
			delayedBoundaries = 1;
		}
	}

	void process(const ProcessArgs& args) override {
		const bool externalGateEdge = gateTrigger.process(inputs[GATE_INPUT].getVoltage());
		const bool requestedFlipButtonState = params[GATE_BUTTON_PARAM].getValue() > 0.5f;
		const bool requestedFreezeState = params[FREEZE_PARAM].getValue() > 0.5f
			|| inputs[FREEZE_INPUT].getVoltage() >= 1.f;
		const bool rawClockEdge = clockTrigger.process(inputs[CLOCK_INPUT].getVoltage());
		const int requestedLengthIndex = clamp(static_cast<int>(std::round(
			params[LENGTH_PARAM].getValue())), 0, 2);

		if (externalGateEdge)
			externalSequenceActive = true;

		// Keep both segment buffers warm in the background. This makes Flip instant
		// after the first grid cell has been captured, even while switched Off.
		if (engine.getState() == FlipEngine::State::IDLE)
			engine.arm();
		if (freezeEngine.getState() == FlipEngine::State::IDLE)
			freezeEngine.arm();

		// Submit clock standard is 1 PPQN. Faster capture lengths are derived
		// internally and remain phase-locked to each incoming quarter-note edge.
		// Length is deliberately ignored while Freeze is Off. Live reverse must
		// always capture and play a complete quarter-note cycle; otherwise a short
		// transient such as a kick can be captured in a different window just by
		// moving the Freeze-only timing control.
		const bool segmentBoundary = gridClock.process(rawClockEdge, 1);
		const bool offbeatBoundary =
			alignmentGridClock.process(rawClockEdge, 2) && !rawClockEdge;
		// The front-panel Flip switch is quantized to the same clock that drives
		// reverse playback. A mid-cell click is remembered and takes effect only
		// on the next boundary, so a kick is never cut in half.
		pendingFlipButtonState = requestedFlipButtonState;
		pendingFreezeState = requestedFreezeState;
		pendingLengthIndex = requestedLengthIndex;
		const bool freezeWasActive = freezeActive;
		if (segmentBoundary) {
			if (flipAlignmentTestMode != 2)
				updateQuantizedState(pendingFlipButtonState, flipAlignmentTestMode,
					flipButtonActive, flipChangeDelayBoundaries);
			if (freezeAlignmentTestMode != 2)
				updateQuantizedState(pendingFreezeState, freezeAlignmentTestMode,
					freezeActive, freezeChangeDelayBoundaries);
			// Length is a Freeze-only control and changes only on a complete
			// quarter-note boundary. Turning the snapped knob can therefore never
			// restart a frozen window halfway through a cell.
			activeLengthIndex = pendingLengthIndex;
		}
		if (offbeatBoundary) {
			if (flipAlignmentTestMode == 2) {
				flipButtonActive = pendingFlipButtonState;
				flipChangeDelayBoundaries = 0;
			}
			if (freezeAlignmentTestMode == 2) {
				freezeActive = pendingFreezeState;
				freezeChangeDelayBoundaries = 0;
			}
		}
		if (freezeWasActive && !freezeActive) {
			// Re-arm the dedicated path on a complete quarter-note edge after
			// Freeze release. This guarantees that the next frozen source is a
			// full cell, even if the previous session used 1/2, 1/4, 1/8 or offbeat.
			freezeEngine.reset();
			freezeEngine.arm();
			freezeGridClock.reset();
		}
		const bool freezeBoundary = freezeGridClock.process(
			rawClockEdge, flipGridDivision(freezeActive, activeLengthIndex))
			|| (freezeAlignmentTestMode == 2 && offbeatBoundary);
		const std::size_t frozenCellSamples = freezeActive
			? freezeGridClock.getCellSamples() : 0;

		const float inputLeft = inputs[AUDIO_L_INPUT].getVoltage();
		const float inputRight = inputs[AUDIO_R_INPUT].isConnected()
			? inputs[AUDIO_R_INPUT].getVoltage()
			: inputLeft;
		// The live reverse engine never sees Freeze or Length. It therefore keeps
		// the proven phase and timing continuously, even while the frozen path is
		// audible. The separate frozen engine may be changed without retiming live.
		const FlipEngine::Frame liveFrame =
			engine.process(inputLeft, inputRight, segmentBoundary, true, false, 0);
		const FlipEngine::Frame frozenFrame =
			freezeEngine.process(inputLeft, inputRight, freezeBoundary, true,
				freezeActive, frozenCellSamples);

		const float liveWetLeft = liveFrame.playbackSample ? liveFrame.outputLeft : 0.f;
		const float liveWetRight = liveFrame.playbackSample ? liveFrame.outputRight : 0.f;
		// Until the dedicated path has a valid frame, fall back to the already
		// phase-locked live reverse rather than creating a silent transition.
		if (frozenFrame.playbackSample) {
			lastFrozenWetLeft = frozenFrame.outputLeft;
			lastFrozenWetRight = frozenFrame.outputRight;
			haveFrozenWet = true;
		}
		const bool holdFrozenTail = !freezeActive && haveFrozenWet
			&& smoothedFreezeMix > 0.f;
		const float frozenWetLeft = frozenFrame.playbackSample
			? frozenFrame.outputLeft : (holdFrozenTail ? lastFrozenWetLeft : liveWetLeft);
		const float frozenWetRight = frozenFrame.playbackSample
			? frozenFrame.outputRight : (holdFrozenTail ? lastFrozenWetRight : liveWetRight);
		const float targetFreezeMix = freezeActive ? 1.f : 0.f;
		if (smoothedFreezeMix < targetFreezeMix)
			smoothedFreezeMix = std::min(
				targetFreezeMix, smoothedFreezeMix + mixSlewPerSample);
		else if (smoothedFreezeMix > targetFreezeMix)
			smoothedFreezeMix = std::max(
				targetFreezeMix, smoothedFreezeMix - mixSlewPerSample);
		const float selectedWetLeft = liveWetLeft * (1.f - smoothedFreezeMix)
			+ frozenWetLeft * smoothedFreezeMix;
		const float selectedWetRight = liveWetRight * (1.f - smoothedFreezeMix)
			+ frozenWetRight * smoothedFreezeMix;

		// Slew only the amplitude mix. The reverse read position and target beat
		// remain untouched, so switching cannot introduce a timing offset.
		// Freeze is a held version of the reverse path. It must therefore make
		// Flip audible even when the front-panel Flip switch is Off. Releasing
		// Freeze returns to the actual Flip switch/gate state.
		const bool flipActive = flipButtonActive || externalSequenceActive || freezeActive;
		const float targetWetMix = flipActive ? 1.f : 0.f;
		if (smoothedWetMix < targetWetMix)
			smoothedWetMix = std::min(targetWetMix, smoothedWetMix + mixSlewPerSample);
		else if (smoothedWetMix > targetWetMix)
			smoothedWetMix = std::max(targetWetMix, smoothedWetMix - mixSlewPerSample);

		outputs[AUDIO_L_OUTPUT].setVoltage(
			inputLeft * (1.f - smoothedWetMix) + selectedWetLeft * smoothedWetMix);
		outputs[AUDIO_R_OUTPUT].setVoltage(
			inputRight * (1.f - smoothedWetMix) + selectedWetRight * smoothedWetMix);

		// De leds tonen de werkelijk hoorbare toestand. Freeze gebruikt de Flip-route,
		// waardoor bij Freeze zowel de groene FRZ-led als de rode FLIP-led brandt.
		lights[FLIP_LIGHT].setSmoothBrightness(flipActive ? 1.f : 0.f, args.sampleTime);
		lights[FREEZE_LIGHT].setSmoothBrightness(freezeActive ? 1.f : 0.f, args.sampleTime);

		if (externalSequenceActive && segmentBoundary && !externalGateEdge)
			externalSequenceActive = false;
	}
};

struct FlipPanelBackground : TransparentWidget {
	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0.f, 0.f, box.size.x, box.size.y);
		nvgFillColor(args.vg, nvgRGB(0x26, 0x26, 0x26));
		nvgFill(args.vg);
	}
};

struct FlipPanelBorder : TransparentWidget {
	void draw(const DrawArgs& args) override {
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0.5f, 0.5f, box.size.x - 1.f, box.size.y - 1.f);
		nvgStrokeColor(args.vg, nvgRGB(0x55, 0x55, 0x55));
		nvgStrokeWidth(args.vg, 1.f);
		nvgStroke(args.vg);
	}
};

struct FlipWidget : SubmitModuleWidget {
	explicit FlipWidget(Flip* module) {
		setModule(module);
		auto* panel = createPanel(asset::plugin(pluginInstance, "res/Flip.svg"));
		setPanel(panel);
		panel->panelBorder->hide();

		// Het Illustrator-paneel heeft exact dezelfde ontwerpmaat als Sync:
		// 68.346 x 380. Het blijft 1:1 en staat gecentreerd in een 5HP-module.
		box.size.x = 75.f;
		panel->box.pos.x = 3.327f;
		auto* background = createWidget<FlipPanelBackground>(Vec(0.f, 0.f));
		background->box.size = Vec(75.f, RACK_GRID_HEIGHT);
		addChildBottom(background);
		auto* border = createWidget<FlipPanelBorder>(Vec(0.f, 0.f));
		border->box.size = Vec(75.f, RACK_GRID_HEIGHT);
		addChild(border);

		// Exacte posities uit Panel-design-Flip-componenets.svg, met alleen
		// de 3.327 px horizontale centreerruimte van het 5HP-frame erbij.
		addInput(createInputCentered<PJ301MPort>(Vec(36.078f, 81.807f), module, Flip::CLOCK_INPUT));

		addParam(createParamCentered<VCVLatch>(Vec(23.033f, 129.515f), module, Flip::GATE_BUTTON_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(23.033f, 129.515f), module, Flip::FLIP_LIGHT));
		addInput(createInputCentered<PJ301MPort>(Vec(51.662f, 129.515f), module, Flip::GATE_INPUT));

		addParam(createParamCentered<VCVLatch>(Vec(23.033f, 188.861f), module, Flip::FREEZE_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(23.033f, 188.861f), module, Flip::FREEZE_LIGHT));
		addInput(createInputCentered<PJ301MPort>(Vec(51.662f, 188.861f), module, Flip::FREEZE_INPUT));

		auto* lengthKnob = createParamCentered<FlipLengthKnob>(
			Vec(36.494f, 242.016f), module, Flip::LENGTH_PARAM);
		lengthKnob->snap = true;
		addParam(lengthKnob);

		addInput(createInputCentered<PJ301MPort>(Vec(21.594f, 302.037f), module, Flip::AUDIO_L_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(53.998f, 302.037f), module, Flip::AUDIO_L_OUTPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(21.594f, 342.001f), module, Flip::AUDIO_R_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(53.998f, 342.001f), module, Flip::AUDIO_R_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuItem("Manual", "", []() {
			system::openBrowser("https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/flip/");
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

Model* modelFlip = createModel<Flip, FlipWidget>("Flip");
