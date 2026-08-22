#include "plugin.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

struct Tag : Module {
	static constexpr size_t MAX_CHARACTERS = 38;

	enum ParamId { PARAMS_LEN };
	enum InputId { INPUTS_LEN };
	enum OutputId { OUTPUTS_LEN };
	enum LightId { LIGHTS_LEN };
	enum ArrowDirection { ARROW_NONE, ARROW_LEFT, ARROW_RIGHT };

	std::string label = "PATCH SECTION";
	int arrowDirection = ARROW_RIGHT;

	Tag() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
	}

	void setLabel(const std::string& value) {
		label = value.substr(0, MAX_CHARACTERS);
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "label", json_string(label.c_str()));
		json_object_set_new(root, "arrowDirection", json_integer(arrowDirection));
		return root;
	}

	void dataFromJson(json_t* root) override {
		if (json_t* labelJson = json_object_get(root, "label")) {
			if (const char* value = json_string_value(labelJson))
				setLabel(value);
		}
		if (json_t* arrowJson = json_object_get(root, "arrowDirection"))
			arrowDirection = clamp(static_cast<int>(json_integer_value(arrowJson)), ARROW_NONE, ARROW_RIGHT);
	}

	void process(const ProcessArgs& args) override {
		(void) args;
	}
};

struct TagTextField : ui::TextField {
	Tag* module = nullptr;

	void step() override {
		ui::TextField::step();
		if (!module)
			return;
		if (text.size() > Tag::MAX_CHARACTERS) {
			text.resize(Tag::MAX_CHARACTERS);
			cursor = std::min(cursor, static_cast<int>(text.size()));
			selection = std::min(selection, static_cast<int>(text.size()));
		}
		module->setLabel(text);
	}
};

struct TagInlineTextField : ui::TextField {
	Tag* module = nullptr;
	std::shared_ptr<Font> font;

	TagInlineTextField() {
		multiline = false;
	}

	int getTextPosition(Vec mousePos) override {
		if (!font)
			font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font || font->handle < 0)
			return 0;

		NVGcontext* vg = APP->window->vg;
		nvgFontFaceId(vg, font->handle);
		nvgFontSize(vg, 11.f);
		nvgTextLetterSpacing(vg, 0.3f);
		int closest = 0;
		float closestDistance = std::numeric_limits<float>::max();
		for (size_t index = 0; index <= text.size(); ++index) {
			const std::string prefix = text.substr(0, index);
			float bounds[4];
			const float position = nvgTextBounds(vg, 0.f, 15.f, prefix.c_str(), nullptr, bounds);
			const float distance = std::fabs(mousePos.x - position);
			if (distance < closestDistance) {
				closestDistance = distance;
				closest = static_cast<int>(index);
			}
		}
		return closest;
	}

	void step() override {
		ui::TextField::step();
		if (text.size() > Tag::MAX_CHARACTERS) {
			text.resize(Tag::MAX_CHARACTERS);
			cursor = std::min(cursor, static_cast<int>(text.size()));
			selection = std::min(selection, static_cast<int>(text.size()));
		}
		if (module)
			module->setLabel(text);
	}

	void draw(const DrawArgs& args) override {
		(void) args;
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			if (!font)
				font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
			if (font && font->handle >= 0) {
				nvgScissor(args.vg, 0.f, 0.f, box.size.x, box.size.y);
				nvgFontFaceId(args.vg, font->handle);
				nvgFontSize(args.vg, 11.f);
				nvgTextLetterSpacing(args.vg, 0.3f);
				nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
				const NVGcolor yellow = nvgRGB(255, 255, 0);
				const int begin = std::min(cursor, selection);
				const int end = std::max(cursor, selection);
				if (this == APP->event->selectedWidget && begin != end) {
					float beginBounds[4];
					float endBounds[4];
					const std::string beforeSelection = text.substr(0, begin);
					const std::string throughSelection = text.substr(0, end);
					const float beginX = nvgTextBounds(args.vg, 0.f, 15.f, beforeSelection.c_str(), nullptr, beginBounds);
					const float endX = nvgTextBounds(args.vg, 0.f, 15.f, throughSelection.c_str(), nullptr, endBounds);
					nvgBeginPath(args.vg);
					nvgRect(args.vg, beginX, 8.f, endX - beginX, 14.f);
					nvgFillColor(args.vg, nvgRGBA(255, 255, 255, 65));
					nvgFill(args.vg);
				}

				nvgFillColor(args.vg, yellow);
				nvgText(args.vg, 0.f, 15.f, text.c_str(), nullptr);

				if (this == APP->event->selectedWidget) {
					float cursorBounds[4];
					const std::string beforeCursor = text.substr(0, cursor);
					const float cursorX = nvgTextBounds(args.vg, 0.f, 15.f, beforeCursor.c_str(), nullptr, cursorBounds);
					nvgBeginPath(args.vg);
					nvgMoveTo(args.vg, cursorX, 8.f);
					nvgLineTo(args.vg, cursorX, 22.f);
					nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
					nvgStrokeWidth(args.vg, 1.25f);
					nvgStroke(args.vg);
				}
				nvgResetScissor(args.vg);
			}
		}
		Widget::drawLayer(args, layer);
	}
};

