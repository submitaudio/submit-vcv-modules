#include "plugin.hpp"

#include <cmath>

struct Pulse : Module {
	enum InputId { LED_INPUT, MIX_INPUT, INPUTS_LEN };
	enum LightId { METER_R_LIGHT, METER_G_LIGHT, METER_B_LIGHT, LIGHTS_LEN };

	dsp::SchmittTrigger ledTrigger;
	float envelope = 0.f;
	float peakHold = 0.f;
	float flashRemaining = 0.f;
	float flashBrightness = 0.f;
	float meterLevel = 0.f;
	bool peakActive = false;

	Pulse() {
		config(0, INPUTS_LEN, 0, LIGHTS_LEN);
		configInput(LED_INPUT, "Yellow pulse trigger");
		configInput(MIX_INPUT, "Mono final mix level monitor");
		configLight(METER_R_LIGHT, "Mix level and peak warning");
	}

	void process(const ProcessArgs& args) override {
		if (ledTrigger.process(inputs[LED_INPUT].getVoltage()))
			flashRemaining = 0.20f;
		flashRemaining = std::max(0.f, flashRemaining - args.sampleTime);
		flashBrightness = flashRemaining > 0.06f ? 1.f : flashRemaining / 0.06f;

		const float absoluteVoltage = std::fabs(inputs[MIX_INPUT].getVoltage());
		const float time = absoluteVoltage > envelope ? 0.005f : 0.30f;
		const float coefficient = 1.f - std::exp(-args.sampleTime / time);
		envelope += (absoluteVoltage - envelope) * coefficient;

		if (absoluteVoltage >= 4.8f)
			peakHold = 0.50f;
		else
			peakHold = std::max(0.f, peakHold - args.sampleTime);

		float red = 0.f;
		float green = 0.f;
		float blue = 0.f;
		const float level = clamp(envelope / 5.f, 0.f, 1.f);
		if (envelope > 0.0001f) {
			const float decibels = 20.f * std::log10(envelope / 4.8f);
			meterLevel = clamp((decibels + 48.f) / 48.f, 0.f, 1.f);
		}
		else {
			meterLevel = 0.f;
		}
		red = level;
		green = level;

		if (flashBrightness > 0.f) {
			red = std::max(red, flashBrightness);
			green = std::max(green, 0.85f * flashBrightness);
		}
		peakActive = peakHold > 0.f;
		if (peakActive) {
			red = 0.50f + 0.50f * flashBrightness;
			green = 0.07f + 0.07f * flashBrightness;
		}

		lights[METER_R_LIGHT].setSmoothBrightness(red, args.sampleTime);
		lights[METER_G_LIGHT].setSmoothBrightness(green, args.sampleTime);
		lights[METER_B_LIGHT].setSmoothBrightness(blue, args.sampleTime);
	}
};

struct PulseLightField : TransparentWidget {
	Pulse* module = nullptr;

	void draw(const DrawArgs& args) override {
		const float width = box.size.x;
		const float height = box.size.y;
		const float radius = 5.f;
		const float flash = module ? module->flashBrightness : 0.f;
		const bool yellow = flash > 0.f;
		const bool peak = module && module->peakActive;
		const float meterLevel = module ? module->meterLevel : 0.f;

		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.f, 0.f, width, height, radius);
		nvgFillColor(args.vg, nvgRGB(10, 11, 10));
		nvgFill(args.vg);

		if (yellow || peak) {
			const NVGcolor colour = peak ? nvgRGB(255, 38, 0) : nvgRGB(255, 255, 0);
			const float peakBrightness = peak ? 0.50f + 0.50f * flash : flash;
			nvgBeginPath(args.vg);
			nvgRoundedRect(args.vg, 1.5f, 1.5f, width - 3.f, height - 3.f, radius - 1.f);
			NVGpaint glow = nvgLinearGradient(args.vg, 0.f, height, 0.f, 0.f,
				nvgRGBAf(colour.r, colour.g, colour.b, peak ? peakBrightness : 1.f),
				nvgRGBAf(colour.r, colour.g, colour.b, peak ? peakBrightness * 0.78f : 0.90f));
			nvgFillPaint(args.vg, glow);
			nvgFill(args.vg);
			if (yellow && !peak) {
				nvgBeginPath(args.vg);
				nvgRoundedRect(args.vg, 4.f, 3.f, width - 8.f, height - 6.f, radius - 2.f);
				nvgFillColor(args.vg, nvgRGBA(255, 255, 190, static_cast<int>(105.f * flash)));
				nvgFill(args.vg);
			}
		}
		else if (meterLevel > 0.001f) {
			const float top = height * (1.f - meterLevel);
			nvgBeginPath(args.vg);
			nvgMoveTo(args.vg, width * 0.5f, height - 3.f);
			nvgLineTo(args.vg, width * 0.5f, top);
			nvgStrokeColor(args.vg, nvgRGB(255, 255, 0));
			nvgStrokeWidth(args.vg, 1.4f);
			nvgStroke(args.vg);
		}

		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0.5f, 0.5f, width - 1.f, height - 1.f, radius);
		nvgStrokeColor(args.vg, nvgRGBA(180, 180, 180, 150));
		nvgStrokeWidth(args.vg, 1.5f);
		nvgStroke(args.vg);
	}
};

struct PulseWidget : SubmitModuleWidget {
	PulseWidget(Pulse* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Pulse.svg")));

		auto* lightField = createWidget<PulseLightField>(Vec(5.622f, 58.745f));
		lightField->box.size = Vec(18.683f, 209.696f);
		lightField->module = module;
		addChild(lightField);

		addInput(createInputCentered<PJ301MPort>(Vec(14.888f, 301.630f), module, Pulse::LED_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(14.888f, 342.225f), module, Pulse::MIX_INPUT));
	}

	void appendContextMenu(Menu* menu) override {
		appendSubmitLinks(menu, "https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/pulse/");
		SubmitModuleWidget::appendContextMenu(menu);
	}
};

Model* modelPulse = createModel<Pulse, PulseWidget>("Pulse");
