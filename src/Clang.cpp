// Copyright (c) 2026 Submit Audio (submitaudio.nl)
// Licensed under GPL v3 - see LICENSE file for details
// Clang V3 - techno micro-percussion, ticks, dust and short noise hits

#include "plugin.hpp"
#include <algorithm>
#include <cmath>

struct ClangReactSmallKnob : SvgKnob {
	ClangReactSmallKnob() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobSmall.svg")));
		shadow->opacity = 0.f;
	}
};

static constexpr int CLANG_MODELS = 8;
static constexpr int CLANG_SOUNDS = 8;
static constexpr int CLANG_VOICES = 8;

static const char* clangPhysicalModelNames[CLANG_MODELS] = {
	"Tick", "Dust", "Metal", "Wire", "Tube", "Scrap", "Fold", "Crush"
};

static const char* clangFmModelNames[CLANG_MODELS] = {
	"Ping", "Ratio", "Bell", "Arc", "Phase", "Stack", "Bits", "Fault"
};

// Keep the FM model data stable while putting Arc on the last model position.
static const int clangFmModelOrder[CLANG_MODELS] = {0, 1, 2, 4, 5, 6, 7, 3};

static const char* clangPhysicalSoundNames[CLANG_MODELS][CLANG_SOUNDS] = {
	{"Pin", "Needle", "Relay", "Switch", "Glass", "Nail", "Static", "Spark"},
	{"Air", "Sand", "Ash", "Tape", "Grain", "Steam", "Mist", "Burst"},
	{"Plate", "Rim", "Bolt", "Sheet", "Rod", "Tank", "Latch", "Shard"},
	{"String", "Cable", "Spring", "Tension", "Buzz", "Zap", "Twist", "Snap"},
	{"Pipe", "Can", "Block", "Valve", "Drum", "Bottle", "Box", "Shell"},
	{"Rust", "Scrape", "Junk", "Chain", "Debris", "Rotor", "Clack", "Crash"},
	{"Bite", "Bend", "Knife", "Pinch", "Curl", "Hard", "Scream", "Edge"},
	{"Bit", "Alias", "Shatter", "Packet", "Chip", "Fault", "Dust", "Break"},
};

static const char* clangFmSoundNames[CLANG_MODELS][CLANG_SOUNDS] = {
	{"Dot", "Pin", "Tap", "Knock", "Pluck", "Strike", "Bleep", "Shard"},
	{"1:1", "3:2", "2:1", "7:3", "5:4", "9:5", "11:7", "13:8"},
	{"Glass", "Alloy", "Thin", "Hard", "Cold", "Split", "Wrong", "Steel"},
	{"Zap", "Snap", "Whip", "Flash", "Tension", "Short", "Coil", "Spasm"},
	{"Fold", "Thru", "Cross", "Skew", "Wrap", "Bend", "Tear", "Flip"},
	{"Two", "Three", "Cluster", "Chord", "Knock", "Engine", "Cutter", "Wall"},
	{"Dust", "Bits", "Static", "Radio", "Hash", "Packet", "Click", "Rip"},
	{"4bit", "6bit", "8bit", "Alias", "Clip", "Fault", "Glitch", "Broken"},
};

struct ClangModelSpec {
	float baseFreq;
	float body;
	float noise;
	float click;
	float bodyDecay;
	float noiseDecay;
	float bright;
	float fold;
	float fm;
	float crush;
	float bend;
	float ghost;
};

struct ClangSoundSpec {
	float pitch;
	float ratio;
	float body;
	float noise;
	float click;
	float decay;
	float bright;
	float fold;
	float bend;
	float delayMs;
};

static const ClangModelSpec clangPhysicalSpecs[CLANG_MODELS] = {
	{2600.f, 0.18f, 0.22f, 1.30f, 0.22f, 0.30f, 0.95f, 0.18f, 0.15f, 0.10f, 0.18f, 0.20f}, // Tick
	{1900.f, 0.08f, 1.25f, 0.32f, 0.18f, 0.88f, 0.72f, 0.26f, 0.20f, 0.20f, 0.10f, 0.44f}, // Dust
	{620.f,  0.98f, 0.35f, 0.48f, 0.88f, 0.48f, 0.64f, 0.42f, 0.26f, 0.08f, 0.24f, 0.18f}, // Metal
	{940.f,  0.58f, 0.48f, 0.42f, 0.58f, 0.56f, 0.82f, 0.62f, 0.42f, 0.18f, 0.36f, 0.32f}, // Wire
	{185.f,  1.25f, 0.18f, 0.24f, 1.35f, 0.34f, 0.38f, 0.20f, 0.18f, 0.06f, 0.52f, 0.12f}, // Tube
	{470.f,  0.48f, 0.92f, 0.62f, 0.46f, 0.72f, 0.68f, 0.50f, 0.32f, 0.28f, 0.28f, 0.48f}, // Scrap
	{720.f,  0.62f, 0.42f, 0.34f, 0.54f, 0.48f, 0.84f, 1.05f, 0.56f, 0.22f, 0.40f, 0.26f}, // Fold
	{1100.f, 0.28f, 0.86f, 0.52f, 0.30f, 0.62f, 0.92f, 0.74f, 0.38f, 0.88f, 0.22f, 0.58f}, // Crush
};

static const ClangModelSpec clangFmSpecs[CLANG_MODELS] = {
	{1500.f, 0.62f, 0.16f, 0.74f, 0.42f, 0.24f, 0.88f, 0.22f, 1.15f, 0.08f, 0.34f, 0.18f}, // Ping
	{360.f,  0.78f, 0.12f, 0.26f, 0.92f, 0.22f, 0.46f, 0.20f, 2.20f, 0.06f, 0.58f, 0.10f}, // Ratio
	{820.f,  0.92f, 0.18f, 0.30f, 1.10f, 0.30f, 0.70f, 0.24f, 1.65f, 0.05f, 0.42f, 0.14f}, // Bell
	{1280.f, 0.42f, 0.45f, 0.62f, 0.34f, 0.46f, 0.92f, 0.72f, 2.80f, 0.22f, 0.30f, 0.40f}, // Arc
	{560.f,  0.66f, 0.24f, 0.32f, 0.62f, 0.38f, 0.74f, 0.90f, 2.40f, 0.16f, 0.66f, 0.22f}, // Phase
	{240.f,  0.86f, 0.30f, 0.24f, 1.20f, 0.44f, 0.52f, 0.38f, 1.90f, 0.10f, 0.48f, 0.18f}, // Stack
	{1800.f, 0.24f, 0.78f, 0.48f, 0.26f, 0.54f, 0.96f, 0.34f, 1.30f, 0.72f, 0.20f, 0.50f}, // Bits
	{760.f,  0.44f, 0.72f, 0.58f, 0.38f, 0.70f, 0.86f, 1.10f, 2.60f, 0.92f, 0.56f, 0.62f}, // Fault
};

static const ClangSoundSpec clangSoundSpecs[CLANG_SOUNDS] = {
	{0.52f, 0.63f, 1.18f, 0.62f, 1.45f, 0.62f, 0.15f, 0.10f, 0.12f, 0.0f},
	{0.70f, 0.79f, 1.00f, 0.82f, 1.22f, 0.78f, 0.28f, 0.18f, 0.20f, 0.7f},
	{1.00f, 1.00f, 0.92f, 1.00f, 1.00f, 1.00f, 0.42f, 0.28f, 0.28f, 1.4f},
	{1.24f, 1.31f, 0.78f, 1.16f, 0.86f, 0.88f, 0.58f, 0.38f, 0.38f, 2.2f},
	{1.58f, 1.57f, 0.72f, 1.28f, 0.76f, 1.18f, 0.72f, 0.52f, 0.50f, 3.4f},
	{1.96f, 1.93f, 0.56f, 1.42f, 0.64f, 0.72f, 0.84f, 0.70f, 0.36f, 1.1f},
	{2.55f, 2.41f, 0.42f, 1.64f, 0.54f, 0.54f, 0.94f, 0.88f, 0.62f, 4.8f},
	{3.32f, 3.10f, 0.34f, 1.92f, 0.48f, 1.42f, 1.00f, 1.00f, 0.74f, 6.2f},
};

// Every model owns its tuning map. This keeps the Sounds knob from becoming
// the same eight transpositions repeated across all sixteen models.
static const float clangPhysicalPitch[CLANG_MODELS][CLANG_SOUNDS] = {
	{1.00f, 1.72f, 0.48f, 0.73f, 2.26f, 0.34f, 1.31f, 3.18f}, // Tick
	{0.41f, 0.67f, 0.29f, 0.53f, 1.13f, 0.82f, 1.72f, 2.47f}, // Dust
	{1.00f, 2.37f, 0.56f, 1.31f, 0.74f, 0.31f, 1.82f, 3.46f}, // Metal
	{1.00f, 0.52f, 1.48f, 0.79f, 2.12f, 3.08f, 0.37f, 1.71f}, // Wire
	{1.00f, 1.62f, 0.53f, 1.21f, 0.34f, 2.43f, 0.72f, 1.89f}, // Tube
	{0.72f, 1.21f, 0.43f, 1.67f, 0.29f, 2.18f, 0.91f, 2.87f}, // Scrap
	{1.00f, 0.61f, 2.08f, 0.38f, 1.43f, 2.71f, 0.82f, 3.62f}, // Fold
	{0.53f, 1.00f, 1.73f, 0.31f, 2.42f, 0.77f, 3.11f, 0.19f}, // Crush
};

static const float clangPhysicalRatio[CLANG_MODELS][CLANG_SOUNDS] = {
	{1.91f, 2.73f, 1.17f, 3.11f, 4.37f, 1.43f, 5.23f, 7.09f},
	{0.71f, 1.37f, 2.19f, 0.83f, 3.71f, 1.11f, 4.63f, 6.17f},
	{1.41f, 3.76f, 2.09f, 1.17f, 2.71f, 0.63f, 4.93f, 6.61f},
	{1.01f, 0.51f, 1.97f, 2.63f, 3.31f, 5.17f, 0.79f, 7.13f},
	{1.00f, 1.49f, 0.50f, 2.01f, 0.25f, 3.07f, 0.67f, 4.13f},
	{1.29f, 2.31f, 0.73f, 3.17f, 0.41f, 4.79f, 1.61f, 6.43f},
	{1.63f, 0.89f, 3.27f, 0.47f, 2.43f, 5.39f, 1.21f, 7.31f},
	{0.77f, 1.57f, 2.83f, 0.37f, 4.31f, 1.13f, 6.07f, 0.23f},
};