struct TagMouseTransform : Widget {
	float transform[6];
	float inverse[6];
	float inverseLinear[6];
	bool hasInverse = true;

	TagMouseTransform() {
		nvgTransformIdentity(transform);
		updateInverse();
	}

	void updateInverse() {
		hasInverse = nvgTransformInverse(inverse, transform);
		std::memcpy(inverseLinear, inverse, 4 * sizeof(float));
		inverseLinear[4] = inverseLinear[5] = 0.f;
	}

	void rotate(float angle) {
		float rotation[6];
		nvgTransformRotate(rotation, angle);
		nvgTransformPremultiply(transform, rotation);
		updateInverse();
	}

	Rect getChildrenBoundingBox() override {
		Rect bounds = Widget::getChildrenBoundingBox();
		Vec topLeft = bounds.pos;
		Vec bottomRight = bounds.getBottomRight();
		nvgTransformPoint(&topLeft.x, &topLeft.y, transform, topLeft.x, topLeft.y);
		nvgTransformPoint(&bottomRight.x, &bottomRight.y, transform, bottomRight.x, bottomRight.y);
		return Rect(topLeft, bottomRight.minus(topLeft));
	}

	void draw(const DrawArgs& args) override {
		DrawArgs transformed = args;
		nvgTransform(transformed.vg, transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
		Widget::draw(transformed);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		DrawArgs transformed = args;
		nvgTransform(transformed.vg, transform[0], transform[1], transform[2], transform[3], transform[4], transform[5]);
		Widget::drawLayer(transformed, layer);
	}

	void onButton(const event::Button& e) override {
		event::Button transformed = e;
		if (hasInverse)
			nvgTransformPoint(&transformed.pos.x, &transformed.pos.y, inverse, e.pos.x, e.pos.y);
		Widget::onButton(transformed);
	}

	void onHover(const event::Hover& e) override {
		event::Hover transformed = e;
		if (hasInverse) {
			nvgTransformPoint(&transformed.pos.x, &transformed.pos.y, inverse, e.pos.x, e.pos.y);
			nvgTransformPoint(&transformed.mouseDelta.x, &transformed.mouseDelta.y,
				inverseLinear, e.mouseDelta.x, e.mouseDelta.y);
		}
		Widget::onHover(transformed);
	}

	void onDragHover(const event::DragHover& e) override {
		event::DragHover transformed = e;
		if (hasInverse) {
			nvgTransformPoint(&transformed.pos.x, &transformed.pos.y, inverse, e.pos.x, e.pos.y);
			nvgTransformPoint(&transformed.mouseDelta.x, &transformed.mouseDelta.y,
				inverseLinear, e.mouseDelta.x, e.mouseDelta.y);
		}
		Widget::onDragHover(transformed);
	}
};

struct TagDisplay : TransparentWidget {
	Tag* module = nullptr;

