#include "plugin.hpp"

#include <cmath>

struct Set : Module {
	enum ParamId { RESET_PARAM, START_PARAM, PULSE_RATE_PARAM, PARAMS_LEN };
	enum InputId { CLOCK_INPUT, RESET_INPUT, START_INPUT, INPUTS_LEN };
	enum OutputId { PULSE_OUTPUT, OUTPUTS_LEN };
	enum LightId { PULSE_LIGHT, RESET_LIGHT, START_LIGHT, LIGHTS_LEN };

	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger resetTrigger;
	dsp::SchmittTrigger resetButtonTrigger;
	dsp::SchmittTrigger startInputTrigger;
	dsp::SchmittTrigger startButtonTrigger;
	dsp::PulseGenerator pulseOutput;
	dsp::PulseGenerator resetLightPulse;

	double elapsedSeconds = 0.0;
	double secondsSinceClock = 0.0;
	double lastClockPeriod = 0.0;
	float pulseLightRemaining = 0.f;
	float pulseLightBrightness = 0.f;
	int pulsePhase = 0;
	int currentBeat = 0;
	bool haveClock = false;
	bool running = false;

	Set() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configButton(RESET_PARAM, "Reset timer");
		configButton(START_PARAM, "Run or pause timer");
		configSwitch(PULSE_RATE_PARAM, 0.f, 2.f, 2.f, "Pulse interval", {"1/4", "1/2", "1/1"});
		getParamQuantity(PULSE_RATE_PARAM)->snapEnabled = true;
		configInput(CLOCK_INPUT, "Clock (1 PPQN)");
		configInput(RESET_INPUT, "Reset pulse phase");
		configInput(START_INPUT, "Start timer");
		configOutput(PULSE_OUTPUT, "Selected musical pulse");
		configLight(PULSE_LIGHT, "Selected musical pulse");
		configLight(RESET_LIGHT, "Timer reset");
		configLight(START_LIGHT, "Timer running");
	}

	void resetTimer() {
		elapsedSeconds = 0.0;
		resetLightPulse.trigger(0.10f);
	}

	void resetPulsePhase() {
		pulsePhase = 0;
		currentBeat = 0;
		pulseOutput.reset();
		pulseLightRemaining = 0.f;
		pulseLightBrightness = 0.f;
	}

	void startTimer() {
		secondsSinceClock = 0.0;
		haveClock = false;
		running = true;
	}

	void toggleTimer() {
		if (running) {
			running = false;
			secondsSinceClock = 0.0;
			haveClock = false;
		}
		else {
			startTimer();
		}
	}

	void process(const ProcessArgs& args) override {
		const bool manualReset = resetButtonTrigger.process(params[RESET_PARAM].getValue());
		const bool externalReset = resetTrigger.process(inputs[RESET_INPUT].getVoltage());
		if (manualReset)
			resetTimer();
		if (externalReset)
			resetPulsePhase();

		const bool manualStart = startButtonTrigger.process(params[START_PARAM].getValue());
		const bool externalStart = startInputTrigger.process(inputs[START_INPUT].getVoltage());
		if (manualStart)
			toggleTimer();
		if (externalStart)
			startTimer();

		if (running)
			secondsSinceClock += args.sampleTime;
		if (clockTrigger.process(inputs[CLOCK_INPUT].getVoltage())) {
			if (running && haveClock) {
				const double maximumRunningInterval = lastClockPeriod > 0.0 ? lastClockPeriod * 2.5 : 10.0;
				if (secondsSinceClock <= maximumRunningInterval) {
					elapsedSeconds += secondsSinceClock;
					lastClockPeriod = secondsSinceClock;
				}
			}
			if (running) {
				secondsSinceClock = 0.0;
				haveClock = true;
			}

			const int rate = clamp(static_cast<int>(std::round(params[PULSE_RATE_PARAM].getValue())), 0, 2);
			const int division = 1 << rate;
			currentBeat = pulsePhase;
			if (pulsePhase % division == 0) {
				pulseOutput.trigger(0.01f);
				pulseLightRemaining = 0.20f;
			}
			pulsePhase = (pulsePhase + 1) % 4;
		}
		pulseLightRemaining = std::max(0.f, pulseLightRemaining - args.sampleTime);
		pulseLightBrightness = pulseLightRemaining > 0.06f ? 1.f : pulseLightRemaining / 0.06f;

		outputs[PULSE_OUTPUT].setVoltage(pulseOutput.process(args.sampleTime) ? 10.f : 0.f);
		lights[PULSE_LIGHT].setBrightness(pulseLightBrightness);
		lights[RESET_LIGHT].setSmoothBrightness(resetLightPulse.process(args.sampleTime) ? 1.f : 0.f, args.sampleTime);
		lights[START_LIGHT].setSmoothBrightness(running ? 1.f : 0.f, args.sampleTime);
	}
};

struct SetDisplay : TransparentWidget {
	Set* module = nullptr;
	std::shared_ptr<Font> font;

	void draw(const DrawArgs& args) override {
		if (!font)
			font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font)
			return;

		double seconds = module ? module->elapsedSeconds : 0.0;
		const int totalSeconds = clamp(static_cast<int>(std::floor(seconds)), 0, 59999);
		const int minutes = totalSeconds / 60;
		const int remainder = totalSeconds % 60;
		char minutesText[5];
		char secondsText[4];
		snprintf(minutesText, sizeof(minutesText), "%03d", minutes);
		snprintf(secondsText, sizeof(secondsText), "%02d", remainder);

		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 20.f);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, nvgRGB(255, 255, 255));
		nvgText(args.vg, 37.5f, 52.6f, minutesText, nullptr);
		nvgFillColor(args.vg, nvgRGB(255, 255, 0));
		nvgText(args.vg, 37.5f, 76.4f, secondsText, nullptr);

		const float beatX[4] = {23.024f, 31.463f, 39.901f, 48.340f};
		for (int beat = 0; beat < 4; ++beat) {
			nvgBeginPath(args.vg);
			nvgRect(args.vg, beatX[beat], 88.892f, 2.012f, 8.395f);
			const bool active = module && module->currentBeat == beat;
			nvgFillColor(args.vg, active ? nvgRGB(255, 255, 0) : nvgRGB(38, 36, 17));
			nvgFill(args.vg);
		}
	}
};

struct SetWidget : SubmitModuleWidget {
	SetWidget(Set* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Set.svg")));

		auto* display = createWidget<SetDisplay>(Vec(0.f, 0.f));
		display->box.size = box.size;
		display->module = module;
		addChild(display);

		addParam(createParamCentered<LEDButton>(Vec(20.515f, 158.862f), module, Set::START_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(20.515f, 158.862f), module, Set::START_LIGHT));
		addParam(createParamCentered<LEDButton>(Vec(53.998f, 158.862f), module, Set::RESET_PARAM));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(53.998f, 158.862f), module, Set::RESET_LIGHT));
		addParam(createParamCentered<CKSSThreeHorizontal>(Vec(37.600f, 215.959f), module, Set::PULSE_RATE_PARAM));
		addChild(createLightCentered<MediumLight<YellowLight>>(Vec(37.424f, 256.845f), module, Set::PULSE_LIGHT));
		addInput(createInputCentered<PJ301MPort>(Vec(20.685f, 302.037f), module, Set::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(53.338f, 302.037f), module, Set::START_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(20.685f, 342.001f), module, Set::RESET_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(53.338f, 342.001f), module, Set::PULSE_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		appendSubmitLinks(menu, "https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/set/");
		SubmitModuleWidget::appendContextMenu(menu);
	}
};

Model* modelSet = createModel<Set, SetWidget>("Set");