static const float clangFmPitch[CLANG_MODELS][CLANG_SOUNDS] = {
	{1.00f, 1.51f, 0.67f, 0.36f, 0.82f, 1.97f, 2.71f, 3.83f}, // Ping
	{0.50f, 0.67f, 1.00f, 0.43f, 0.80f, 0.56f, 0.64f, 0.62f}, // Ratio
	{1.00f, 0.71f, 1.91f, 0.43f, 0.58f, 1.29f, 0.83f, 0.31f}, // Bell
	{1.00f, 1.73f, 0.47f, 2.31f, 0.69f, 3.17f, 0.36f, 4.09f}, // Arc
	{0.63f, 1.00f, 1.57f, 0.41f, 2.13f, 0.79f, 2.83f, 0.29f}, // Phase
	{0.50f, 0.33f, 0.25f, 0.40f, 0.67f, 0.19f, 1.13f, 0.14f}, // Stack
	{1.00f, 1.83f, 0.52f, 2.77f, 0.31f, 3.91f, 0.73f, 5.17f}, // Bits
	{0.50f, 0.71f, 1.00f, 1.43f, 0.37f, 2.19f, 0.83f, 0.21f}, // Fault
};

static const float clangFmRatio[CLANG_MODELS][CLANG_SOUNDS] = {
	{1.01f, 1.53f, 2.07f, 0.49f, 3.13f, 4.01f, 5.17f, 7.03f},
	{1.00f, 1.50f, 2.00f, 2.333f, 1.25f, 1.80f, 1.571f, 1.625f},
	{2.71f, 3.76f, 5.19f, 6.83f, 1.41f, 4.13f, 2.37f, 7.29f},
	{1.79f, 2.61f, 0.73f, 3.89f, 1.23f, 5.11f, 0.43f, 7.17f},
	{1.00f, 1.99f, 3.01f, 0.51f, 4.03f, 1.49f, 5.07f, 0.25f},
	{1.50f, 2.00f, 2.51f, 1.25f, 3.01f, 0.75f, 4.49f, 0.375f},
	{1.07f, 2.13f, 3.91f, 5.03f, 0.61f, 6.17f, 1.49f, 7.31f},
	{1.00f, 1.07f, 1.13f, 2.97f, 0.47f, 4.73f, 6.11f, 0.19f},
};

// Per-slot loudness trims keep sound browsing consistent without flattening
// the actual timbral differences between models and sounds.
static const float clangOutputTrim[2][CLANG_MODELS][CLANG_SOUNDS] = {
	{
		{1.053f, 0.784f, 1.115f, 0.762f, 0.981f, 0.741f, 2.475f, 0.775f}, // Tick
		{0.840f, 0.924f, 2.588f, 2.734f, 0.800f, 2.272f, 1.077f, 1.160f}, // Dust
		{0.856f, 1.082f, 0.920f, 0.857f, 0.832f, 0.900f, 1.410f, 0.845f}, // Metal
		{0.906f, 0.953f, 0.878f, 1.039f, 0.876f, 1.049f, 0.665f, 0.925f}, // Wire
		{0.819f, 0.843f, 0.893f, 0.929f, 0.847f, 0.878f, 0.928f, 0.942f}, // Tube
		{0.775f, 1.110f, 0.821f, 1.179f, 1.062f, 0.721f, 0.899f, 0.650f}, // Scrap
		{0.929f, 0.665f, 0.827f, 0.841f, 0.779f, 0.982f, 0.818f, 0.802f}, // Fold
		{1.046f, 0.726f, 0.816f, 0.906f, 0.801f, 0.752f, 1.190f, 2.363f}, // Crush
	},
	{
		{0.893f, 0.710f, 0.777f, 0.839f, 0.906f, 0.817f, 0.987f, 0.691f}, // Ping
		{0.918f, 0.835f, 0.814f, 0.818f, 0.863f, 0.789f, 0.848f, 0.798f}, // Ratio
		{0.771f, 0.701f, 0.858f, 0.717f, 0.878f, 1.056f, 0.687f, 0.819f}, // Bell
		{0.793f, 0.931f, 0.912f, 0.832f, 0.843f, 2.026f, 0.775f, 0.765f}, // Arc
		{0.868f, 0.948f, 0.848f, 0.780f, 0.886f, 0.854f, 0.809f, 0.894f}, // Phase
		{0.966f, 1.003f, 1.071f, 0.997f, 1.013f, 1.150f, 0.633f, 0.926f}, // Stack
		{0.983f, 0.852f, 0.957f, 1.080f, 0.651f, 1.064f, 1.253f, 0.725f}, // Bits
		{0.961f, 0.779f, 0.896f, 0.901f, 0.898f, 0.965f, 0.840f, 0.880f}, // Fault
	},
};

// Separate transient and body trims prevent short ticks from disappearing
// while keeping aggressive sustained sounds from dominating the mix.
static const float clangTransientTrim[2][CLANG_MODELS][CLANG_SOUNDS] = {
	{
		{1.28f, 1.20f, 1.24f, 1.16f, 1.18f, 1.14f, 1.10f, 1.08f}, // Tick
		{1.02f, 1.00f, 1.08f, 1.10f, 1.00f, 1.08f, 1.06f, 1.04f}, // Dust
		{1.16f, 1.12f, 1.10f, 1.08f, 1.06f, 1.04f, 1.08f, 0.94f}, // Metal
		{1.22f, 1.16f, 1.12f, 1.08f, 1.06f, 1.12f, 1.00f, 0.98f}, // Wire
		{1.02f, 1.00f, 1.04f, 1.00f, 1.02f, 1.00f, 1.04f, 1.02f}, // Tube
		{1.12f, 1.16f, 1.04f, 1.12f, 1.10f, 1.02f, 1.12f, 1.08f}, // Scrap
		{1.12f, 0.98f, 1.04f, 1.06f, 0.96f, 1.02f, 1.08f, 0.96f}, // Fold
		{1.16f, 1.12f, 0.96f, 1.06f, 1.04f, 1.02f, 1.06f, 1.04f}, // Crush
	},
	{
		{1.18f, 1.12f, 1.08f, 1.04f, 1.06f, 1.12f, 1.08f, 1.04f}, // Ping
		{1.04f, 1.02f, 1.00f, 0.98f, 1.00f, 0.98f, 0.96f, 0.94f}, // Ratio
		{1.04f, 1.00f, 0.98f, 1.00f, 0.98f, 0.96f, 0.94f, 0.92f}, // Bell
		{1.08f, 1.04f, 1.02f, 1.04f, 1.02f, 1.04f, 1.00f, 0.98f}, // Arc
		{1.04f, 1.02f, 1.00f, 1.02f, 1.00f, 0.98f, 0.96f, 0.94f}, // Phase
		{1.08f, 1.04f, 1.00f, 1.02f, 1.00f, 0.98f, 0.96f, 0.94f}, // Stack
		{1.10f, 1.06f, 1.02f, 1.04f, 1.00f, 0.98f, 0.96f, 0.94f}, // Bits
		{1.06f, 1.02f, 1.00f, 1.02f, 0.98f, 0.96f, 0.94f, 0.92f}, // Fault
	},
};

static const float clangBodyTrim[2][CLANG_MODELS][CLANG_SOUNDS] = {
	{
		{1.00f, 0.98f, 0.98f, 0.96f, 0.96f, 0.94f, 0.92f, 0.90f}, // Tick
		{1.00f, 0.98f, 1.08f, 1.06f, 0.94f, 1.06f, 1.04f, 1.02f}, // Dust
		{0.90f, 0.88f, 0.86f, 0.84f, 0.84f, 0.82f, 1.00f, 0.70f}, // Metal
		{0.94f, 0.92f, 0.90f, 0.88f, 0.86f, 0.84f, 0.74f, 0.72f}, // Wire
		{0.92f, 0.90f, 0.88f, 0.86f, 0.88f, 0.86f, 0.84f, 0.82f}, // Tube
		{0.94f, 1.04f, 0.90f, 1.02f, 1.00f, 0.84f, 0.82f, 0.80f}, // Scrap
		{0.92f, 0.82f, 0.88f, 0.86f, 0.76f, 0.82f, 0.80f, 0.72f}, // Fold
		{0.94f, 0.92f, 0.80f, 0.88f, 0.86f, 0.84f, 1.04f, 1.02f}, // Crush
	},
	{
		{0.94f, 0.92f, 0.90f, 0.88f, 0.86f, 0.82f, 0.84f, 0.80f}, // Ping
		{0.96f, 0.94f, 0.92f, 0.90f, 0.88f, 0.86f, 0.84f, 0.82f}, // Ratio
		{0.88f, 0.82f, 0.86f, 0.84f, 0.82f, 0.80f, 0.78f, 0.76f}, // Bell
		{0.84f, 0.82f, 0.80f, 0.78f, 0.80f, 0.78f, 0.76f, 0.74f}, // Arc
		{0.90f, 0.88f, 0.86f, 0.84f, 0.82f, 0.80f, 0.78f, 0.76f}, // Phase
		{0.96f, 0.94f, 0.92f, 0.90f, 0.88f, 0.86f, 0.84f, 0.82f}, // Stack
		{0.94f, 0.92f, 0.90f, 0.88f, 0.86f, 0.84f, 0.82f, 0.80f}, // Bits
		{0.90f, 0.88f, 0.86f, 0.84f, 0.82f, 0.80f, 0.78f, 0.76f}, // Fault
	},
};

static const float clangModelMotionDepth[2][CLANG_MODELS] = {
	{1.20f, 0.82f, 0.96f, 1.22f, 0.78f, 1.08f, 1.28f, 1.38f}, // Physical
	{1.12f, 1.28f, 0.98f, 1.42f, 1.30f, 1.16f, 1.38f, 1.34f}, // FM
};

static const float clangSoundMotionDepth[CLANG_SOUNDS] = {
	0.82f, 1.00f, 1.16f, 1.34f, 0.78f, 1.08f, 1.30f, 1.48f
};

struct ClangVoice {
	float age = 0.f;
	float attackEnv = 0.f;
	float attackTime = 0.00022f;
	float bodyEnv = 0.f;
	float noiseEnv = 0.f;
	float clickEnv = 0.f;
	float subEnv = 0.f;
	float phase1 = 0.f;
	float phase2 = 0.f;
	float phaseSub = 0.f;
	float freq = 440.f;
	float subFreq = 110.f;
	float ratio = 1.7f;
	float noiseTone = 0.f;
	float hpX = 0.f;
	float hpY = 0.f;
	float bodyLevel = 1.f;
	float noiseLevel = 1.f;
	float clickLevel = 1.f;
	float bodyDecayScale = 1.f;
	float noiseDecayScale = 1.f;
	float bright = 0.5f;
	float foldAmt = 0.f;
	float fmIndex = 1.f;
	float crushAmt = 0.f;
	float pitchEnv = 0.f;
	float pitchBend = 0.f;
	float ghostDelay = 0.f;
	float ghostEnv = 0.f;
	float ghostAmt = 0.f;
	float tailCharacter = 1.f;
	float outputTrim = 1.f;
	float transientTrim = 1.f;
	float bodyTrim = 1.f;
	float motionDepth = 1.f;
	int engine = 0;
	int model = 0;
	int sound = 0;
};