	void draw(const DrawArgs& args) override {
		nvgStrokeColor(args.vg, nvgRGB(255, 255, 255));
		nvgStrokeWidth(args.vg, 1.f);
		nvgLineCap(args.vg, NVG_ROUND);
		nvgLineJoin(args.vg, NVG_ROUND);

		const int direction = module ? module->arrowDirection : Tag::ARROW_RIGHT;
		const float arrowY = 69.094f;
		if (direction != Tag::ARROW_NONE) {
			nvgBeginPath(args.vg);
			if (direction == Tag::ARROW_LEFT) {
				nvgMoveTo(args.vg, 20.684f, arrowY);
				nvgLineTo(args.vg, 9.324f, arrowY);
				nvgLineTo(args.vg, 16.438f, 64.563f);
				nvgMoveTo(args.vg, 9.324f, arrowY);
				nvgLineTo(args.vg, 16.438f, 73.625f);
			}
			else {
				nvgMoveTo(args.vg, 9.316f, arrowY);
				nvgLineTo(args.vg, 20.676f, arrowY);
				nvgLineTo(args.vg, 13.562f, 64.563f);
				nvgMoveTo(args.vg, 20.676f, arrowY);
				nvgLineTo(args.vg, 13.562f, 73.625f);
			}
			nvgStroke(args.vg);
		}

	}

	void onButton(const event::Button& e) override {
		if (!module || e.button != GLFW_MOUSE_BUTTON_LEFT || e.action != GLFW_PRESS) {
			TransparentWidget::onButton(e);
			return;
		}

		if (e.pos.y >= 54.f && e.pos.y <= 82.f) {
			module->arrowDirection = (module->arrowDirection + 1) % 3;
			e.consume(this);
			return;
		}

		TransparentWidget::onButton(e);
	}
};

struct TagWidget : SubmitModuleWidget {
	TagWidget(Tag* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Tag.svg")));
		auto* display = createWidget<TagDisplay>(Vec(0.f, 0.f));
		display->box.size = box.size;
		display->module = module;
		addChild(display);

		auto* transform = createWidget<TagMouseTransform>(Vec(15.f, 86.f));
		transform->rotate(float(M_PI) * 0.5f);
		auto* field = createWidget<TagInlineTextField>(Vec(0.f, -15.f));
		field->box.size = Vec(264.f, 30.f);
		field->module = module;
		field->setText(module ? module->label : "PATCH SECTION");
		field->cursor = static_cast<int>(field->text.size());
		field->selection = field->cursor;
		transform->addChild(field);
		addChild(transform);
	}

	void appendContextMenu(Menu* menu) override {
		Tag* module = dynamic_cast<Tag*>(this->module);
		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Label (maximum 38 characters)"));
		auto* field = new TagTextField;
		field->module = module;
		field->box.size = Vec(220.f, 28.f);
		field->setText(module ? module->label : "");
		menu->addChild(field);
		menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Top arrow"));
		menu->addChild(createCheckMenuItem("None", "", [module]() { return module && module->arrowDirection == Tag::ARROW_NONE; }, [module]() { if (module) module->arrowDirection = Tag::ARROW_NONE; }));
		menu->addChild(createCheckMenuItem("Left", "", [module]() { return module && module->arrowDirection == Tag::ARROW_LEFT; }, [module]() { if (module) module->arrowDirection = Tag::ARROW_LEFT; }));
		menu->addChild(createCheckMenuItem("Right", "", [module]() { return module && module->arrowDirection == Tag::ARROW_RIGHT; }, [module]() { if (module) module->arrowDirection = Tag::ARROW_RIGHT; }));
		appendSubmitLinks(menu, "https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/tag/");
		SubmitModuleWidget::appendContextMenu(menu);
	}
};

Model* modelTag = createModel<Tag, TagWidget>("Tag");
