#include "plugin.hpp"

#include <array>
#include <cmath>

namespace {

struct SumKnob : SvgKnob {
	SumKnob() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobMini.svg")));
		shadow->opacity = 0.f;
	}
};

struct SumMuteButton : SvgSwitch {
	SumMuteButton() {
		momentary = false;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChainMuteButton_1.svg")));
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChainMuteButton_0.svg")));
	}
};

struct ClicklessMute {
	float gain = 1.f;

	float process(bool muted, float sampleTime) {
		const float target = muted ? 0.f : 1.f;
		const float coefficient = 1.f - std::exp(-sampleTime / 0.003f);
		gain += (target - gain) * coefficient;
		return gain;
	}
};

} // namespace

struct SumM4 : Module {
	enum ParamId {
		CH1_LEVEL_PARAM, CH1_PAN_PARAM, CH1_MUTE_PARAM,
		CH2_LEVEL_PARAM, CH2_PAN_PARAM, CH2_MUTE_PARAM,
		CH3_LEVEL_PARAM, CH3_PAN_PARAM, CH3_MUTE_PARAM,
		CH4_LEVEL_PARAM, CH4_PAN_PARAM, CH4_MUTE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CH1_INPUT, CH2_INPUT, CH3_INPUT, CH4_INPUT,
		CHAIN_LEFT_INPUT, CHAIN_RIGHT_INPUT,
		INPUTS_LEN
	};
	enum OutputId { LEFT_OUTPUT, RIGHT_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	std::array<ClicklessMute, 4> mutes;

	SumM4() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for (int channel = 0; channel < 4; ++channel) {
			const int param = channel * 3;
			configParam(param, 0.f, 1.f, 0.8f, string::f("Channel %d level", channel + 1), "%", 0.f, 100.f);
			configParam(param + 1, -1.f, 1.f, 0.f, string::f("Channel %d pan", channel + 1), "%", 0.f, 100.f);
			configSwitch(param + 2, 0.f, 1.f, 0.f, string::f("Channel %d mute", channel + 1), {"On", "Muted"});
			configInput(CH1_INPUT + channel, string::f("Channel %d mono", channel + 1));
		}
		configInput(CHAIN_LEFT_INPUT, "Audio chain in left");
		configInput(CHAIN_RIGHT_INPUT, "Audio chain in right");
		configOutput(LEFT_OUTPUT, "Audio chain out left");
		configOutput(RIGHT_OUTPUT, "Audio chain out right");
	}

	void process(const ProcessArgs& args) override {
		float left = 0.f;
		float right = 0.f;
		for (int channel = 0; channel < 4; ++channel) {
			const int param = channel * 3;
			const float signal = inputs[CH1_INPUT + channel].getVoltage()
				* params[param].getValue()
				* mutes[channel].process(params[param + 2].getValue() > 0.5f, args.sampleTime);
			const float position = (params[param + 1].getValue() + 1.f) * 0.25f * float(M_PI);
			left += signal * std::cos(position);
			right += signal * std::sin(position);
		}
		outputs[LEFT_OUTPUT].setVoltage(left + inputs[CHAIN_LEFT_INPUT].getVoltage());
		outputs[RIGHT_OUTPUT].setVoltage(right + inputs[CHAIN_RIGHT_INPUT].getVoltage());
	}
};

struct SumS4 : Module {
	enum ParamId {
		CH1_LEVEL_PARAM, CH1_MUTE_PARAM,
		CH2_LEVEL_PARAM, CH2_MUTE_PARAM,
		CH3_LEVEL_PARAM, CH3_MUTE_PARAM,
		CH4_LEVEL_PARAM, CH4_MUTE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CH1_LEFT_INPUT, CH1_RIGHT_INPUT,
		CH2_LEFT_INPUT, CH2_RIGHT_INPUT,
		CH3_LEFT_INPUT, CH3_RIGHT_INPUT,
		CH4_LEFT_INPUT, CH4_RIGHT_INPUT,
		CHAIN_LEFT_INPUT, CHAIN_RIGHT_INPUT,
		INPUTS_LEN
	};
	enum OutputId { LEFT_OUTPUT, RIGHT_OUTPUT, OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };

	std::array<ClicklessMute, 4> mutes;