struct Clang : Module {
	enum ParamId {
		MODEL_PARAM,
		ENGINE_PARAM,
		SOUND_PARAM,
		DECAY_PARAM,
		TUNE_PARAM,
		BODY_PARAM,
		MOTION_PARAM,
		NOISE_PARAM,
		VARIATION_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIG_INPUT,
		MODEL_INPUT,
		SOUND_INPUT,
		MOTION_INPUT,
		TUNE_INPUT,
		DECAY_INPUT,
		NOISE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		TRIGGER_LIGHT,
		LIGHTS_LEN
	};

	ClangVoice voices[CLANG_VOICES];
	dsp::SchmittTrigger trigIn;
	uint32_t rng = 0x937a4c29u;
	float dcX = 0.f;
	float dcY = 0.f;
	float triggerLightEnv = 0.f;
	int lastEngine = 0;

	Clang() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MODEL_PARAM, 0.f, 7.f, 0.f, "Model");
		configSwitch(ENGINE_PARAM, 0.f, 1.f, 0.f, "Engine", {"Physical", "FM"});
		configParam(SOUND_PARAM, 0.f, 7.f, 0.f, "Sounds");
		configParam(DECAY_PARAM, 0.f, 1.f, 0.8494f, "Decay");
		configParam(TUNE_PARAM, -2.f, 2.f, 0.f, "Tune", " oct");
		configParam(BODY_PARAM, 0.f, 1.f, 0.79759f, "Body");
		configParam(MOTION_PARAM, 0.f, 1.f, 0.f, "Motion");
		configParam(NOISE_PARAM, 0.f, 1.f, 0.f, "Noise");
		configParam(VARIATION_PARAM, 0.f, 1.f, 0.15181f, "Tune variation");
		getParamQuantity(MODEL_PARAM)->snapEnabled = true;
		getParamQuantity(SOUND_PARAM)->snapEnabled = true;
		configInput(TRIG_INPUT, "Trigger");
		configInput(MODEL_INPUT, "Model CV");
		configInput(SOUND_INPUT, "Sounds CV");
		configInput(MOTION_INPUT, "Motion CV");
		configInput(TUNE_INPUT, "Pitch (1V/oct)");
		configInput(DECAY_INPUT, "Decay CV");
		configInput(NOISE_INPUT, "Noise CV");
		configOutput(OUT_OUTPUT, "Audio Out");
	}

	float nextWhite() {
		rng ^= rng << 13;
		rng ^= rng >> 17;
		rng ^= rng << 5;
		return (float)(int32_t)rng / 2147483648.f;
	}

	float stablePhase01(uint32_t key) const {
		key ^= key >> 16;
		key *= 0x7feb352du;
		key ^= key >> 15;
		key *= 0x846ca68bu;
		key ^= key >> 16;
		return (float)(key & 0x00ffffffu) / 16777216.f;
	}

	float dcBlock(float x) {
		float y = x - dcX + 0.995f * dcY;
		dcX = x;
		dcY = y;
		return y;
	}

	float fold(float x, float amount) {
		float drive = 1.f + amount * 9.f;
		x *= drive;
		if (x > 1.f)
			x = 2.f - x;
		if (x < -1.f)
			x = -2.f - x;
		return clamp(x, -1.6f, 1.6f);
	}

	int chooseVoice() const {
		int best = 0;
		float bestEnergy = 1e30f;
		for (int v = 0; v < CLANG_VOICES; v++) {
			// Include every tail in voice stealing. Otherwise a quiet body can
			// be replaced while its sub/ghost tail is still sustaining.
			float e = voices[v].bodyEnv + voices[v].noiseEnv + voices[v].clickEnv
				+ voices[v].subEnv * 0.85f + voices[v].ghostEnv * 0.65f;
			if (e < bestEnergy) {
				best = v;
				bestEnergy = e;
			}
		}
		return best;
	}

	int readSnapParam(ParamId param, InputId input, int maxIndex) {
		int value = clamp((int)std::round(params[param].getValue()), 0, maxIndex);
		if (inputs[input].isConnected()) {
			float cv = clamp(inputs[input].getVoltage(), 0.f, 10.f);
			value = clamp((int)std::floor(cv / 10.f * (maxIndex + 1)), 0, maxIndex);
		}
		return value;
	}

	void triggerVoice(int engine, int model, int sound) {
		ClangVoice& voice = voices[chooseVoice()];
		const ClangModelSpec& modelSpec = engine == 0 ? clangPhysicalSpecs[model] : clangFmSpecs[model];
		const ClangSoundSpec& soundSpec = clangSoundSpecs[sound];
		voice.engine = engine;
		voice.model = model;
		voice.sound = sound;
		voice.outputTrim = clangOutputTrim[engine][model][sound];
		voice.transientTrim = clangTransientTrim[engine][model][sound];
		voice.bodyTrim = clangBodyTrim[engine][model][sound];
		voice.motionDepth = clangModelMotionDepth[engine][model] * clangSoundMotionDepth[sound];
		voice.age = 0.f;
		voice.attackEnv = 0.f;
		voice.attackTime = 0.00022f;
		if (engine == 0 && model == 2 && sound == 5)
			voice.attackTime = 0.00060f; // Metal -> Tank: soften its phase-start impact.
		if (engine == 0 && model == 0 && sound == 5)
			voice.attackTime = 0.00038f; // Tick -> Nail: soften the hard start.
		if (engine == 0 && model == 2 && sound == 2)
			voice.attackTime = 0.00036f; // Metal -> Bolt: soften the hard start.
		if (engine == 0 && model == 2 && sound == 4)
			voice.attackTime = 0.00034f; // Metal -> Rod: soften the hard start.
		if (engine == 0 && model == 4 && sound == 0)
			voice.attackTime = 0.00070f; // Tube -> Pipe: soften the hard start.
		if (engine == 0 && model == 4 && sound == 1)
			voice.attackTime = 0.00055f; // Tube -> Can: soften the hard start.
		if (engine == 0 && model == 4 && sound == 2)
			voice.attackTime = 0.00050f; // Tube -> Block: soften the hard start.
		if (engine == 0 && model == 4 && sound == 4)
			voice.attackTime = 0.00065f; // Tube -> Drum: soften the hard start.
		if (engine == 0 && model == 4 && sound == 7)
			voice.attackTime = 0.00055f; // Tube -> Shell: soften the hard start.
		if (engine == 0 && model == 5 && sound == 0)
			voice.attackTime = 0.00045f; // Scrap -> Rust: soften the hard start.
		if (engine == 0 && model == 5 && sound == 2)
			voice.attackTime = 0.00045f; // Scrap -> Junk: soften the hard start.
		if (engine == 0 && model == 5 && sound == 4)
			voice.attackTime = 0.00045f; // Scrap -> Debris: soften the hard start.
		if (engine == 0 && model == 5 && sound == 5)
			voice.attackTime = 0.00045f; // Scrap -> Rotor: soften the hard start.
		if (engine == 0 && model == 5 && sound == 6)
			voice.attackTime = 0.00045f; // Scrap -> Clack: soften the hard start.
		if (engine == 0 && model == 5 && sound == 7)
			voice.attackTime = 0.00055f; // Scrap -> Crash: soften the hard start.
		if (engine == 0 && model == 7 && sound == 7)
			voice.attackTime = 0.00055f; // Crush -> Break: soften the hard start.
		if (engine == 1 && model == 0 && sound == 0)
			voice.attackTime = 0.00045f; // FM -> Dot: soften the hard start.
		if (engine == 1 && model == 0 && sound == 3)
			voice.attackTime = 0.00040f; // FM -> Knock: soften the hard start.
		if (engine == 1 && model == 1 && sound == 0)
			voice.attackTime = 0.00045f; // FM -> Ratio 1:1: soften the hard start.
		// Keep each preset's oscillator relationship stable so its onset does not
		// alternate between a weak and strong click. Advance the RNG exactly as
		// before so noise and Tune Variation retain their existing behaviour.
		nextWhite();
		nextWhite();
		nextWhite();
		uint32_t phaseKey = 0x6d2b79f5u ^ (uint32_t)(engine * 131 + model * 17 + sound);
		voice.phase1 = 2.f * M_PI * stablePhase01(phaseKey ^ 0x9e3779b9u);
		voice.phase2 = 2.f * M_PI * stablePhase01(phaseKey ^ 0x85ebca6bu);
		voice.phaseSub = 2.f * M_PI * stablePhase01(phaseKey ^ 0xc2b2ae35u);
		voice.noiseTone = 0.f;
		voice.hpX = 0.f;
		voice.hpY = 0.f;

		float tuneOctaves = params[TUNE_PARAM].getValue() + inputs[TUNE_INPUT].getVoltage();
		float tune = std::pow(2.f, tuneOctaves);
		float variation = params[VARIATION_PARAM].getValue();
		// Tune Variation widens a centered low-to-high pitch range per hit.
		// At zero every trigger keeps the exact mapped pitch; higher settings
		// spread the complete voice, including its sub component.
		float jitterDepth = engine == 0 ? (0.10f + 0.06f * (float)model) : (0.16f + 0.10f * (float)model);
		float jitter = std::pow(2.f, nextWhite() * variation * jitterDepth);
		float pitchMap = engine == 0 ? clangPhysicalPitch[model][sound] : clangFmPitch[model][sound];
		voice.freq = clamp(modelSpec.baseFreq * pitchMap * tune * jitter, 22.f, 18000.f);
		float subBase = modelSpec.baseFreq * (engine == 0 ? 0.18f : 0.11f) * (0.72f + 0.32f * (float)(sound % 3));
		voice.subFreq = clamp(subBase * tune * jitter * std::pow(2.f, -0.55f + 0.14f * (float)sound), 35.f, 420.f);
		voice.ratio = engine == 0 ? clangPhysicalRatio[model][sound] : clangFmRatio[model][sound];

		voice.clickEnv = 1.0f;
		voice.subEnv = 0.90f;
		voice.noiseEnv = 0.72f + 0.08f * (float)((sound * 5 + model * 3) & 3);
		voice.bodyEnv = 0.86f + 0.10f * (float)((sound * 3 + model) & 3);
		voice.bodyLevel = modelSpec.body * soundSpec.body;
		voice.noiseLevel = modelSpec.noise * soundSpec.noise;
		voice.clickLevel = modelSpec.click * soundSpec.click;
		voice.bodyDecayScale = modelSpec.bodyDecay * soundSpec.decay;
		voice.noiseDecayScale = modelSpec.noiseDecay * (0.70f + 0.60f * soundSpec.noise);
		voice.bright = clamp(modelSpec.bright * 0.62f + soundSpec.bright * 0.52f, 0.f, 1.35f);
		voice.foldAmt = clamp(modelSpec.fold * 0.55f + soundSpec.fold * 0.65f, 0.f, 1.6f);
		voice.fmIndex = modelSpec.fm * (0.72f + 0.55f * soundSpec.ratio);
		voice.crushAmt = clamp(modelSpec.crush + soundSpec.fold * 0.28f, 0.f, 1.4f);
		voice.pitchEnv = 1.f;
		voice.pitchBend = (modelSpec.bend * 0.75f + soundSpec.bend * 0.65f) * (engine == 0 ? 0.55f : 0.95f) * variation;
		voice.ghostDelay = (soundSpec.delayMs + modelSpec.ghost * 3.5f * (0.25f + variation)) / 1000.f;
		voice.ghostEnv = 0.f;
		voice.ghostAmt = clamp(modelSpec.ghost * (0.25f + soundSpec.fold * 0.75f), 0.f, 0.85f);
		// Keep the default response percussive. High Decay can still open the
		// full resonant tail through the late-decay curve below.
		voice.tailCharacter = engine == 1 ? 0.42f : 1.f;
		// Arc -> Zap is a sharp electrical hit, not a sustained oscillator.
		// Keep its bright zap intact while shortening the resonant tail.
		if (engine == 1 && model == 3 && (sound == 0 || sound == 3 || sound == 4 || sound == 5 || sound == 6 || sound == 7)) {
			voice.tailCharacter = 0.34f;
			voice.ghostAmt *= 0.35f;
		}
		if (engine == 1 && model == 0 && sound == 5) {
			// Strike is a short metallic impact, with no resonant drone tail.
			voice.tailCharacter = 0.10f;
			voice.pitchBend += 0.86f;
			voice.ghostAmt *= 0.04f;
			voice.outputTrim *= 1.05f;
		}
		if (engine == 1 && model == 0 && sound == 7) {
			// Shard is a short metallic splinter with almost no resonance.
			voice.tailCharacter = 0.09f;
			voice.pitchBend += 0.72f;
			voice.ghostAmt *= 0.04f;
			voice.outputTrim *= 1.02f;
		}
		if (engine == 1 && model == 2 && sound == 3) {
			// Bell -> Hard stays punchy at the new default Decay instead of
			// turning into a long resonant drone.
			voice.tailCharacter = 0.18f;
			voice.ghostAmt *= 0.12f;
		}
		if (engine == 1 && model == 2 && sound == 6) {
			// Bell -> Wrong keeps its unstable colour without a long ringing tail.
			voice.tailCharacter = 0.18f;
			voice.ghostAmt *= 0.12f;
		}
		if (engine == 1 && model == 2 && sound == 7) {
			// Bell -> Steel keeps its metallic ring without dominating the mix.
			voice.tailCharacter = 0.18f;
			voice.ghostAmt *= 0.12f;
		}
		if (engine == 1 && model == 3 && sound == 0) {
			// Arc -> Zap is an immediate electrical hit, not a sustained drone.
			voice.tailCharacter = 0.08f;
			voice.pitchBend += 0.38f;
			voice.ghostAmt *= 0.04f;
			voice.outputTrim *= 0.96f;
		}
		if (engine == 1 && model == 3 && sound == 3) {
			// Arc -> Flash is a bright short hit, not a long electrical drone.
			voice.tailCharacter = 0.10f;
			voice.ghostAmt *= 0.06f;
			voice.outputTrim *= 0.96f;
		}
		if (engine == 1 && model == 3 && sound == 4) {
			// Arc -> Tension keeps the charged character without a long drone tail.
			voice.tailCharacter = 0.12f;
			voice.ghostAmt *= 0.08f;
			voice.outputTrim *= 0.96f;
		}
		if (engine == 1 && model == 3 && sound == 7) {
			// Spasm is a short electrical convulsion, not a sustained drone.
			voice.tailCharacter = 0.09f;
			voice.pitchBend += 0.62f;
			voice.ghostAmt *= 0.05f;
			voice.outputTrim *= 0.90f;
		}
		if (engine == 1 && model == 4 && sound == 3) {
			// Phase -> Skew keeps its animated phase colour without a long tail.
			voice.tailCharacter = 0.14f;
			voice.ghostAmt *= 0.08f;
		}
		if (engine == 1 && model == 4 && sound == 6) {
			// Phase -> Tear keeps its rough movement without a long resonant tail.
			voice.tailCharacter = 0.12f;
			voice.ghostAmt *= 0.07f;
		}
		if (engine == 1 && model == 4 && sound == 7) {
			// Phase -> Flip keeps its phase inversion without a long tail.
			voice.tailCharacter = 0.12f;
			voice.ghostAmt *= 0.07f;
		}
		if (engine == 1 && model == 6 && sound == 7) {
			// Bits -> Rip keeps its digital tear without a long tail.
			voice.tailCharacter = 0.12f;
			voice.ghostAmt *= 0.06f;
		}
		if (engine == 1 && model == 7 && sound == 7) {
			// Fault -> Broken keeps its damaged digital edge without a long tail.
			voice.tailCharacter = 0.12f;
			voice.ghostAmt *= 0.06f;
		}
		if (engine == 1 && model == 3 && sound == 6) {
			// Coil is a compact electrical impact with a controlled resonance.
			voice.tailCharacter = 0.10f;
			voice.pitchBend += 0.55f;
			voice.ghostAmt *= 0.06f;
			voice.outputTrim *= 0.90f;
		}
		if (engine == 0 && model == 7 && sound == 2) {
			// Shatter is a short digital fracture with a falling impact pitch.
			voice.tailCharacter = 0.28f;
			voice.pitchBend += 0.30f;
		}
		if (engine == 0 && model == 2 && sound == 7) {
			// Metal -> Shard keeps its metallic hit without a long ring.
			voice.tailCharacter = 0.18f;
			voice.ghostAmt *= 0.10f;
		}
		if (engine == 0 && model == 3 && sound == 7) {
			// Wire -> Snap is a short mechanical hit, not a long string ring.
			voice.tailCharacter = 0.16f;
			voice.ghostAmt *= 0.08f;
		}
		if (engine == 0 && model == 4 && sound == 3) {
			// Tube -> Valve is only slightly shortened; keep its warm body.
			voice.tailCharacter = 0.68f;
			voice.ghostAmt *= 0.16f;
		}
		if (engine == 0 && model == 4 && sound == 7) {
			// Tube -> Shell gets a small tail reduction while keeping its body.
			voice.tailCharacter = 0.64f;
			voice.ghostAmt *= 0.14f;
		}
		if (engine == 0 && model == 6 && sound == 7) {
			// Fold -> Edge stays sharp without a long folded resonance.
			voice.tailCharacter = 0.18f;
			voice.ghostAmt *= 0.08f;
		}
	}

	void process(const ProcessArgs& args) override {
		int model = readSnapParam(MODEL_PARAM, MODEL_INPUT, CLANG_MODELS - 1);
		int sound = readSnapParam(SOUND_PARAM, SOUND_INPUT, CLANG_SOUNDS - 1);
		int engine = clamp((int)std::round(params[ENGINE_PARAM].getValue()), 0, 1);
		if (engine == 1)
			model = clangFmModelOrder[model];
		if (engine != lastEngine) {
			// Do not let voices from the previous engine survive a switch.
			for (int v = 0; v < CLANG_VOICES; v++) {
				voices[v].bodyEnv = 0.f;
				voices[v].noiseEnv = 0.f;
				voices[v].clickEnv = 0.f;
				voices[v].subEnv = 0.f;
				voices[v].pitchEnv = 0.f;
				voices[v].ghostDelay = 0.f;
				voices[v].ghostEnv = 0.f;
				voices[v].age = 5.f;
			}
			lastEngine = engine;
		}

		if (trigIn.process(inputs[TRIG_INPUT].getVoltage(), 0.1f, 2.f)) {
			triggerVoice(engine, model, sound);
			triggerLightEnv = 1.f;
		}
		triggerLightEnv *= std::exp(-args.sampleTime / 0.080f);
		lights[TRIGGER_LIGHT].setBrightness(triggerLightEnv);

		float decay = clamp(params[DECAY_PARAM].getValue() + inputs[DECAY_INPUT].getVoltage() / 10.f, 0.f, 1.f);
		// Preserve the tight low end, then open up the tail progressively near
		// the top of the control instead of making the whole range feel slow.
		float lateDecay = std::max(0.f, decay - 0.50f);
		float decayCurve = decay + 5.0f * lateDecay * lateDecay;
		float bodyControl = params[BODY_PARAM].getValue();
		float noiseAmount = clamp(params[NOISE_PARAM].getValue() + inputs[NOISE_INPUT].getVoltage() / 10.f, 0.f, 1.f);
		float motion = params[MOTION_PARAM].getValue();
		if (inputs[MOTION_INPUT].isConnected())
			motion = clamp(motion + inputs[MOTION_INPUT].getVoltage() / 10.f, 0.f, 1.f);
		// Body balances resonant mass against the transient; Motion animates
		// pitch, phase and modulation instead of acting as another distortion.
		// Keep the existing excitation colour stable; Motion gets its character
		// from phase and modulation movement, not from extra wavefolding.
		float edge = clamp(0.22f + bodyControl * 0.28f, 0.f, 1.f);
		float damp = bodyControl;

		float out = 0.f;
		float activeEnergy = 0.f;
		float pitchEnvDecay = 0.f;
		float ghostEnvDecay = 0.f;
		bool pitchEnvDecayReady = false;
		bool ghostEnvDecayReady = false;
		for (int v = 0; v < CLANG_VOICES; v++) {
			ClangVoice& voice = voices[v];
			// Silent voice slots must not run the full oscillator and noise bank.
			// Keep delayed ghost layers alive, but otherwise pay DSP only for
			// voices that are actually contributing to the output.
			float envelopeEnergy = voice.bodyEnv + voice.noiseEnv + voice.clickEnv
				+ voice.subEnv + voice.ghostEnv;
			if (envelopeEnergy < 1e-5f) {
				if (voice.ghostDelay > 0.f && voice.ghostAmt > 1e-5f) {
					voice.age += args.sampleTime;
					voice.ghostDelay -= args.sampleTime;
					if (voice.ghostDelay > 0.f)
						continue;
					voice.ghostDelay = 0.f;
					voice.ghostEnv = voice.ghostAmt;
				}
				else {
					voice.attackEnv = 0.f;
					continue;
				}
			}
			voice.age += args.sampleTime;
			// Smooth the first samples of every new voice to prevent phase-reset clicks.
			voice.attackEnv = std::min(1.f, voice.attackEnv + args.sampleTime / voice.attackTime);
			if (voice.age > 4.f) {
				// Safety release: no voice may survive indefinitely if an envelope
				// ever receives an invalid or unexpectedly long state.
				voice.bodyEnv = 0.f;
				voice.noiseEnv = 0.f;
				voice.clickEnv = 0.f;
				voice.attackEnv = 0.f;
				voice.subEnv = 0.f;
				voice.pitchEnv = 0.f;
				voice.ghostDelay = 0.f;
				voice.ghostEnv = 0.f;
				voice.age = 5.f;
			}
			int engineV = voice.engine;
			int modelV = voice.model;
			int soundV = voice.sound;
			float motionAmount = clamp(motion * voice.motionDepth, 0.f, 1.55f);
			float soundPos = (float)soundV / 7.f;

			static const float bodyContour[CLANG_SOUNDS] = {0.42f, 0.76f, 0.28f, 1.18f, 0.54f, 0.17f, 0.88f, 1.46f};
			static const float noiseContour[CLANG_SOUNDS] = {0.24f, 0.58f, 0.37f, 1.34f, 0.82f, 0.29f, 0.64f, 1.62f};
			static const float subContour[CLANG_SOUNDS] = {0.36f, 1.28f, 0.54f, 0.82f, 0.31f, 0.22f, 0.67f, 1.08f};
			bool shortPingHit = voice.engine == 1 && voice.model == 0 && (voice.sound == 5 || voice.sound == 7);
			bool compactBellHard = voice.engine == 1 && voice.model == 2 && voice.sound == 3;
			bool compactBellWrong = voice.engine == 1 && voice.model == 2 && voice.sound == 6;
			bool compactBellSteel = voice.engine == 1 && voice.model == 2 && voice.sound == 7;
			bool shortArcZap = voice.engine == 1 && voice.model == 3 && voice.sound == 0;
			bool shortArcFlash = voice.engine == 1 && voice.model == 3 && voice.sound == 3;
			bool shortArcTension = voice.engine == 1 && voice.model == 3 && voice.sound == 4;
			bool shortArcCoil = voice.engine == 1 && voice.model == 3 && voice.sound == 6;
			bool shortArcSpasm = voice.engine == 1 && voice.model == 3 && voice.sound == 7;
			bool compactPhaseSkew = voice.engine == 1 && voice.model == 4 && voice.sound == 3;
			bool compactPhaseTear = voice.engine == 1 && voice.model == 4 && voice.sound == 6;
			bool compactPhaseFlip = voice.engine == 1 && voice.model == 4 && voice.sound == 7;
			bool compactBitsRip = voice.engine == 1 && voice.model == 6 && voice.sound == 7;
			bool compactFaultBroken = voice.engine == 1 && voice.model == 7 && voice.sound == 7;
			bool compactMetalShard = voice.engine == 0 && voice.model == 2 && voice.sound == 7;
			bool compactWireSnap = voice.engine == 0 && voice.model == 3 && voice.sound == 7;
			bool gentleTubeValve = voice.engine == 0 && voice.model == 4 && voice.sound == 3;
			bool gentleTubeShell = voice.engine == 0 && voice.model == 4 && voice.sound == 7;
			bool compactFoldEdge = voice.engine == 0 && voice.model == 6 && voice.sound == 7;
			float tailCurve = shortPingHit
				? 0.08f + 0.18f * decay
				: (shortArcZap
					? 0.04f + 0.07f * decay
					: (shortArcFlash
						? 0.04f + 0.11f * decay
						: (shortArcTension
							? 0.04f + 0.11f * decay
								: (shortArcCoil
									? 0.04f + 0.11f * decay
									: (shortArcSpasm
										? 0.04f + 0.11f * decay
										: ((compactPhaseSkew || compactPhaseTear || compactPhaseFlip)
							? 0.12f + 0.42f * decay
							: ((compactBitsRip || compactFaultBroken)
								? 0.12f + 0.40f * decay
								: (compactFoldEdge
									? 0.16f + 0.46f * decay
									: ((gentleTubeValve || gentleTubeShell)
									? 0.58f + 0.35f * decay
									: ((compactMetalShard || compactWireSnap)
									? 0.16f + 0.48f * decay
									: ((compactBellHard || compactBellWrong || compactBellSteel)
									? 0.16f + 0.58f * decay
									: (voice.tailCharacter < 1.f
										? voice.tailCharacter + (3.0f - voice.tailCharacter) * decay * decay
										: 1.f))))))))))));
			float bodyMs = clamp((5.f + 138.f * decayCurve) * bodyContour[soundV] * voice.bodyDecayScale * tailCurve, 0.20f, 2200.f);
			float noiseMs = clamp((1.5f + 74.f * decayCurve) * noiseContour[soundV] * voice.noiseDecayScale * tailCurve, 0.20f, 1600.f);
			float clickMs = (0.18f + 5.5f * (1.f - edge)) * (0.48f + 0.13f * (float)((soundV * 3 + modelV) & 3));

			if (modelV == 0) {
				bodyMs *= 0.30f;
				noiseMs *= 0.32f;
				clickMs *= 0.45f;
			}
			else if (modelV == 1) {
				bodyMs *= 0.24f;
				noiseMs *= 0.90f;
			}
			else if (modelV == 4) {
				bodyMs *= 1.35f;
				noiseMs *= 0.50f;
			}
			else if (modelV >= 5) {
				bodyMs *= 0.62f;
				noiseMs *= 0.82f;
			}

			float damping = 0.30f + 0.70f * bodyControl;
			bodyMs *= damping;
			noiseMs *= 0.45f + 0.85f * damping;

			voice.bodyEnv *= std::exp(-args.sampleTime / std::max(0.0002f, bodyMs / 1000.f));
			voice.noiseEnv *= std::exp(-args.sampleTime / std::max(0.0002f, noiseMs / 1000.f));
			voice.clickEnv *= std::exp(-args.sampleTime / std::max(0.00005f, clickMs / 1000.f));
			float subMs = clamp((18.f + 170.f * decayCurve) * subContour[soundV] * (0.55f + 0.80f * damp) * tailCurve, 0.20f, 2400.f);
			voice.subEnv *= std::exp(-args.sampleTime / std::max(0.0002f, subMs / 1000.f));
			if (!pitchEnvDecayReady) {
				pitchEnvDecay = std::exp(-args.sampleTime / std::max(0.0002f, (1.5f + 18.f * decayCurve) / 1000.f));
				pitchEnvDecayReady = true;
			}
			voice.pitchEnv *= pitchEnvDecay;
			if (voice.ghostDelay > 0.f) {
				voice.ghostDelay -= args.sampleTime;
				if (voice.ghostDelay <= 0.f)
					voice.ghostEnv = voice.ghostAmt;
			}
			if (voice.ghostEnv > 0.f) {
				if (!ghostEnvDecayReady) {
					ghostEnvDecay = std::exp(-args.sampleTime / std::max(0.0002f, (1.2f + 8.f * decayCurve) / 1000.f));
					ghostEnvDecayReady = true;
				}
				voice.ghostEnv *= ghostEnvDecay;
			}

			float bend = std::pow(2.f, voice.pitchEnv * voice.pitchBend);
			float motionCycle = std::sin(voice.phaseSub * (0.85f + 5.25f * motionAmount) + voice.phase1 * 0.07f) * motionAmount;
			voice.phase1 += 2.f * M_PI * clamp(voice.freq * bend * (1.f + motionCycle * 0.11f), 15.f, 20000.f) * args.sampleTime;
			voice.phase2 += 2.f * M_PI * clamp(voice.freq * voice.ratio * (1.f + voice.pitchEnv * voice.pitchBend * 0.18f + motionCycle * 0.24f), 15.f, 20000.f) * args.sampleTime;
			voice.phaseSub += 2.f * M_PI * clamp(voice.subFreq * (1.f + voice.pitchEnv * voice.pitchBend * 0.30f), 20.f, 900.f) * args.sampleTime;
			if (voice.phase1 > 2.f * M_PI)
				voice.phase1 -= 2.f * M_PI;
			if (voice.phase2 > 2.f * M_PI)
				voice.phase2 -= 2.f * M_PI;
			if (voice.phaseSub > 2.f * M_PI)
				voice.phaseSub -= 2.f * M_PI;

			float white = nextWhite();
			float follow = 0.10f + 0.62f * motion + 0.30f * voice.bright + 0.02f * (float)modelV;
			voice.noiseTone += clamp(follow, 0.05f, 0.98f) * (white - voice.noiseTone);

			float hpIn = voice.noiseTone;
			float hp = hpIn - voice.hpX + clamp(0.58f + 0.30f * damp - 0.16f * voice.bright, 0.30f, 0.96f) * voice.hpY;
			voice.hpX = hpIn;
			voice.hpY = hp;

			// Preserve the original random stream even when a colour is not needed.
			float burstWhite = nextWhite();
			float spitWhite = nextWhite();
			float ghostWhite = nextWhite();

			// Evaluate a sound building block only when the selected model uses it.
			// The value is cached, so repeated use within a voice remains identical.
			#define CLANG_LAZY(name, ...) \
				float name##Cache = 0.f; \
				bool name##Ready = false; \
				auto name = [&]() -> float { \
					if (!name##Ready) { \
						name##Cache = (__VA_ARGS__); \
						name##Ready = true; \
					} \
					return name##Cache; \
				}

			float s1 = std::sin(voice.phase1);
			CLANG_LAZY(phase2Base, std::sin(voice.phase2));
			float noisePhase = noiseAmount * (0.15f + 0.85f * ((soundV == 3 || soundV == 4 || soundV == 7) ? 1.f : 0.f));
			CLANG_LAZY(motionPhase, std::sin(voice.phaseSub * (1.1f + 3.9f * motionAmount) + voice.phase2 * 0.11f) * motionAmount * 0.95f);
			CLANG_LAZY(s2, std::sin(voice.phase2 + motionPhase() + voice.noiseTone * motionAmount * noisePhase * (1.5f + 3.f * soundPos)));
			CLANG_LAZY(low, std::sin(voice.phaseSub + s1 * voice.pitchEnv * (0.15f + 0.38f * motion)) * voice.subEnv * (0.25f + voice.bodyLevel * 0.85f));
			CLANG_LAZY(body, (s1 * 0.72f + s2() * 0.28f) * voice.bodyEnv * voice.bodyLevel * (0.42f + 1.12f * bodyControl));
			float localEdge = clamp(edge * 0.72f + bodyControl * 0.18f + voice.foldAmt * 0.38f, 0.f, 1.f);

			CLANG_LAZY(airNoise, voice.noiseTone * (0.45f + 0.45f * damp));
			CLANG_LAZY(sandNoise, hp * (white > 0.20f + 0.10f * (float)(soundV & 1) ? 1.f : -0.35f));
			CLANG_LAZY(tapeNoise, std::tanh((voice.noiseTone * 0.70f + std::sin(voice.phaseSub * 3.7f) * 0.30f) * (1.6f + 2.0f * edge)));
			CLANG_LAZY(grainNoise, std::sin(voice.phase2 * (3.0f + 0.37f * (float)soundV) + hp * (4.0f + 6.0f * edge)) * (0.45f + std::fabs(hp) * 0.55f));
			CLANG_LAZY(metalDust, fold(hp * 0.55f + std::sin(voice.phase1 * 2.03f + voice.phase2 * 0.29f) * 0.45f, clamp(localEdge + 0.10f, 0.f, 1.f)));
			CLANG_LAZY(digitalDust, std::round(hp * (3.f + 2.f * (float)(soundV % 4))) / (3.f + 2.f * (float)(soundV % 4)));
			CLANG_LAZY(steamNoise, (voice.noiseTone * 0.52f + std::sin(voice.phaseSub * (5.0f + 0.60f * (float)modelV)) * 0.48f) * voice.noiseEnv);
			CLANG_LAZY(burstNoise, fold((burstWhite * 0.62f + hp * 0.38f) * (1.2f + 1.8f * edge), clamp(voice.crushAmt + 0.18f, 0.f, 1.f)));
			CLANG_LAZY(crackleNoise, (std::fabs(hp) > 0.055f + 0.025f * (float)(soundV & 3) ? (hp > 0.f ? 1.f : -1.f) : 0.f) * (0.60f + 0.40f * std::fabs(hp)));
			CLANG_LAZY(whistleNoise, std::sin(voice.phase2 * (2.7f + 0.41f * (float)soundV) + hp * (3.0f + 4.0f * motionAmount)) * (0.35f + 0.65f * std::fabs(hp)));
			CLANG_LAZY(flutterNoise, std::sin(voice.phaseSub * (0.8f + 2.1f * (float)((soundV + modelV) & 3)) + voice.phase1 * 0.21f) * (0.30f + 0.70f * std::fabs(voice.noiseTone)));
			CLANG_LAZY(packetNoise, std::round((hp + std::sin(voice.phase1 * 0.37f) * 0.22f) * (4.f + (float)(soundV & 3))) / (4.f + (float)(soundV & 3)));
			CLANG_LAZY(rumbleNoise, std::sin(voice.phaseSub * (0.42f + 0.17f * (float)modelV) + hp * 2.2f) * 0.72f + hp * 0.28f);
			CLANG_LAZY(spitNoise, fold((spitWhite * 0.42f + hp * 0.58f) * (1.5f + 2.5f * motionAmount), 0.15f + localEdge * 0.55f));
			CLANG_LAZY(metalTickNoise, (std::sin(voice.phase1 * (2.01f + 0.13f * (float)soundV) + voice.phase2 * 0.07f) > 0.72f ? 1.f : -0.28f) * (0.35f + 0.65f * std::fabs(hp)));
			CLANG_LAZY(vaporNoise, std::tanh(voice.noiseTone * 1.8f + std::sin(voice.phaseSub * (3.2f + 0.31f * (float)soundV)) * 0.42f));

			float noise = 0.f;
			if (noiseAmount > 0.f) {
				int noiseColor = (engineV * 13 + modelV * 5 + soundV * 3) & 15;
				float shapedNoise = hp;
				switch (noiseColor) {
					case 0: shapedNoise = airNoise(); break;
					case 1: shapedNoise = sandNoise(); break;
					case 2: shapedNoise = tapeNoise(); break;
					case 3: shapedNoise = grainNoise(); break;
					case 4: shapedNoise = metalDust(); break;
					case 5: shapedNoise = digitalDust(); break;
					case 6: shapedNoise = steamNoise(); break;
					case 7: shapedNoise = burstNoise(); break;
					case 8: shapedNoise = crackleNoise(); break;
					case 9: shapedNoise = whistleNoise(); break;
					case 10: shapedNoise = flutterNoise(); break;
					case 11: shapedNoise = packetNoise(); break;
					case 12: shapedNoise = rumbleNoise(); break;
					case 13: shapedNoise = spitNoise(); break;
					case 14: shapedNoise = metalTickNoise(); break;
					default: shapedNoise = vaporNoise(); break;
				}
				// Compensate quiet model profiles so Noise at maximum is always a
				// meaningful control, without flattening the model-specific colour.
				float noiseGain = 0.42f + 0.88f * clamp(voice.noiseLevel, 0.f, 1.f);
				noise = shapedNoise * voice.noiseEnv * noiseGain * noiseAmount;
			}
			float rawClick = (white > (0.35f + 0.45f * damp - 0.18f * voice.bright) ? 1.f : -1.f) * voice.clickEnv * voice.clickLevel;
			float tonalClick = (s1 * 0.58f + std::sin(voice.phase1 * 2.317f + voice.phase2 * 0.13f) * 0.42f) * voice.clickEnv * voice.clickLevel;
			// Click is an intentional material property, never a universal attack.
			float clickPresence = 0.f;
			if (engineV == 0 && modelV == 0)
				clickPresence = 0.90f; // Tick
			else if (engineV == 0 && modelV == 3 && (soundV == 0 || soundV == 2 || soundV == 5 || soundV == 7))
				clickPresence = 0.52f; // Wire snaps
			else if (engineV == 0 && modelV == 5 && (soundV == 0 || soundV == 6))
				clickPresence = 0.34f; // Scrap clacks
			else if (engineV == 0 && modelV == 6 && (soundV == 2 || soundV == 6))
				clickPresence = 0.28f; // Fold bites
			else if (engineV == 0 && modelV == 7 && (soundV == 0 || soundV == 6))
				clickPresence = 0.24f; // Crush bits
			else if (engineV == 1 && modelV == 0)
				clickPresence = 0.48f; // Ping strikes
			else if (engineV == 1 && modelV == 3)
				clickPresence = 0.42f; // Arc flashes
			rawClick *= clickPresence;
			tonalClick *= clickPresence;
			CLANG_LAZY(click, tonalClick * (1.f - 0.42f * noiseAmount) + rawClick * noiseAmount * 0.72f);
			CLANG_LAZY(ghostTone, (s2() * 0.68f + std::sin(voice.phaseSub * 2.01f) * 0.32f) * voice.ghostEnv);
			CLANG_LAZY(ghost, ghostTone() * (1.f - 0.55f * noiseAmount) + ghostWhite * voice.ghostEnv * voice.noiseLevel * noiseAmount * 0.70f);
			CLANG_LAZY(chirp, std::sin(voice.phase1 + std::sin(voice.phase2 * (1.4f + 0.11f * modelV)) * (1.2f + 5.0f * localEdge)) * voice.bodyEnv);
			CLANG_LAZY(scrape, fold(noise * 0.92f + s2() * voice.noiseEnv * 0.24f * noiseAmount, clamp(localEdge + 0.25f, 0.f, 1.f)));
			CLANG_LAZY(crushStepsA, 3.f + 26.f * (1.f - clamp(localEdge + voice.crushAmt * 0.45f, 0.f, 1.f)));
			CLANG_LAZY(crushedMix, std::round((body() * 0.55f + noise * 0.70f + click() * 0.25f) * crushStepsA()) / crushStepsA());
			CLANG_LAZY(ring, (s1 * 0.62f + std::sin(voice.phase2 * 1.013f) * 0.38f) * voice.bodyEnv * voice.bodyLevel);
			CLANG_LAZY(tube, std::tanh(low() * 1.4f + body() * 0.32f) * (0.9f + 0.35f * damp));
			CLANG_LAZY(wire, fold(std::sin(voice.phase2 + s1 * 0.8f) * voice.bodyEnv * 0.55f + noise * 0.35f, clamp(localEdge + 0.15f, 0.f, 1.f)));
			CLANG_LAZY(membrane, std::sin(voice.phaseSub + s1 * (0.35f + 0.80f * voice.pitchEnv)) * voice.subEnv);
			CLANG_LAZY(glass, (std::sin(voice.phase1 * 1.003f) + std::sin(voice.phase2 * 2.011f) * 0.63f + std::sin(voice.phase1 * 3.917f) * 0.27f) * voice.bodyEnv * 0.52f);
			CLANG_LAZY(spring, fold(std::sin(voice.phase1 + phase2Base() * (2.2f + 4.8f * localEdge)) * voice.bodyEnv, 0.10f + localEdge * 0.60f));
			CLANG_LAZY(pulse, (s1 >= 0.f ? 1.f : -1.f) * voice.bodyEnv);
			CLANG_LAZY(grain, std::sin(voice.phase2 * (2.0f + 0.37f * (float)soundV) + std::sin(voice.phase1 * 0.51f) * 3.1f) * voice.noiseEnv);
			CLANG_LAZY(cluster, (s1 * 0.45f + std::sin(voice.phase1 * 1.503f) * 0.31f + std::sin(voice.phase1 * 2.017f) * 0.24f) * voice.bodyEnv);

			float soundOut = 0.f;
			if (engineV == 0) {
				switch (modelV) {
					case 0: // Tick: dry electronic and mechanical micro-transients.
						switch (soundV) {
							case 0: soundOut = tonalClick * 1.65f; break;
							case 1: soundOut = chirp() * 1.05f + tonalClick * 0.42f; break;
							case 2: soundOut = tonalClick * 0.92f + ghostTone() * 0.88f + low() * 0.18f; break;
							case 3: soundOut = pulse() * 0.62f + tonalClick * 0.46f; break;
							case 4: soundOut = glass() * 0.96f + tonalClick * 0.22f; break;
							case 5: soundOut = membrane() * 0.82f + tonalClick * 0.64f; break;
							case 6: soundOut = crushedMix() * 0.72f + digitalDust() * voice.noiseEnv * noiseAmount * 0.42f; break;
							default: soundOut = chirp() * 0.76f + burstNoise() * voice.noiseEnv * noiseAmount * 0.68f + ghost() * 0.34f; break;
						}
						break;
					case 1: // Dust: eight differently filtered particulate textures.
						switch (soundV) {
							case 0: soundOut = grain() * 0.38f + airNoise() * voice.noiseEnv * noiseAmount * 1.10f; break;
							case 1: soundOut = grain() * 0.24f + sandNoise() * voice.noiseEnv * noiseAmount * 1.20f; break;
							case 2: soundOut = ghostTone() * 0.34f + hp * voice.noiseEnv * noiseAmount * 0.88f; break;
							case 3: soundOut = tapeNoise() * voice.noiseEnv * noiseAmount + low() * 0.12f; break;
							case 4: soundOut = grain() * (0.42f + noiseAmount * 0.38f) + grainNoise() * voice.noiseEnv * noiseAmount * 0.72f; break;
							case 5: soundOut = steamNoise() * noiseAmount * 1.08f + chirp() * 0.16f; break;
							case 6: soundOut = ghostTone() * 0.52f + airNoise() * voice.noiseEnv * noiseAmount * 0.76f; break;
							default: soundOut = burstNoise() * voice.noiseEnv * noiseAmount * 1.18f + pulse() * 0.12f; break;
						}
						break;
					case 2: // Metal: plates, rims, bolts and tanks use different mode sets.
						switch (soundV) {
							case 0: soundOut = ring() * 0.72f + glass() * 0.55f; break;
							case 1: soundOut = tonalClick * 0.72f + ring() * 0.48f; break;
							case 2: soundOut = membrane() * 0.62f + tonalClick * 0.58f + ring() * 0.24f; break;
							case 3: soundOut = glass() * 1.02f + metalDust() * voice.noiseEnv * noiseAmount * 0.18f; break;
							case 4: soundOut = s1 * voice.bodyEnv * 0.84f + s2() * voice.bodyEnv * 0.28f; break;
							case 5: soundOut = membrane() * 0.82f + ring() * 0.56f; break;
							case 6: soundOut = tonalClick * 0.68f + ghostTone() * 0.62f + glass() * 0.22f; break;
							default: soundOut = glass() * 0.82f + fold(ring(), localEdge) * 0.48f + metalDust() * voice.noiseEnv * noiseAmount * 0.32f; break;
						}
						break;
					case 3: // Wire: tension, spring and electrical gestures.
						switch (soundV) {
							case 0: soundOut = spring() * 0.92f + s1 * voice.bodyEnv * 0.26f; break;
							case 1: soundOut = low() * 0.52f + wire() * 0.64f; break;
							case 2: soundOut = spring() * 1.08f + ghostTone() * 0.34f; break;
							case 3: soundOut = s1 * voice.bodyEnv * 0.42f + chirp() * 0.66f; break;
							case 4: soundOut = pulse() * 0.36f + wire() * 0.82f + noise * 0.22f; break;
							case 5: soundOut = chirp() * 0.92f + tonalClick * 0.28f + noise * 0.20f; break;
							case 6: soundOut = fold(wire() + ghostTone() * 0.32f, localEdge) * 0.92f; break;
							default: soundOut = tonalClick * 0.62f + spring() * 0.58f + burstNoise() * voice.noiseEnv * noiseAmount * 0.24f; break;
						}
						break;
					case 4: // Tube: low, hollow objects with recognisably different cavities.
						switch (soundV) {
							case 0: soundOut = tube() * 1.05f; break;
							case 1: soundOut = tube() * 0.68f + ring() * 0.36f + tonalClick * 0.22f; break;
							case 2: soundOut = membrane() * 1.02f + pulse() * 0.12f; break;
							case 3: soundOut = low() * 0.48f + chirp() * 0.52f + ghostTone() * 0.22f; break;
							case 4: soundOut = membrane() * 0.94f + low() * 0.42f; break;
							case 5: soundOut = std::sin(voice.phase1 + s2() * 0.34f) * voice.bodyEnv * 0.76f + low() * 0.31f; break;
							case 6: soundOut = pulse() * 0.27f + tube() * 0.78f; break;
							default: soundOut = tube() * 0.72f + glass() * 0.31f + ghostTone() * 0.38f; break;
						}
						break;
					case 5: // Scrap: asymmetric impacts, drags and loose assemblies.
						switch (soundV) {
							case 0: soundOut = fold(ring() * 0.55f + tonalClick * 0.48f, localEdge * 0.42f); break;
							case 1: soundOut = scrape() * 0.82f + grain() * 0.28f + noise * 0.24f; break;
							case 2: soundOut = low() * 0.52f + cluster() * 0.58f + tonalClick * 0.25f; break;
							case 3: soundOut = ring() * 0.42f + ghostTone() * 0.84f + metalDust() * voice.noiseEnv * noiseAmount * 0.24f; break;
							case 4: soundOut = cluster() * 0.48f + scrape() * 0.52f + noise * 0.32f; break;
							case 5: soundOut = pulse() * 0.22f + spring() * 0.76f + low() * 0.18f; break;
							case 6: soundOut = tonalClick * 0.86f + membrane() * 0.36f; break;
							default: soundOut = fold(cluster() * 0.62f + burstNoise() * voice.noiseEnv * noiseAmount * 0.62f, localEdge) + low() * 0.28f; break;
						}
						break;
					case 6: // Fold: tonal material driven through distinct nonlinear paths.
						switch (soundV) {
							case 0: soundOut = fold(body(), 0.25f + localEdge * 0.52f); break;
							case 1: soundOut = fold(low() * 0.62f + body() * 0.55f, localEdge * 0.76f); break;
							case 2: soundOut = fold(chirp() * 0.92f + tonalClick * 0.16f, 0.45f + localEdge); break;
							case 3: soundOut = std::tanh((body() + membrane() * 0.28f) * (2.2f + 5.f * edge)); break;
							case 4: soundOut = fold(spring() * 0.78f + low() * 0.25f, localEdge) + ghostTone() * 0.18f; break;
							case 5: soundOut = pulse() * 0.62f + fold(body(), 0.72f + localEdge) * 0.58f; break;
							case 6: soundOut = fold(chirp() + glass() * 0.34f, 0.78f + localEdge) * 0.88f + noise * 0.18f; break;
							default: soundOut = fold(wire() * 0.82f + tonalClick * 0.24f, 0.58f + localEdge) + ghostTone() * 0.31f; break;
						}
						break;
					default: // Crush: clocked, aliased and packet-like transients.
						switch (soundV) {
							case 0: soundOut = std::round(body() * 4.f) * 0.25f + tonalClick * 0.22f; break;
							case 1: soundOut = std::round(chirp() * 9.f) / 9.f; break;
							case 2: { // Shatter: a short digital fracture, not a generic hit.
								float fracture = pulse() * 0.52f + chirp() * 0.34f + std::round(body() * 7.f) / 7.f * 0.42f;
								soundOut = fold(fracture + digitalDust() * voice.noiseEnv * noiseAmount * 0.32f, clamp(localEdge + 0.24f, 0.f, 1.f)) * 1.18f;
								break;
							}
							case 3: soundOut = pulse() * 0.34f + digitalDust() * voice.noiseEnv * noiseAmount * 0.62f + ghostTone() * 0.25f; break;
							case 4: soundOut = std::round(cluster() * 13.f) / 13.f + tonalClick * 0.16f; break;
							case 5: soundOut = fold(std::round(body() * 6.f) / 6.f + ghostTone() * 0.35f, localEdge + 0.22f); break;
							case 6: soundOut = digitalDust() * voice.noiseEnv * noiseAmount * 0.85f + grain() * 0.28f; break;
							default: soundOut = fold(crushedMix() + burstNoise() * voice.noiseEnv * noiseAmount * 0.48f, 0.38f + localEdge) + low() * 0.18f; break;
						}
						break;
				}
			}
			else {
				float modEnv = voice.noiseEnv * (0.35f + voice.fmIndex * (0.75f + 2.8f * edge + 4.5f * motionAmount));
					CLANG_LAZY(carrier, std::sin(voice.phase1 + phase2Base() * modEnv + motionPhase() * (0.55f + 1.25f * motionAmount) + noise * edge * 0.20f));
					CLANG_LAZY(side, std::sin(voice.phase2 * (1.f + 0.02f * (float)modelV) + motionPhase() * 0.72f));
					CLANG_LAZY(fm, (carrier() * 0.78f + side() * 0.22f) * voice.bodyEnv);
				CLANG_LAZY(crushSteps, 4.f + 44.f * (1.f - clamp(edge * 0.82f + voice.crushAmt * 0.42f, 0.f, 1.f)));
					CLANG_LAZY(digital, std::round((fm() + noise * 0.35f) * crushSteps()) / crushSteps());
					CLANG_LAZY(fmBell, (std::sin(voice.phase1 + side() * modEnv * 0.42f) * 0.52f + std::sin(voice.phase2 * 1.997f) * 0.48f) * voice.bodyEnv);
					CLANG_LAZY(fmZap, fold(carrier() * 0.80f + chirp() * 0.35f + click() * 0.16f, clamp(localEdge + 0.22f, 0.f, 1.f)));
					CLANG_LAZY(fmStack, fm() + std::sin(voice.phase1 * 1.498f) * voice.bodyEnv * 0.34f + std::sin(voice.phase1 * 2.002f) * voice.bodyEnv * 0.22f);
					CLANG_LAZY(phaseBody, fold(carrier() * voice.bodyEnv + side() * voice.bodyEnv * 0.30f, 0.25f + localEdge * 0.75f));
					CLANG_LAZY(fmFault, fold(digital() * 0.78f + noise * 0.55f + ghost() * 0.35f, clamp(localEdge + voice.crushAmt * 0.30f, 0.f, 1.f)));
				switch (modelV) {
					case 0: // Ping
						switch (soundV) {
							case 0: soundOut = carrier() * voice.bodyEnv * 0.72f + tonalClick * 0.42f; break;
							case 1: soundOut = chirp() * 0.88f + tonalClick * 0.28f; break;
							case 2: soundOut = fm() * 0.68f + membrane() * 0.42f; break;
							case 3: soundOut = low() * 0.72f + fm() * 0.38f; break;
							case 4: soundOut = carrier() * voice.bodyEnv * 0.58f + ghostTone() * 0.46f; break;
							case 5: // Strike: short metallic impact, never a sustained carrier.
								soundOut = fold(tonalClick * 0.92f + spring() * 0.62f + glass() * 0.24f
									+ metalDust() * voice.noiseEnv * (0.24f + 0.18f * noiseAmount),
									0.42f + localEdge * 0.38f);
								break;
							case 6: soundOut = pulse() * 0.28f + fm() * 0.72f; break;
							default: // Shard: a short metallic splinter, not a ringing carrier.
								soundOut = fold(glass() * 0.58f + spring() * 0.46f + tonalClick * 0.34f
									+ metalDust() * voice.noiseEnv * (0.20f + 0.16f * noiseAmount),
									0.36f + localEdge * 0.34f);
								break;
						}
						break;
					case 1: // Ratio
						switch (soundV) {
							case 0: soundOut = fm() * 0.68f + low() * 0.48f; break;
							case 1: soundOut = fm() * 0.82f + side() * voice.bodyEnv * 0.26f; break;
							case 2: soundOut = fm() * 0.62f + membrane() * 0.64f; break;
							case 3: soundOut = fm() * 0.88f + glass() * 0.24f; break;
							case 4: soundOut = carrier() * voice.bodyEnv * 0.58f + side() * voice.bodyEnv * 0.51f; break;
							case 5: soundOut = fold(fm(), localEdge * 0.48f) + low() * 0.26f; break;
							case 6: soundOut = fmBell() * 0.82f + side() * voice.bodyEnv * 0.33f; break;
							default: soundOut = cluster() * 0.38f + fm() * 0.84f; break;
						}
						break;
					case 2: // Bell
						switch (soundV) {
							case 0: soundOut = fmBell() * 0.86f + glass() * 0.42f; break;
							case 1: soundOut = fmBell() * 1.05f + ring() * 0.18f; break;
							case 2: soundOut = glass() * 0.92f + carrier() * voice.bodyEnv * 0.22f; break;
							case 3: soundOut = fold(fmBell(), 0.30f + localEdge * 0.55f); break;
							case 4: soundOut = low() * 0.36f + fmBell() * 0.73f; break;
							case 5: soundOut = fmBell() * 0.58f + ghostTone() * 0.82f; break;
							case 6: soundOut = phaseBody() * 0.72f + glass() * 0.42f; break;
							default: soundOut = cluster() * 0.44f + fmBell() * 0.78f + metalDust() * voice.noiseEnv * noiseAmount * 0.18f; break;
						}
						break;
					case 3: // Arc
						switch (soundV) {
			case 0: soundOut = fmZap() * 0.78f + tonalClick * 0.20f; break;
							case 1: soundOut = tonalClick * 0.74f + chirp() * 0.58f; break;
							case 2: soundOut = chirp() * 0.88f + ghostTone() * 0.42f; break;
							case 3: soundOut = fmZap() * 0.72f + ghostTone() * 0.68f; break;
							case 4: soundOut = spring() * 0.44f + fmZap() * 0.76f; break;
							case 5: soundOut = tonalClick * 0.88f + fm() * 0.24f; break;
							case 6: soundOut = fmZap() * 0.88f + chirp() * 0.32f + pulse() * 0.18f + low() * 0.12f + noise * 0.12f; break;
							default: soundOut = fmZap() * 0.86f + chirp() * 0.36f + pulse() * 0.22f + burstNoise() * voice.noiseEnv * noiseAmount * 0.24f; break;
						}
						break;
					case 4: // Phase
						switch (soundV) {
							case 0: soundOut = phaseBody() * 0.88f; break;
							case 1: soundOut = carrier() * voice.bodyEnv * 0.56f + phaseBody() * 0.48f; break;
							case 2: soundOut = phaseBody() * 0.72f + side() * voice.bodyEnv * 0.44f; break;
							case 3: soundOut = fold(phaseBody() + low() * 0.22f, localEdge * 0.70f); break;
							case 4: soundOut = pulse() * 0.31f + phaseBody() * 0.74f; break;
							case 5: soundOut = spring() * 0.42f + phaseBody() * 0.72f; break;
							case 6: soundOut = fold(phaseBody() + chirp() * 0.38f, 0.48f + localEdge); break;
							default: soundOut = -phaseBody() * 0.68f + ghostTone() * 0.54f + fm() * 0.28f; break;
						}
						break;
					case 5: // Stack
						switch (soundV) {
							case 0: soundOut = fmStack() * 0.72f + low() * 0.28f; break;
							case 1: soundOut = cluster() * 0.62f + fm() * 0.54f; break;
							case 2: soundOut = cluster() * 0.84f + low() * 0.31f; break;
							case 3: soundOut = fmStack() * 0.58f + glass() * 0.42f; break;
							case 4: soundOut = membrane() * 0.64f + cluster() * 0.48f; break;
							case 5: soundOut = low() * 0.52f + fmStack() * 0.72f + pulse() * 0.13f; break;
							case 6: soundOut = fold(cluster() + fm() * 0.32f, localEdge + 0.25f); break;
							default: soundOut = cluster() * 0.75f + ghostTone() * 0.64f + fm() * 0.34f; break;
						}
						break;
					case 6: // Bits
						switch (soundV) {
							case 0: soundOut = digital() * 0.68f + digitalDust() * voice.noiseEnv * noiseAmount * 0.32f; break;
							case 1: soundOut = std::round(fm() * 5.f) / 5.f; break;
							case 2: soundOut = pulse() * 0.31f + digital() * 0.66f + noise * 0.18f; break;
							case 3: soundOut = digital() * 0.42f + tapeNoise() * voice.noiseEnv * noiseAmount * 0.58f; break;
							case 4: soundOut = std::round(phaseBody() * 11.f) / 11.f + grain() * 0.21f; break;
							case 5: soundOut = digital() * 0.55f + ghostTone() * 0.74f; break;
							case 6: soundOut = tonalClick * 0.76f + digital() * 0.38f; break;
							default: soundOut = fold(digital() * 0.74f + burstNoise() * voice.noiseEnv * noiseAmount * 0.48f, localEdge + 0.32f); break;
						}
						break;
					default: // Fault
						switch (soundV) {
							case 0: soundOut = std::round(fmFault() * 4.f) * 0.25f; break;
							case 1: soundOut = std::round(fmFault() * 6.f) / 6.f; break;
							case 2: soundOut = std::round(fmFault() * 8.f) * 0.125f; break;
							case 3: soundOut = fold(digital() + side() * voice.bodyEnv * 0.22f, 0.34f + localEdge); break;
							case 4: soundOut = std::tanh(fm() * (3.2f + 6.f * edge)) * 0.72f; break;
							case 5: soundOut = fmFault() * 0.78f + ghostTone() * 0.58f; break;
							case 6: soundOut = fold(fmFault() + chirp() * 0.42f, 0.62f + localEdge) + noise * 0.18f; break;
							default: soundOut = fold(digital() * 0.62f + cluster() * 0.44f + burstNoise() * voice.noiseEnv * noiseAmount * 0.46f, 0.74f + localEdge); break;
						}
						break;
				}
			}
			// Noise remains model/sound-shaped, but the Noise knob is now audible
			// on every voice rather than only on a few special variants.
			float noiseCharacter = noise * (1.35f + 0.18f * (float)modelV + 0.10f * (float)soundV);
			soundOut += noiseCharacter;
			// A smoothstep onset keeps the attack duration unchanged, but removes the
			// steep first-sample slope that made random phase starts click unevenly.
			float attackGain = voice.attackEnv * voice.attackEnv * (3.f - 2.f * voice.attackEnv);
			soundOut *= attackGain;
			float bodyMix = 0.58f + 0.82f * bodyControl;
			// Transients and motion are now supplied by the selected sound route;
			// no universal click or drone is added after the model-specific DSP.
			soundOut *= bodyMix;
			float brightTrim = 1.f - 0.18f * clamp(voice.bright, 0.f, 1.f);
			float transientWeight = std::exp(-voice.age / 0.018f);
			float levelTrim = voice.bodyTrim + (voice.transientTrim - voice.bodyTrim) * transientWeight;
			soundOut *= brightTrim * voice.outputTrim * levelTrim;

			out += soundOut;
			activeEnergy += clamp(voice.bodyEnv + voice.noiseEnv + voice.clickEnv + voice.subEnv + voice.ghostEnv, 0.f, 1.f);
		}
		#undef CLANG_LAZY

		// Keep dense overlaps punchy instead of letting eight tails saturate
		// into a continuous drone when Decay is high.
		float overlapGain = 1.f / std::sqrt(std::max(1.f, activeEnergy));
		out *= overlapGain;
		out = dcBlock(out);
		out = std::tanh(out * 2.1f) * 5.f;
		outputs[OUT_OUTPUT].setVoltage(clamp(out, -10.f, 10.f));
	}
};

