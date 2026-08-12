#include "plugin.hpp"
#include <cmath>
#include <cstring>
#include <string>

struct ReactImpactPatternKnob : SvgKnob {
    ReactImpactPatternKnob() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobMedium.svg")));
        shadow->opacity = 0.f;
    }
};

struct ReactImpactSmallKnob : SvgKnob {
    ReactImpactSmallKnob() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/SubmitKnobSmall.svg")));
        shadow->opacity = 0.f;
    }
};

struct ReactDropButton : SvgSwitch {
    ReactDropButton() {
        momentary = true;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChainMuteButton_0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChainMuteButton_1.svg")));
    }
};

struct ReactVariationButton : SvgSwitch {
    ReactVariationButton() {
        momentary = false;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChainMuteButton_0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/ChainMuteButton_1.svg")));
    }
};

// ============================================================
//  REACT V2 — Groove Behavior Engine v4
//  GENRE switch (8 genres) + MORPH sticky (12 varianten per genre)
//  Deterministisch — puur position weights
//  MetaModule-proof
// ============================================================

struct Pattern {
    bool a[16], b[16], c[16], d[16];
};

static const Pattern PATTERNS[8][12] = {

// ================================================================
// GENRE 0: TECHNO
// ================================================================
{{
// 0 FLOOR
{1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{0,0,0,0, 0,0,0,0, 0,0,0,1, 0,0,0,0},
},{
// 1 FLOOR+
{1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0},
{0,0,0,1, 1,0,0,0, 0,0,0,1, 1,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{0,0,0,0, 0,0,0,1, 0,0,0,0, 0,0,0,1},
},{
// 2 BREAK1
{1,0,1,0, 0,0,1,0, 1,0,0,0, 1,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,1,0,1, 0,1,0,0, 0,1,0,1, 0,1,0,0},
{0,0,0,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
},{
// 3 BREAK2
{1,0,0,0, 0,0,1,0, 1,0,0,1, 0,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,1,0, 0,1,0,0, 0,0,1,0, 0,1,0,0},
{0,0,0,0, 0,0,0,0, 0,0,0,1, 0,0,0,0},
},{
// 4 MINIMAL
{1,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,1,0},
},{
// 5 HALF
{1,0,0,0, 0,0,0,1, 1,0,0,0, 0,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,1,0,0},
},{
// 6 SHUFFLE
{1,0,0,1, 0,0,1,0, 0,1,0,0, 1,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,1,0,0, 0,1,0,0, 0,1,0,0, 0,1,0,0},
{0,0,0,1, 0,0,0,0, 0,0,0,1, 0,0,0,0},
},{
// 7 SYNCO1
{1,0,0,1, 0,0,0,0, 1,0,0,0, 1,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,1,0},
{0,0,1,0, 0,1,0,0, 0,0,1,0, 0,0,0,1},
{0,0,0,0, 0,0,0,1, 0,0,0,0, 0,0,0,0},
},{
// 8 SYNCO2
{1,0,0,0, 0,1,0,0, 1,0,0,0, 0,1,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,1,0, 0,0,0,1, 0,0,1,0, 0,0,0,0},
{0,0,0,0, 0,0,0,0, 0,0,0,1, 0,0,1,0},
},{
// 9 CLUSTER
{1,1,0,0, 0,0,0,0, 1,1,0,0, 0,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{0,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,1,0},
},{
// 10 SPARSE
{1,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,0,0},
{0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
{0,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,1},
},{
// 11 BERLIN
{1,0,1,0, 0,0,0,0, 1,0,1,0, 0,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,1,0,1, 0,0,0,0, 0,1,0,1, 0,0,0,0},
{0,0,0,0, 0,0,0,1, 0,0,0,0, 0,0,0,1},
}},

// ================================================================
// GENRE 1: HOUSE
// ================================================================
{{
// 0 FLOOR
{1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{0,0,0,1, 0,0,0,0, 0,0,0,1, 0,0,0,0},
},{
// 1 DEEP
{1,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,1},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{0,0,0,0, 0,0,0,1, 0,0,0,0, 0,0,1,0},
},{
// 2 CLASSIC
{1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,1,0, 0,1,1,0, 0,0,1,0, 0,1,1,0},
{0,0,0,1, 0,0,0,0, 0,0,1,0, 0,0,0,1},
},{
// 3 GARAGE
{1,0,0,0, 0,0,1,0, 1,0,0,0, 0,0,1,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,1,1,0, 0,0,1,0, 0,1,1,0, 0,0,1,0},
{0,0,0,1, 0,1,0,0, 0,0,0,1, 0,0,1,0},
},{
// 4 JACK
{1,0,0,0, 1,0,0,0, 1,0,0,1, 1,0,0,0},
{0,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0},
{0,0,1,0, 0,1,1,0, 0,0,1,0, 0,1,1,0},
{0,1,0,0, 0,0,0,1, 0,1,0,0, 0,0,1,0},
},{
// 5 LATIN
{1,0,0,0, 0,0,0,1, 1,0,0,0, 0,0,0,0},
{0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
{0,1,0,1, 0,0,1,0, 0,1,0,1, 0,0,1,0},
{0,0,0,1, 0,1,0,0, 0,0,1,0, 0,1,0,1},
},{
// 6 ACID
{1,0,0,0, 0,0,1,0, 0,0,0,0, 1,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,1, 1,0,0,0},
{0,0,1,0, 0,1,1,0, 0,0,1,0, 0,1,1,0},
{0,1,0,0, 0,0,0,1, 0,1,0,0, 0,0,1,0},
},{
// 7 DISCO
{1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0},
{0,0,0,0, 1,0,0,1, 0,0,0,0, 1,0,0,0},
{0,1,1,0, 0,1,1,0, 0,1,1,0, 0,1,1,0},
{0,0,0,1, 0,0,1,0, 0,0,0,1, 0,0,1,0},
},{
// 8 MINIMAL
{1,0,0,0, 0,0,0,0, 1,0,0,1, 0,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,1,0},
{0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,1,0},
{0,0,0,0, 0,0,0,1, 0,0,0,0, 0,0,0,0},
},{
// 9 SWING
{1,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,1,0},
{0,0,0,1, 0,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,1,0, 0,1,0,0, 0,0,1,0, 0,1,0,0},
{0,1,0,0, 0,0,0,1, 0,1,0,0, 0,0,1,0},
},{
// 10 PEAK
{1,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,1,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,1,1, 0,1,1,0, 0,0,1,1, 0,1,1,0},
{0,1,0,0, 0,0,1,0, 0,1,0,0, 0,0,1,1},
},{
// 11 AFTER
{1,0,0,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,0,1},
{0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
{0,0,0,0, 0,0,0,1, 0,0,0,0, 0,0,1,0},
}},

// ================================================================
// GENRE 2: BROKEN
// ================================================================
{{
// 0 BASIC
{0,0,1,0, 0,1,0,0, 0,0,1,0, 1,0,0,0},
{1,0,0,0, 0,0,0,1, 0,0,0,0, 0,1,0,0},
{0,1,0,0, 1,0,0,0, 0,1,0,1, 0,0,0,0},
{0,0,0,1, 0,0,1,0, 0,0,0,0, 0,0,1,0},
},{
// 1 SYNCO
{0,1,0,0, 1,0,0,1, 0,0,0,0, 1,0,1,0},
{1,0,0,0, 0,0,1,0, 0,1,0,0, 0,0,0,1},
{0,0,1,0, 0,1,0,0, 1,0,0,1, 0,0,0,0},
{0,0,1,0, 0,0,0,1, 0,1,0,0, 0,0,0,0},
},{
// 2 OFFBEAT
{0,0,0,1, 0,0,0,0, 0,1,0,0, 0,0,1,0},
{1,0,0,0, 0,1,0,0, 1,0,0,0, 0,0,0,1},
{0,1,0,0, 1,0,1,0, 0,0,0,1, 0,1,0,0},
{0,1,0,0, 0,0,0,0, 0,0,1,0, 1,0,0,0},
},{
// 3 D&B
{1,0,0,0, 0,0,1,0, 0,1,0,0, 0,0,1,0},
{0,0,1,0, 1,0,0,0, 0,0,0,1, 1,0,0,0},
{1,0,0,1, 0,0,1,0, 1,0,0,0, 0,1,0,0},
{0,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1},
},{
// 4 JUNGLE
{1,0,0,1, 0,0,0,0, 0,1,0,0, 1,0,0,0},
{0,0,1,0, 1,0,0,1, 0,0,0,0, 0,1,0,1},
{0,1,0,0, 0,0,1,0, 1,0,0,1, 0,0,0,0},
{0,0,1,0, 0,0,0,1, 0,0,1,0, 0,0,0,0},
},{
// 5 IDM
{0,1,0,1, 0,0,1,0, 1,0,0,0, 0,1,0,0},
{1,0,0,0, 1,0,0,1, 0,0,1,0, 1,0,0,0},
{0,0,1,0, 0,1,0,0, 0,1,0,0, 1,0,0,1},
{0,1,0,1, 0,0,0,0, 1,0,0,0, 0,0,1,0},
},{
// 6 GLITCH
{1,1,0,0, 0,0,0,1, 0,0,1,0, 0,1,0,0},
{0,0,0,1, 1,0,1,0, 0,1,0,0, 1,0,0,1},
{1,0,0,0, 0,1,0,1, 1,0,0,0, 0,0,1,0},
{1,0,0,0, 0,0,1,0, 0,0,0,1, 0,1,0,0},
},{
// 7 HALF
{1,0,0,0, 0,0,0,0, 0,0,1,0, 0,1,0,0},
{0,0,0,1, 1,0,0,0, 0,1,0,0, 0,0,0,1},
{0,1,0,0, 0,0,1,0, 1,0,0,0, 0,0,1,0},
{0,0,1,0, 0,0,0,0, 0,0,0,0, 1,0,0,0},
},{
// 8 TECH
{1,0,0,0, 0,1,0,0, 1,0,0,0, 0,0,1,0},
{0,0,1,0, 1,0,0,0, 0,0,0,1, 1,0,0,0},
{0,1,0,1, 0,0,0,1, 0,1,0,0, 1,0,0,0},
{0,0,0,1, 0,0,1,0, 0,1,0,0, 0,0,0,1},
},{
// 9 MINIMAL
{0,0,0,0, 1,0,0,0, 0,0,0,1, 0,0,0,0},
{1,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,1,0},
{0,0,1,0, 0,0,0,1, 0,1,0,0, 0,0,0,0},
{0,0,0,0, 0,1,0,0, 0,0,0,0, 0,0,1,0},
},{
// 10 LIQUID
{1,0,0,0, 0,0,1,0, 0,0,0,1, 0,1,0,0},
{0,0,1,0, 1,0,0,0, 1,0,0,0, 0,0,0,1},
{0,1,0,1, 0,0,0,0, 0,0,1,0, 1,0,1,0},
{0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,0,1},
},{
// 11 CHAOS
{1,0,1,0, 0,1,0,1, 0,0,0,1, 1,0,0,0},
{0,1,0,0, 1,0,0,0, 1,0,1,0, 0,0,1,0},
{1,0,0,1, 0,0,1,0, 0,1,0,0, 1,0,0,1},
{1,0,0,1, 0,1,0,0, 1,0,0,0, 0,1,0,1},
}},

// ================================================================
// GENRE 3: INDUSTRIAL
// ================================================================
{{
// 0 BASIC
{1,1,0,0, 1,1,0,0, 1,1,0,0, 1,1,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{1,0,1,0, 1,0,1,0, 1,0,1,0, 1,0,1,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
},{
// 1 STOMP
{1,0,1,0, 1,0,1,0, 1,0,1,0, 1,0,1,0},
{0,1,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,1},
{1,1,0,0, 1,1,0,0, 1,1,0,0, 1,1,0,0},
{0,0,1,1, 0,0,1,1, 0,0,1,1, 0,0,1,1},
},{
// 2 MARCH
{1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{1,0,1,0, 1,0,1,0, 1,0,1,0, 1,0,1,0},
{0,1,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,1},
},{
// 3 PULSE
{1,1,1,0, 0,0,0,0, 1,1,1,0, 0,0,0,0},
{0,0,0,1, 1,1,0,0, 0,0,0,1, 1,1,0,0},
{1,0,0,1, 0,0,1,0, 1,0,0,1, 0,0,1,0},
{0,0,1,0, 0,1,0,0, 0,0,1,0, 0,1,0,0},
},{
// 4 GRIND
{1,1,0,1, 1,0,1,1, 0,1,1,0, 1,1,0,1},
{0,0,1,0, 0,1,0,0, 1,0,0,1, 0,0,1,0},
{1,0,1,1, 0,1,0,1, 1,0,1,0, 1,1,0,1},
{1,0,1,0, 1,0,0,1, 0,1,0,1, 0,0,1,0},
},{
// 5 DRILL
{1,0,1,1, 0,0,1,0, 1,1,0,1, 0,0,0,1},
{0,1,0,0, 1,1,0,1, 0,0,1,0, 1,1,0,0},
{1,0,0,1, 0,1,1,0, 1,0,0,1, 0,1,0,0},
{0,1,0,0, 1,0,1,0, 0,1,0,0, 1,0,1,0},
},{
// 6 NOISE
{1,1,1,1, 0,0,0,0, 1,1,1,1, 0,0,0,0},
{0,0,0,0, 1,1,1,1, 0,0,0,0, 1,1,1,1},
{1,1,0,0, 1,1,0,0, 0,0,1,1, 0,0,1,1},
{1,1,0,0, 1,1,0,0, 0,0,1,1, 0,0,1,1},
},{
// 7 BURST
{1,1,1,0, 0,0,0,0, 0,0,1,1, 1,0,0,0},
{0,0,0,1, 1,1,0,0, 1,0,0,0, 0,1,1,0},
{1,0,0,0, 0,1,1,1, 0,0,0,0, 1,0,0,1},
{1,1,0,0, 0,0,0,0, 1,1,0,0, 0,0,0,0},
},{
// 8 METAL
{1,0,0,1, 0,1,0,0, 1,0,0,1, 0,1,0,0},
{0,1,1,0, 1,0,1,1, 0,1,1,0, 1,0,1,0},
{1,0,1,0, 0,1,0,1, 1,0,1,0, 0,1,0,1},
{0,1,0,1, 0,1,0,1, 1,0,1,0, 1,0,1,0},
},{
// 9 MINIMAL
{1,0,0,0, 0,0,1,0, 1,0,0,0, 0,0,1,0},
{0,0,1,0, 1,0,0,0, 0,0,1,0, 1,0,0,0},
{1,0,0,1, 0,0,0,1, 1,0,0,1, 0,0,0,1},
{0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
},{
// 10 TECHNO
{1,0,0,0, 1,0,1,0, 1,0,0,0, 1,0,0,1},
{0,1,0,1, 0,0,0,1, 0,1,0,1, 0,0,1,0},
{1,0,1,0, 0,1,0,0, 1,0,1,0, 0,1,0,1},
{0,0,0,1, 0,0,1,0, 0,0,0,1, 0,0,1,0},
},{
// 11 WALL
{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1},
{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
{1,0,1,0, 1,0,1,0, 1,0,1,0, 1,0,1,0},
{1,0,1,0, 1,0,1,0, 1,0,1,0, 1,0,1,0},
}},

// ================================================================
// GENRE 4: MACHINE
// ================================================================
{{
// 0 4/4
{1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1},
},{
// 1 8TH
{1,0,1,0, 1,0,1,0, 1,0,1,0, 1,0,1,0},
{0,1,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,1},
{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
{0,1,0,0, 0,1,0,0, 0,1,0,0, 0,1,0,0},
},{
// 2 16TH
{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1},
{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
{1,0,1,0, 1,0,1,0, 1,0,1,0, 1,0,1,0},
{0,1,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,1},
},{
// 3 HALF
{1,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0},
{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
{0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
},{
// 4 MOTORIK
{1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{0,1,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,1},
{0,0,1,0, 0,0,0,1, 0,0,1,0, 0,0,0,1},
},{
// 5 KRAUTROCK
{1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{0,1,0,0, 0,1,0,0, 0,1,0,0, 0,1,0,0},
{0,0,0,1, 0,0,0,1, 0,0,0,1, 0,0,0,1},
},{
// 6 GRID
{1,0,1,0, 0,0,1,0, 1,0,1,0, 0,0,0,0},
{0,1,0,0, 1,0,0,1, 0,1,0,0, 1,0,1,0},
{1,0,0,1, 0,1,0,0, 1,0,0,1, 0,1,0,0},
{0,0,1,0, 0,0,1,0, 0,0,0,1, 0,0,0,1},
},{
// 7 PULSE
{1,0,0,1, 0,0,1,0, 0,1,0,0, 1,0,0,0},
{0,1,0,0, 1,0,0,0, 1,0,0,1, 0,0,1,0},
{0,0,1,0, 0,1,0,1, 0,0,1,0, 0,1,0,1},
{0,1,0,0, 1,0,0,1, 0,0,1,0, 0,1,0,0},
},{
// 8 STEP
{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1},
{0,1,0,0, 1,0,0,0, 0,1,0,0, 1,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{0,0,0,1, 0,0,0,0, 0,0,0,1, 0,0,0,0},
},{
// 9 CLOCK
{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,0,0},
{0,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0},
{0,0,0,0, 0,1,0,0, 0,0,0,0, 0,1,0,0},
},{
// 10 SYNC
{1,0,1,0, 1,0,1,0, 1,0,1,0, 1,0,1,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,1,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,1},
{0,1,0,1, 0,0,0,0, 0,1,0,1, 0,0,0,0},
},{
// 11 COMPLEX
{1,0,0,1, 0,1,0,0, 1,0,0,0, 0,1,0,1},
{0,1,0,0, 1,0,0,1, 0,0,1,0, 1,0,0,0},
{0,0,1,0, 0,0,1,1, 0,1,0,0, 0,0,1,0},
{0,0,1,0, 0,1,0,1, 0,0,1,0, 1,0,0,1},
}},

// ================================================================
// GENRE 5: COLLAPSE
// ================================================================
{{
// 0 BEGIN
{1,0,0,0, 1,0,0,0, 1,0,0,0, 0,0,0,0},
{0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
{0,1,0,0, 0,0,0,0, 0,1,0,0, 0,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,0,0},
},{
// 1 CRACK
{1,0,0,0, 0,0,0,0, 1,0,1,0, 0,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,0,1},
{0,0,1,0, 0,0,0,0, 0,1,0,0, 0,0,0,0},
{0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
},{
// 2 STUTTER
{1,1,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0},
{0,0,0,1, 1,0,0,0, 0,0,0,0, 0,1,0,0},
{0,0,1,0, 0,0,0,0, 0,0,1,1, 0,0,0,0},
{0,0,1,0, 0,0,0,0, 0,0,0,0, 0,0,1,0},
},{
// 3 BREAK
{1,0,0,0, 0,0,1,1, 0,0,0,0, 0,0,0,0},
{0,0,1,0, 1,0,0,0, 0,1,0,0, 0,0,0,0},
{0,1,0,0, 0,0,0,0, 1,0,0,1, 0,0,0,0},
{0,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,0,0},
},{
// 4 ERUPT
{1,1,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
{0,0,0,1, 1,1,0,0, 0,0,0,0, 0,0,0,0},
{0,0,0,0, 0,0,1,1, 1,0,0,0, 0,0,0,0},
{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
},{
// 5 SILENCE
{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
{0,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0},
{0,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,0,0},
{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},
},{
// 6 FRAGMENT
{0,0,1,0, 0,0,0,0, 1,0,0,0, 0,1,0,0},
{1,0,0,0, 0,0,0,0, 0,0,0,1, 0,0,0,0},
{0,0,0,0, 1,0,0,1, 0,0,0,0, 0,0,1,0},
{0,0,0,0, 0,1,0,0, 0,0,0,0, 0,0,0,0},
},{
// 7 REBUILD
{0,0,0,0, 0,0,0,0, 1,0,0,0, 1,0,0,0},
{0,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,1,0},
{0,0,0,0, 1,0,0,0, 0,0,1,0, 0,0,0,1},
{0,0,0,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
},{
// 8 WAVE
{1,0,0,0, 0,0,0,0, 1,1,0,0, 0,0,0,0},
{0,0,0,0, 1,0,0,1, 0,0,0,0, 1,0,0,0},
{0,1,0,1, 0,0,0,0, 0,0,1,0, 0,0,0,0},
{0,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,1,0},
},{
// 9 GLITCH
{1,1,0,1, 0,0,0,0, 0,1,0,0, 0,0,1,0},
{0,0,1,0, 1,0,1,0, 0,0,0,1, 0,0,0,0},
{0,0,0,0, 0,1,0,1, 1,0,0,0, 0,1,0,0},
{0,1,0,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
},{
// 10 STORM
{1,1,0,0, 0,1,1,0, 0,0,1,0, 0,0,0,0},
{0,0,1,1, 0,0,0,1, 1,0,0,0, 0,1,0,0},
{1,0,0,0, 1,0,0,0, 0,1,1,0, 0,0,1,0},
{0,0,1,0, 0,1,0,0, 0,0,1,0, 0,1,0,0},
},{
// 11 END
{0,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1},
{0,0,0,0, 0,0,0,0, 1,1,0,0, 0,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
}},

// ================================================================
// GENRE 6: GHOST
// ================================================================
{{
// 0 WHISPER
{0,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,0,0},
{0,1,0,0, 1,0,0,0, 0,1,0,0, 1,0,0,0},
{1,0,0,1, 0,0,0,0, 1,0,0,0, 0,0,1,0},
{0,0,0,0, 0,0,0,1, 0,0,0,0, 0,0,0,0},
},{
// 1 TRACE
{0,0,1,0, 0,0,0,0, 0,0,0,0, 0,1,0,0},
{1,0,0,0, 0,1,0,0, 1,0,0,0, 0,0,0,1},
{0,0,0,1, 0,0,1,0, 0,0,1,0, 0,0,0,0},
{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,1,0,0},
},{
// 2 SHADOW
{0,0,0,0, 0,1,0,0, 0,0,0,0, 0,0,1,0},
{0,1,0,1, 0,0,0,1, 0,1,0,0, 1,0,0,0},
{1,0,0,0, 1,0,0,0, 0,0,1,0, 0,0,0,1},
{0,0,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
},{
// 3 DRIFT
{0,0,0,1, 0,0,0,0, 0,0,1,0, 0,0,0,0},
{1,0,0,0, 0,0,1,0, 0,1,0,0, 0,0,0,0},
{0,1,0,0, 1,0,0,1, 0,0,0,0, 1,0,1,0},
{0,0,0,0, 0,0,0,0, 0,1,0,0, 0,0,0,0},
},{
// 4 BREATH
{0,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,0,1},
{0,1,0,0, 0,0,0,1, 0,1,0,0, 0,0,1,0},
{1,0,1,0, 0,0,0,0, 1,0,0,1, 0,0,0,0},
{0,0,0,0, 0,0,0,0, 0,0,0,1, 0,0,0,0},
},{
// 5 FADE
{0,0,0,0, 0,0,0,1, 0,0,0,0, 1,0,0,0},
{1,0,1,0, 0,0,0,0, 0,1,0,1, 0,0,0,0},
{0,0,0,0, 1,0,1,0, 0,0,0,0, 0,1,0,0},
{0,0,0,0, 0,1,0,0, 0,0,0,0, 0,0,0,0},
},{
// 6 ECHO
{0,0,1,0, 0,0,0,0, 0,1,0,0, 0,0,0,0},
{0,0,0,0, 1,0,0,1, 0,0,0,0, 1,0,0,0},
{1,0,0,0, 0,0,1,0, 0,0,0,1, 0,1,0,0},
{0,0,0,1, 0,0,0,0, 0,0,0,0, 0,0,0,1},
},{
// 7 PULSE
{0,1,0,0, 0,0,0,0, 0,0,0,1, 0,0,0,0},
{1,0,0,1, 0,0,0,0, 1,0,0,0, 0,0,1,0},
{0,0,0,0, 0,1,0,1, 0,0,0,0, 1,0,0,1},
{0,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,0,0},
},{
// 8 MINIMAL
{0,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0},
{0,0,1,0, 0,0,0,0, 0,0,0,0, 0,0,1,0},
{0,0,0,0, 0,1,0,0, 0,0,0,0, 0,0,0,1},
{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},
},{
// 9 SWIRL
{0,0,0,1, 0,0,1,0, 0,1,0,0, 1,0,0,0},
{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1},
{0,1,0,0, 1,0,0,1, 0,0,0,0, 0,1,0,0},
{0,1,0,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
},{
// 10 LINGER
{0,0,0,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
{0,1,0,0, 0,0,0,0, 0,0,0,1, 0,0,0,0},
{0,0,0,1, 0,0,0,0, 0,0,0,0, 0,1,0,0},
{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1},
},{
// 11 DENSE
{0,1,0,1, 0,0,1,0, 1,0,0,1, 0,0,0,0},
{1,0,0,0, 1,0,0,1, 0,1,0,0, 1,0,1,0},
{0,0,1,0, 0,1,0,0, 0,0,1,0, 0,1,0,1},
{0,0,1,0, 0,1,0,0, 0,0,1,0, 0,1,0,0},
}},

// ================================================================
// GENRE 7: PULSE
// ================================================================
{{
// 0 GRID
{1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0},
{0,1,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,1},
{0,0,1,0, 0,0,0,1, 0,0,1,0, 0,0,0,1},
},{
// 1 MOTOR
{1,0,0,0, 1,0,1,0, 1,0,0,0, 1,0,1,0},
{0,0,0,0, 1,0,0,0, 0,0,0,1, 1,0,0,0},
{0,1,1,0, 0,1,1,0, 0,1,1,0, 0,1,1,0},
{0,0,0,1, 0,0,1,0, 0,0,0,1, 0,1,0,0},
},{
// 2 DRIVE
{1,0,0,1, 1,0,0,0, 1,0,0,1, 1,0,0,0},
{0,0,0,0, 1,0,0,0, 0,1,0,0, 1,0,0,0},
{0,1,0,1, 0,1,1,0, 0,1,0,1, 0,1,1,0},
{0,0,1,0, 0,0,0,1, 0,0,1,0, 0,1,0,1},
},{
// 3 CHARGE
{1,0,0,0, 0,1,0,0, 1,0,0,0, 0,0,1,0},
{0,0,0,1, 1,0,0,0, 0,0,1,0, 1,0,0,0},
{0,1,1,0, 0,1,0,1, 0,1,1,0, 0,1,0,1},
{0,0,0,1, 0,1,0,0, 0,0,0,1, 0,0,1,0},
},{
// 4 TENSION
{1,0,0,0, 1,0,0,1, 1,0,0,0, 0,0,1,0},
{0,0,0,0, 1,0,0,0, 0,0,1,0, 1,0,0,0},
{0,0,1,1, 0,1,1,0, 0,0,1,1, 0,1,1,0},
{0,1,0,0, 0,0,1,0, 0,1,0,1, 0,0,1,0},
},{
// 5 SKIP
{1,0,0,0, 0,0,1,0, 1,0,0,0, 0,1,0,0},
{0,0,1,0, 1,0,0,0, 0,0,0,1, 1,0,0,0},
{0,1,0,1, 0,0,1,0, 0,1,0,1, 0,0,1,0},
{0,0,0,1, 0,1,0,0, 0,0,1,0, 0,1,0,1},
},{
// 6 RISER
{1,0,0,0, 1,0,0,0, 1,0,1,0, 1,0,1,1},
{0,0,0,0, 1,0,0,0, 0,0,1,0, 1,0,0,1},
{0,0,1,0, 0,1,1,0, 0,1,1,1, 0,1,1,1},
{0,0,0,0, 0,0,1,0, 0,1,0,1, 1,0,1,1},
},{
// 7 BREAKER
{1,0,1,0, 0,1,0,0, 1,0,0,1, 0,0,1,0},
{0,0,0,1, 1,0,0,0, 0,1,0,0, 1,0,0,1},
{0,1,0,0, 1,0,1,0, 0,1,0,1, 0,0,1,0},
{0,0,1,0, 0,1,0,1, 0,0,1,0, 1,0,0,1},
},{
// 8 MINIMAL
{1,0,0,0, 0,1,0,0, 1,0,0,0, 0,0,1,0},
{0,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,1,0},
{0,0,1,0, 0,0,0,0, 0,0,1,0, 0,0,0,0},
{0,0,0,1, 0,0,0,0, 0,0,0,1, 0,0,0,0},
},{
// 9 SYNC
{1,0,0,1, 0,1,0,0, 1,0,0,1, 0,1,0,0},
{0,1,0,0, 1,0,0,1, 0,1,0,0, 1,0,0,1},
{0,0,1,0, 0,1,0,0, 0,0,1,0, 0,1,0,0},
{0,1,0,0, 1,0,1,0, 0,1,0,0, 1,0,0,1},
},{
// 10 SURGE
{1,1,0,0, 1,0,1,0, 1,1,0,0, 1,0,1,1},
{0,0,1,0, 1,0,0,1, 0,0,1,0, 1,0,0,1},
{0,1,1,0, 0,1,1,1, 0,1,1,0, 0,1,1,1},
{0,0,1,1, 0,1,0,1, 0,0,1,1, 0,1,0,1},
},{
// 11 MAX
{1,1,0,1, 1,1,0,1, 1,1,0,1, 1,1,1,1},
{0,0,1,0, 1,0,0,1, 0,0,1,0, 1,0,0,1},
{1,1,1,1, 0,1,1,1, 1,1,1,1, 0,1,1,1},
{0,1,1,0, 1,1,0,1, 0,1,1,0, 1,1,0,1},
}},

}; // einde PATTERNS

static const char* GENRE_NAMES[8] = {
    "Techno","House","Pulse","Broken",
    "Machine","Collapse","Ghost","Drive"
};

static constexpr int GENRE_PATTERN_INDEX[8] = {
    0, // Techno
    1, // House
    7, // Pulse
    2, // Broken
    4, // Machine
    5, // Collapse
    6, // Ghost
    3  // Drive
};

static const char* MORPH_NAMES[8][12] = {
    { "Floor","Floor+","Break1","Break2","Minimal","Half","Shuffle","Synco1","Synco2","Cluster","Sparse","Berlin" },
    { "Floor","Deep","Classic","Garage","Jack","Latin","Acid","Disco","Minimal","Swing","Peak","After" },
    { "Grid","Motor","Drive","Charge","Tension","Skip","Riser","Breaker","Minimal","Sync","Surge","Max" },
    { "Basic","Synco","Offbeat","D&B","Jungle","IDM","Glitch","Half","Tech","Minimal","Liquid","Chaos" },
    { "4/4","8th","16th","Half","Motorik","Krautrock","Grid","Pulse","Step","Clock","Sync","Complex" },
    { "Begin","Crack","Stutter","Break","Erupt","Silence","Fragment","Rebuild","Wave","Glitch","Storm","End" },
    { "Whisper","Trace","Shadow","Drift","Breath","Fade","Echo","Pulse","Minimal","Swirl","Linger","Dense" },
    { "Basic","Stomp","March","Pulse","Grind","Drill","Noise","Burst","Metal","Minimal","Techno","Wall" },
};

// ============================================================
//  PARAM QUANTITIES
// ============================================================
struct GenreQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        int idx = (int)clamp(getValue(), 0.f, 7.f);
        return GENRE_NAMES[idx];
    }
};

struct MorphQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        if (!module) return "---";
        int genre = (int)clamp(module->getParam(4).getValue(), 0.f, 7.f); // GENRE_PARAM = 4
        int morph  = (int)clamp(getValue(), 0.f, 11.f);
        return MORPH_NAMES[genre][morph];
    }
};

// ============================================================
//  MODULE
// ============================================================

struct React : Module {

    enum ParamId {
        PATTERN_PARAM,
        SNARE_PARAM,
        HIHAT_PARAM,
        PERC_PARAM,
        GENRE_PARAM,
        DROP_PARAM,
        ALT_PARAM,
        PARAMS_LEN
    };

    enum InputId {
        CLK_INPUT,
        RESET_INPUT,
        MORPH_INPUT,
        DROP_INPUT,
        ALT_INPUT,
        INPUTS_LEN
    };

    enum OutputId {
        OUT_A, OUT_B, OUT_C, OUT_D,
        OUTPUTS_LEN
    };

    enum LightId {
        LIGHT_A, LIGHT_B, LIGHT_C, LIGHT_D,
        LIGHT_SNARE_G, LIGHT_SNARE_R,
        LIGHT_HIHAT_G, LIGHT_HIHAT_R,
        LIGHT_PERC_G,  LIGHT_PERC_R,
        LIGHT_DROP,
        LIGHT_ALT_G, LIGHT_ALT_R,
        LIGHTS_LEN
    };

    int snareMorph = 0;
    int hihatMorph = 0;
    int percMorph  = 0;

    // DROP state
    bool dropActive    = false;
    int  dropBarCount  = 0;
    int  dropBarLen    = 16; // 1 bar = 16 stappen
    float prevDrop     = 0.f;

    // ALT state
    bool altActive     = false;
    bool altPending    = false;  // wacht op bar begin
    float prevAlt      = 0.f;

    // Morph RNG
    uint32_t rngState = 12345;
    float fastRand() {
        rngState ^= rngState << 13;
        rngState ^= rngState >> 17;
        rngState ^= rngState << 5;
        return (rngState & 0x7FFFFFFF) / float(0x7FFFFFFF);
    }

    // Clock
    float prevClk   = 0.f;
    float prevReset = 0.f;
    float clockPeriod = 44100.f;
    float clockTimer  = 0.f;
    bool  clockRunning = false;
	bool  quarterNoteClock = true;
	bool  quarterPeriodValid = false;
	bool  quarterResetPending = true;
    float subPhase  = 0.f;
    int   subCount  = 0;

    // Gates
    float gateA = 0.f, gateB = 0.f, gateC = 0.f, gateD = 0.f;
    static constexpr float GATE_LEN = 0.003f;

    // LEDs
    float ledA = 0.f, ledB = 0.f, ledC = 0.f, ledD = 0.f;

    // Sample rate
    float sampleRate = 44100.f;

    // Voor display
    float displayBpm = 0.f;

    React() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam<MorphQuantity>(PATTERN_PARAM, 0.f, 11.f, 0.f, "Pattern");
        configParam(SNARE_PARAM,   0.f,  7.f, 0.f,  "Snare");
        configParam(HIHAT_PARAM,   0.f,  7.f, 0.f,  "Hihat");
        configParam(PERC_PARAM,    0.f,  7.f, 0.f,  "Perc");
        configParam<GenreQuantity>(GENRE_PARAM,  0.f,  7.f, 0.f, "Genre");
        configParam(DROP_PARAM, 0.f, 1.f, 0.f, "Drop");
        configParam(ALT_PARAM,  0.f, 1.f, 0.f, "Variation");

        paramQuantities[PATTERN_PARAM]->snapEnabled = true;
        paramQuantities[GENRE_PARAM]->snapEnabled  = true;
        paramQuantities[SNARE_PARAM]->snapEnabled  = true;
        paramQuantities[HIHAT_PARAM]->snapEnabled  = true;
        paramQuantities[PERC_PARAM]->snapEnabled   = true;

        configInput(CLK_INPUT,   "Clock");
        configInput(RESET_INPUT, "Reset");
        configInput(MORPH_INPUT, "Morph");
        configInput(DROP_INPUT,  "Drop");
        configInput(ALT_INPUT,   "Variation");
        configOutput(OUT_A, "Kick");
        configOutput(OUT_B, "Snare");
        configOutput(OUT_C, "Hihat");
        configOutput(OUT_D, "Perc");
    }

    void onSampleRateChange(const SampleRateChangeEvent& e) override {
        sampleRate = e.sampleRate;
    }

    // Menu helpers
    bool isSnareLockedGenre() { return (int)params[SNARE_PARAM].getValue() > 0; }
    bool isHihatLockedGenre() { return (int)params[HIHAT_PARAM].getValue() > 0; }
    bool isPercLockedGenre()  { return (int)params[PERC_PARAM].getValue()  > 0; }

    int getSnareGenre() { return (int)clamp(params[SNARE_PARAM].getValue(), 0.f, 7.f); }
    int getHihatGenre() { return (int)clamp(params[HIHAT_PARAM].getValue(), 0.f, 7.f); }
    int getPercGenre()  { return (int)clamp(params[PERC_PARAM].getValue(),  0.f, 7.f); }

    void process(const ProcessArgs& args) override {

        // --- Klok ---
        float clkIn = inputs[CLK_INPUT].getVoltage();
        bool clkRise = (clkIn >= 2.f && prevClk < 2.f);
        prevClk = clkIn;
		const bool clockWasRunning = clockRunning;

		// In 1-PPQN mode Reset prepares step 0 for the next quarter-note edge.
		// Keeping the reset pending prevents a free-running subdivision from
		// starting a new bar between two external clock pulses.
		float resetIn = inputs[RESET_INPUT].getVoltage();
		bool resetRise = (resetIn >= 2.f && prevReset < 2.f);
		prevReset = resetIn;
		if (resetRise && quarterNoteClock) {
			subPhase = 0.f;
			subCount = 0;
			quarterPeriodValid = false;
			quarterResetPending = true;
		}

		bool quarterEdgeTick = false;

        if (clkRise) {
            if (clockRunning && clockTimer > 0.f) {
				if (quarterNoteClock) {
					// The measured quarter note predicts the three internal sixteenths.
					// Do not smooth this value: every external edge hard-reanchors phase,
					// so tempo changes cannot leave React permanently beside the grid.
					clockPeriod = std::max(1.f, clockTimer * 0.25f);
					quarterPeriodValid = true;
				}
				else {
					// Preserve the original 4-PPQN timing for legacy patches.
					clockPeriod = clockPeriod * 0.5f + clockTimer * 0.5f;
				}
                // BPM = kwartsnoten per minuut
                // clockPeriod = samples per 16th noot
                // 1 kwartsnoot = 4 x 16th
                displayBpm = (sampleRate * 60.f) / (clockPeriod * 4.f);
            }
            clockTimer   = 0.f;
            clockRunning = true;

			if (quarterNoteClock) {
				// Quarter-note pattern steps are emitted on the external edge itself.
				// The first/reset edge is step 0; later edges are 4, 8, 12, 0.
				if (!clockWasRunning || quarterResetPending)
					subCount = 0;
				else
					subCount = (((subCount / 4) * 4) + 4) % 16;
				subPhase = 0.f;
				quarterResetPending = false;
				quarterEdgeTick = true;
			}
        }
        clockTimer += 1.f;

		// Keep simultaneous Reset/Clock behaviour unchanged in legacy mode.
		if (resetRise && !quarterNoteClock) {
            subPhase = 0.f;
            subCount = 0;
        }

        if (!clockRunning) {
            outputs[OUT_A].setVoltage(0.f);
            outputs[OUT_B].setVoltage(0.f);
            outputs[OUT_C].setVoltage(0.f);
            outputs[OUT_D].setVoltage(0.f);
            return;
        }

        // --- Knoppen ---
        int snareKnob = (int)clamp(params[SNARE_PARAM].getValue(), 0.f, 7.f);
        int hihatKnob = (int)clamp(params[HIHAT_PARAM].getValue(), 0.f, 7.f);
        int percKnob  = (int)clamp(params[PERC_PARAM].getValue(),  0.f, 7.f);

        int genre = (int)clamp(params[GENRE_PARAM].getValue(), 0.f, 7.f);
        int pattern = (int)clamp(params[PATTERN_PARAM].getValue(), 0.f, 11.f);
        int patternGenre = GENRE_PATTERN_INDEX[genre];

        // --- DROP logica ---
        float dropBtn = params[DROP_PARAM].getValue();
        float dropCv  = inputs[DROP_INPUT].getVoltage();
        dropActive = (dropBtn > 0.5f || dropCv >= 2.f);

        // --- VARIATION logica ---
        float altBtn = params[ALT_PARAM].getValue();
        float altCv  = inputs[ALT_INPUT].getVoltage();
        bool altCvRise = (altCv >= 2.f && prevAlt <= 0.5f);
        prevAlt = (altCv >= 2.f) ? 1.f : 0.f;
        if (altCvRise) {
            altBtn = (altBtn > 0.5f) ? 0.f : 1.f;
            params[ALT_PARAM].setValue(altBtn);
        }
        altPending = (altBtn > 0.5f);
        int altPattern = (pattern + 6) % 12;
        int activePattern = altActive ? altPattern : pattern;

        // MORPH CV — 0-10V crossfade naar volgend patroon
        float morphCv = clamp(inputs[MORPH_INPUT].getVoltage() / 10.f, 0.f, 1.f);
        int patternNext = (pattern + 1) % 12;

        // 0 = FREE (volgt genre+morph), 1-7 = LOCK op ander genre
        int snareGenre = GENRE_PATTERN_INDEX[(snareKnob == 0) ? genre : (snareKnob % 8)];
        int hihatGenre = GENRE_PATTERN_INDEX[(hihatKnob == 0) ? genre : (hihatKnob % 8)];
        int percGenre  = GENRE_PATTERN_INDEX[(percKnob  == 0) ? genre : (percKnob  % 8)];

        if (snareKnob == 0) snareMorph = pattern;
        if (hihatKnob == 0) hihatMorph = pattern;
        if (percKnob  == 0) percMorph  = pattern;

        // --- Subdivisions ---
		bool subTick = quarterEdgeTick;
		if (quarterNoteClock) {
			// Only steps 1-3 inside a quarter note are generated internally.
			// Its boundary step is reserved for the next real clock edge, so even
			// clock jitter can never produce an early downbeat.
			if (!clkRise && quarterPeriodValid && (subCount % 4) < 3) {
				subPhase += 1.f / clockPeriod;
				if (subPhase >= 1.f) {
					subPhase -= 1.f;
					subCount = (subCount + 1) % 16;
					subTick = true;
				}
			}
		}
		else {
			// Original 4-PPQN scheduler, retained for patch compatibility.
			subPhase += 1.f / clockPeriod;
			if (subPhase >= 1.f) {
				subPhase -= 1.f;
				subCount = (subCount + 1) % 16;
				subTick = true;
			}
		}

        // ALT wissel op bar begin
        if (subTick && subCount == 0 && altPending != altActive) {
            altActive = altPending;
        }

        bool doA = false, doB = false, doC = false, doD = false;

        if (subTick) {
            // Basis patroon — ALT wisselaar
            bool baseA = dropActive ? false : PATTERNS[patternGenre][activePattern].a[subCount];
            bool baseB = PATTERNS[snareGenre][snareMorph].b[subCount];
            bool baseC = PATTERNS[hihatGenre][hihatMorph].c[subCount];
            bool baseD = PATTERNS[percGenre][percMorph].d[subCount];

            // Morph CV — naar volgend patroon
            if (morphCv > 0.001f) {
                bool nextB = PATTERNS[snareGenre][patternNext].b[subCount];
                bool nextC = PATTERNS[hihatGenre][patternNext].c[subCount];
                bool nextD = PATTERNS[percGenre][patternNext].d[subCount];

                // Kick blijft de vaste ankerlaag; MORPH beweegt alleen de overige parts.
                doA = baseA;

                // Snare/Hihat/Perc alleen als Free
                if (snareKnob == 0) {
                    if (baseB != nextB) doB = (fastRand() < morphCv) ? nextB : baseB;
                    else doB = baseB;
                } else {
                    doB = baseB;
                }

                if (hihatKnob == 0) {
                    if (baseC != nextC) doC = (fastRand() < morphCv) ? nextC : baseC;
                    else doC = baseC;
                } else {
                    doC = baseC;
                }

                if (percKnob == 0) {
                    if (baseD != nextD) doD = (fastRand() < morphCv) ? nextD : baseD;
                    else doD = baseD;
                } else {
                    doD = baseD;
                }
            } else {
                doA = baseA;
                doB = baseB;
                doC = baseC;
                doD = baseD;
            }
        }

        // --- Gates ---
        float gateLenSamples = GATE_LEN * sampleRate;
        if (doA) gateA = gateLenSamples;
        if (doB) gateB = gateLenSamples;
        if (doC) gateC = gateLenSamples;
        if (doD) gateD = gateLenSamples;

        if (gateA > 0.f) { gateA -= 1.f; outputs[OUT_A].setVoltage(10.f); }
        else               outputs[OUT_A].setVoltage(0.f);
        if (gateB > 0.f) { gateB -= 1.f; outputs[OUT_B].setVoltage(10.f); }
        else               outputs[OUT_B].setVoltage(0.f);
        if (gateC > 0.f) { gateC -= 1.f; outputs[OUT_C].setVoltage(10.f); }
        else               outputs[OUT_C].setVoltage(0.f);
        if (gateD > 0.f) { gateD -= 1.f; outputs[OUT_D].setVoltage(10.f); }
        else               outputs[OUT_D].setVoltage(0.f);

        // --- LEDs ---
        float ledDecay = 1.f - (12.f / sampleRate);
        if (doA) ledA = 1.f; else ledA *= ledDecay;
        if (doB) ledB = 1.f; else ledB *= ledDecay;
        if (doC) ledC = 1.f; else ledC *= ledDecay;
        if (doD) ledD = 1.f; else ledD *= ledDecay;

        lights[LIGHT_A].setBrightness(ledA);
        lights[LIGHT_B].setBrightness(ledB);
        lights[LIGHT_C].setBrightness(ledC);
        lights[LIGHT_D].setBrightness(ledD);

        // DROP LED
        lights[LIGHT_DROP].setBrightness(dropActive ? 1.f : 0.f);

        // ALT LED — groen = hoofd, rood = alt
        // LED toont pending staat — reageert direct op knop
        lights[LIGHT_ALT_G].setBrightness(altPending ? 0.f : 1.f);
        lights[LIGHT_ALT_R].setBrightness(altPending ? 1.f : 0.f);

        // Free/Lock LEDs
        lights[LIGHT_SNARE_G].setBrightness(snareKnob == 0 ? 1.f : 0.f);
        lights[LIGHT_SNARE_R].setBrightness(snareKnob != 0 ? 1.f : 0.f);
        lights[LIGHT_HIHAT_G].setBrightness(hihatKnob == 0 ? 1.f : 0.f);
        lights[LIGHT_HIHAT_R].setBrightness(hihatKnob != 0 ? 1.f : 0.f);
        lights[LIGHT_PERC_G].setBrightness(percKnob  == 0 ? 1.f : 0.f);
        lights[LIGHT_PERC_R].setBrightness(percKnob  != 0 ? 1.f : 0.f);
    }

    // Menu
    json_t* dataToJson() override {
        json_t* root = json_object();
		json_object_set_new(root, "quarterNoteClock", json_boolean(quarterNoteClock));
        return root;
    }

	void dataFromJson(json_t* root) override {
		json_t* clockJson = json_object_get(root, "quarterNoteClock");
		// Oude patches gebruikten één puls per zestiende (4 PPQN).
		quarterNoteClock = clockJson ? json_boolean_value(clockJson) : false;
	}
};

// ============================================================
//  LCD DISPLAY WIDGET
// ============================================================

struct ReactDisplay : Widget {
    React* module = nullptr;

    ReactDisplay() {
        box.size = Vec(151.382f, 46.256f);
    }

    void draw(const DrawArgs& args) override {
        NVGcolor white = nvgRGB(255, 255, 255);

        nvgFontFaceId(args.vg, APP->window->uiFont->handle);
        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

        if (!module) {
            nvgFillColor(args.vg, white);
            nvgFontSize(args.vg, 13.f);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, "Patch clock first", NULL);
            return;
        }

        if (!module->inputs[React::CLK_INPUT].isConnected()) {
            nvgFillColor(args.vg, white);
            nvgFontSize(args.vg, 13.f);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgText(args.vg, box.size.x / 2.f, box.size.y / 2.f, "Patch clock first", NULL);
            return;
        }

        // Module is beschikbaar
        int genre   = (int)clamp(module->params[React::GENRE_PARAM].getValue(),   0.f, 7.f);
        int pattern = (int)clamp(module->params[React::PATTERN_PARAM].getValue(), 0.f, 11.f);
        int snareK  = (int)clamp(module->params[React::SNARE_PARAM].getValue(),   0.f, 7.f);
        int hihatK  = (int)clamp(module->params[React::HIHAT_PARAM].getValue(),   0.f, 7.f);
        int percK   = (int)clamp(module->params[React::PERC_PARAM].getValue(),    0.f, 7.f);

        nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(args.vg, white);

        if (module->clockRunning) {
            char line1[64];
            snprintf(line1, sizeof(line1), "%s -> %s", GENRE_NAMES[genre], MORPH_NAMES[genre][pattern]);
            nvgFontSize(args.vg, 12.5f);
            nvgText(args.vg, 8.f, 3.f, line1, NULL);

            char line2[32];
            snprintf(line2, sizeof(line2), "%3.0f BPM", module->displayBpm);
            nvgFontSize(args.vg, 11.5f);
            nvgText(args.vg, 8.f, 18.f, line2, NULL);
        } else {
            nvgFontSize(args.vg, 12.5f);
            nvgText(args.vg, 8.f, 3.f, "--- -> ---", NULL);
            nvgFontSize(args.vg, 11.5f);
            nvgText(args.vg, 8.f, 18.f, "--- BPM", NULL);
        }

        // Regel 3: KIT/Lock status
        nvgFontSize(args.vg, 9.5f);
        nvgFillColor(args.vg, white);
        char line3[64];
        snprintf(line3, sizeof(line3), "SN: %s   HI: %s   PE: %s",
            snareK == 0 ? "kit" : "lock",
            hihatK == 0 ? "kit" : "lock",
            percK  == 0 ? "kit" : "lock");
        nvgText(args.vg, 8.f, 33.f, line3, NULL);
    }
};

// ============================================================
//  WIDGET
// ============================================================

struct ReactWidget : SubmitModuleWidget {
    ReactWidget(React* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/React.svg")));

        // LCD Display
        ReactDisplay* display = createWidget<ReactDisplay>(Vec(21.288f, 40.975f));
        display->module = module;
        addChild(display);

        // Main controls
        addParam(createParamCentered<ReactImpactSmallKnob>(Vec(34.694f, 130.361f), module, React::GENRE_PARAM));
        addParam(createParamCentered<ReactImpactPatternKnob>(Vec(105.38f, 141.841f), module, React::PATTERN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(164.623f, 120.109f), module, React::MORPH_INPUT));

        // Free/Lock LEDs
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(17.537f, 177.748f), module, React::LIGHT_SNARE_G));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(17.537f, 177.748f), module, React::LIGHT_SNARE_R));
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(82.270f, 177.748f), module, React::LIGHT_HIHAT_G));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(82.270f, 177.748f), module, React::LIGHT_HIHAT_R));
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(145.135f, 177.748f), module, React::LIGHT_PERC_G));
        addChild(createLightCentered<SmallLight<RedLight>>(Vec(145.135f, 177.748f), module, React::LIGHT_PERC_R));

        // Part lock controls
        addParam(createParamCentered<ReactImpactSmallKnob>(Vec(34.694f, 207.062f), module, React::SNARE_PARAM));
        addParam(createParamCentered<ReactImpactSmallKnob>(Vec(96.884f, 207.062f), module, React::HIHAT_PARAM));
        addParam(createParamCentered<ReactImpactSmallKnob>(Vec(159.13f, 207.062f), module, React::PERC_PARAM));

        // Drop / Variation
        addParam(createParamCentered<ReactDropButton>(Vec(28.840f, 265.883f), module, React::DROP_PARAM));
        addParam(createParamCentered<ReactVariationButton>(Vec(74.221f, 265.883f), module, React::ALT_PARAM));

        // Inputs
        addInput(createInputCentered<PJ301MPort>(Vec(29.324f, 295.692f), module, React::DROP_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(74.424f, 295.692f), module, React::ALT_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(119.524f, 295.692f), module, React::CLK_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(164.623f, 295.692f), module, React::RESET_INPUT));

        // Outputs + LEDs
        const float outXs[4] = { 29.324f, 74.424f, 119.524f, 164.623f };
        const float ledXs[4] = { 17.287f, 58.919f, 105.112f, 151.306f };
        for (int i = 0; i < 4; i++) {
            addOutput(createOutputCentered<PJ301MPort>(Vec(outXs[i], 342.966f), module,
                      React::OutputId(React::OUT_A + i)));
            addChild(createLightCentered<SmallLight<YellowLight>>(Vec(ledXs[i], 324.240f), module,
                     React::LightId(React::LIGHT_A + i)));
        }
    }

    void appendContextMenu(Menu* menu) override {
        React* module = dynamic_cast<React*>(this->module);
        if (!module) return;

        menu->addChild(new MenuSeparator);
		menu->addChild(createMenuLabel("Clock input"));
		menu->addChild(createCheckMenuItem("1 PPQN (quarter note)", "", [=]() {
			return module->quarterNoteClock;
		}, [=]() {
			module->quarterNoteClock = true;
			module->clockRunning = false;
			module->clockTimer = 0.f;
			module->subPhase = 0.f;
			module->subCount = 0;
			module->quarterPeriodValid = false;
			module->quarterResetPending = true;
		}));
		menu->addChild(createCheckMenuItem("4 PPQN (legacy)", "", [=]() {
			return !module->quarterNoteClock;
		}, [=]() {
			module->quarterNoteClock = false;
			module->clockRunning = false;
			module->clockTimer = 0.f;
			module->subPhase = 0.f;
			module->subCount = 0;
			module->quarterPeriodValid = false;
			module->quarterResetPending = true;
		}));

		menu->addChild(new MenuSeparator);

        // Snare status
        int snareKnob = (int)clamp(module->params[React::SNARE_PARAM].getValue(), 0.f, 7.f);
        std::string snareLabel = "Snare: ";
        snareLabel += (snareKnob == 0) ? "KIT" : std::string("Lock (") + GENRE_NAMES[snareKnob % 8] + ")";
        menu->addChild(createMenuLabel(snareLabel));

        // Hihat status
        int hihatKnob = (int)clamp(module->params[React::HIHAT_PARAM].getValue(), 0.f, 7.f);
        std::string hihatLabel = "Hihat: ";
        hihatLabel += (hihatKnob == 0) ? "KIT" : std::string("Lock (") + GENRE_NAMES[hihatKnob % 8] + ")";
        menu->addChild(createMenuLabel(hihatLabel));

        // Perc status
        int percKnob = (int)clamp(module->params[React::PERC_PARAM].getValue(), 0.f, 7.f);
        std::string percLabel = "Perc: ";
        percLabel += (percKnob == 0) ? "KIT" : std::string("Lock (") + GENRE_NAMES[percKnob % 8] + ")";
        menu->addChild(createMenuLabel(percLabel));

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Manual", "", []() {
            system::openBrowser("https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/react/");
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

Model* modelReact = createModel<React, ReactWidget>("React");