	SumS4() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		for (int channel = 0; channel < 4; ++channel) {
			const int param = channel * 2;
			const int input = channel * 2;
			configParam(param, 0.f, 1.f, 0.8f, string::f("Channel %d level", channel + 1), "%", 0.f, 100.f);
			configSwitch(param + 1, 0.f, 1.f, 0.f, string::f("Channel %d mute", channel + 1), {"On", "Muted"});
			configInput(input, string::f("Channel %d left", channel + 1));
			configInput(input + 1, string::f("Channel %d right", channel + 1));
		}
		configInput(CHAIN_LEFT_INPUT, "Audio chain in left");
		configInput(CHAIN_RIGHT_INPUT, "Audio chain in right");
		configOutput(LEFT_OUTPUT, "Audio chain out left");
		configOutput(RIGHT_OUTPUT, "Audio chain out right");
	}

	void process(const ProcessArgs& args) override {
		float left = 0.f;
		float right = 0.f;
		for (int channel = 0; channel < 4; ++channel) {
			const int param = channel * 2;
			const int input = channel * 2;
			const float gain = params[param].getValue()
				* mutes[channel].process(params[param + 1].getValue() > 0.5f, args.sampleTime);
			const float inputLeft = inputs[input].getVoltage();
			const float inputRight = inputs[input + 1].isConnected() ? inputs[input + 1].getVoltage() : inputLeft;
			left += inputLeft * gain;
			right += inputRight * gain;
		}
		outputs[LEFT_OUTPUT].setVoltage(left + inputs[CHAIN_LEFT_INPUT].getVoltage());
		outputs[RIGHT_OUTPUT].setVoltage(right + inputs[CHAIN_RIGHT_INPUT].getVoltage());
	}
};

struct SumM4Widget : SubmitModuleWidget {
	SumM4Widget(SumM4* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/SumM4.svg")));

		const Vec levelPositions[4] = {{7.308f, 27.389f}, {7.308f, 46.603f}, {7.308f, 65.816f}, {7.308f, 85.030f}};
		const Vec panPositions[4] = {{18.050f, 27.389f}, {18.050f, 46.603f}, {18.050f, 65.816f}, {18.050f, 85.030f}};
		const Vec mutePositions[4] = {{18.025f, 18.847f}, {18.025f, 38.046f}, {18.025f, 57.204f}, {18.025f, 76.363f}};
		const Vec inputPositions[4] = {{7.280f, 18.856f}, {7.280f, 38.070f}, {7.280f, 57.284f}, {7.280f, 76.497f}};
		for (int channel = 0; channel < 4; ++channel) {
			const int param = channel * 3;
			addParam(createParamCentered<SumKnob>(mm2px(levelPositions[channel]), module, param));
			addParam(createParamCentered<SumKnob>(mm2px(panPositions[channel]), module, param + 1));
			addParam(createParamCentered<SumMuteButton>(mm2px(mutePositions[channel]), module, param + 2));
			addInput(createInputCentered<PJ301MPort>(mm2px(inputPositions[channel]), module, SumM4::CH1_INPUT + channel));
		}
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.432f, 102.575f)), module, SumM4::CHAIN_LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.432f, 116.020f)), module, SumM4::CHAIN_RIGHT_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(18.273f, 102.575f)), module, SumM4::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(18.273f, 116.020f)), module, SumM4::RIGHT_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		appendSubmitLinks(menu, "https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/sum-m4/");
		SubmitModuleWidget::appendContextMenu(menu);
	}
};

struct SumS4Widget : SubmitModuleWidget {
	SumS4Widget(SumS4* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/SumS4.svg")));

		const Vec levelPositions[4] = {{18.050f, 27.389f}, {18.050f, 46.603f}, {18.050f, 65.816f}, {18.050f, 85.030f}};
		const Vec mutePositions[4] = {{18.025f, 18.847f}, {18.025f, 38.046f}, {18.025f, 57.204f}, {18.025f, 76.363f}};
		const Vec leftPositions[4] = {{7.280f, 18.856f}, {7.280f, 38.070f}, {7.280f, 57.284f}, {7.280f, 76.497f}};
		const Vec rightPositions[4] = {{7.280f, 27.778f}, {7.280f, 47.044f}, {7.280f, 66.089f}, {7.280f, 85.245f}};
		for (int channel = 0; channel < 4; ++channel) {
			const int param = channel * 2;
			const int input = channel * 2;
			addParam(createParamCentered<SumKnob>(mm2px(levelPositions[channel]), module, param));
			addParam(createParamCentered<SumMuteButton>(mm2px(mutePositions[channel]), module, param + 1));
			addInput(createInputCentered<PJ301MPort>(mm2px(leftPositions[channel]), module, input));
			addInput(createInputCentered<PJ301MPort>(mm2px(rightPositions[channel]), module, input + 1));
		}
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.432f, 102.575f)), module, SumS4::CHAIN_LEFT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.432f, 116.020f)), module, SumS4::CHAIN_RIGHT_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(18.273f, 102.575f)), module, SumS4::LEFT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(18.273f, 116.020f)), module, SumS4::RIGHT_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		appendSubmitLinks(menu, "https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/sum-s4/");
		SubmitModuleWidget::appendContextMenu(menu);
	}
};

Model* modelSumM4 = createModel<SumM4, SumM4Widget>("SumM4");
Model* modelSumS4 = createModel<SumS4, SumS4Widget>("SumS4");