struct ClangDisplay : Widget {
	Clang* module = nullptr;
	std::shared_ptr<Font> font;

	ClangDisplay() {
		box.size = Vec(130.933f, 22.946f);
	}

	void draw(const DrawArgs& args) override {
		if (!font)
			font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font)
			return;
		int engine = module ? clamp((int)std::round(module->params[Clang::ENGINE_PARAM].getValue()), 0, 1) : 0;
		int model = module ? clamp((int)std::round(module->params[Clang::MODEL_PARAM].getValue()), 0, 7) : 0;
		int sound = module ? clamp((int)std::round(module->params[Clang::SOUND_PARAM].getValue()), 0, 7) : 0;
		if (engine == 1)
			model = clangFmModelOrder[model];
		std::string txt = engine == 0
			? std::string(clangPhysicalModelNames[model]) + " -> " + clangPhysicalSoundNames[model][sound]
			: std::string(clangFmModelNames[model]) + " -> " + clangFmSoundNames[model][sound];
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, 11.f);
		nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, txt.c_str(), NULL);
	}
};

struct ClangWidget : SubmitModuleWidget {
	ClangWidget(Clang* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Clang.svg")));

		auto* display = createWidget<ClangDisplay>(Vec(18.274f, 38.988f));
		display->module = module;
		addChild(display);

		addParam(createParamCentered<ClangReactSmallKnob>(Vec(34.362f, 104.772f), module, Clang::MODEL_PARAM));
		addParam(createParam<CKSS>(Vec(115.317f, 96.846f), module, Clang::ENGINE_PARAM));
		addParam(createParamCentered<ClangReactSmallKnob>(Vec(83.121f, 104.772f), module, Clang::SOUND_PARAM));
		addParam(createParamCentered<ClangReactSmallKnob>(Vec(34.362f, 198.627f), module, Clang::TUNE_PARAM));
		addParam(createParamCentered<ClangReactSmallKnob>(Vec(83.191f, 198.627f), module, Clang::DECAY_PARAM));
		addParam(createParamCentered<ClangReactSmallKnob>(Vec(132.020f, 198.627f), module, Clang::NOISE_PARAM));
		addParam(createParamCentered<ClangReactSmallKnob>(Vec(34.362f, 289.851f), module, Clang::MOTION_PARAM));
		addParam(createParamCentered<ClangReactSmallKnob>(Vec(83.191f, 289.851f), module, Clang::BODY_PARAM));
		addParam(createParamCentered<ClangReactSmallKnob>(Vec(132.020f, 289.851f), module, Clang::VARIATION_PARAM));

		addInput(createInputCentered<PJ301MPort>(Vec(33.797f, 141.144f), module, Clang::MODEL_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(82.924f, 141.144f), module, Clang::SOUND_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(33.797f, 234.472f), module, Clang::TUNE_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(82.924f, 234.472f), module, Clang::DECAY_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(131.290f, 234.472f), module, Clang::NOISE_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(33.797f, 342.132f), module, Clang::MOTION_INPUT));
		addInput(createInputCentered<PJ301MPort>(Vec(82.924f, 342.132f), module, Clang::TRIG_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(Vec(131.912f, 342.132f), module, Clang::OUT_OUTPUT));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(63.410f, 321.463f), module, Clang::TRIGGER_LIGHT));
		}

		void appendContextMenu(Menu* menu) override {
			menu->addChild(new MenuSeparator);
			menu->addChild(createMenuItem("Manual", "", []() {
				system::openBrowser("https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/clang/");
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

Model* modelClang = createModel<Clang, ClangWidget>("Clang");
