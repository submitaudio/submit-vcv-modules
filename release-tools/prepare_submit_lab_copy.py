#!/usr/bin/env python3
"""Create a minimal buildable Submit Lab beta copy for Circles and SUB.

The active Submit tree remains the only source of truth. This script performs no
Git actions, never changes the source tree, and refuses to overwrite a target.
"""

import argparse
import json
from pathlib import Path
import re
import shutil
import sys


LAB_SLUGS = ["Circles", "Sub"]
LAB_MODELS = ["modelCircles", "modelSub"]
LAB_MODULES = [
    {
        "slug": "Circles",
        "name": "Circles",
        "description": (
            "Always-quantized circular eight-step sequencer with slow sub "
            "and scale-chain outputs."
        ),
        "tags": ["Sequencer"],
    },
    {
        "slug": "Sub",
        "name": "SUB",
        "description": (
            "Monophonic analog bass voice with a dedicated warm sub layer."
        ),
        "tags": ["VCO", "Synth voice"],
    },
]
LAB_ASSETS = [
    "res/Circles.svg",
    "res/CirclesComponents.svg",
    "res/Sub.svg",
    "res/SubmitKnobMedium.svg",
    "res/SubmitKnobPetite.svg",
    "res/SubmitKnobSmall.svg",
    "res/SubmitKnobTiny.svg",
    "res/beta-submit-lab.svg",
]

INFO_URL = (
    "https://community.vcvrack.com/t/"
    "submit-audio-development-updates-for-vcv-rack-and-metamodule/26001"
)
SOURCE_URL = "https://github.com/submitaudio/submit-vcv-modules"
BUG_URL = f"{SOURCE_URL}/issues"
CHANGELOG_URL = f"{SOURCE_URL}/releases"

VERSION_RE = re.compile(r"^2\.\d+\.\d+$")
CREATE_MODEL_RE = re.compile(
    r'Model\*\s+(model\w+)\s*=\s*createModel<[^;]+>\("([^"]+)"\)\s*;'
)
EXTERN_MODEL_RE = re.compile(r"(?m)^extern Model\* (model\w+);\s*$")
ASSET_RE = re.compile(r'"(res/[A-Za-z0-9_.-]+\.svg)"')

MAKEFILE = """# Generated Submit Lab beta build
RACK_DIR ?= ../..

FLAGS += -DSUBMIT_LAB_BUILD
CFLAGS +=
CXXFLAGS +=
LDFLAGS +=

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

include $(RACK_DIR)/plugin.mk
"""

PLUGIN_CPP = """#include \"plugin.hpp\"

Plugin* pluginInstance;

void init(Plugin* p) {
\tpluginInstance = p;
\tp->addModel(modelCircles);
\tp->addModel(modelSub);
}
"""

PLUGIN_HPP = """#pragma once

#define SUBMIT_URL \"https://submitaudio.nl\"
#define SUBMIT_LAB_INFO_URL \"https://community.vcvrack.com/t/submit-audio-development-updates-for-vcv-rack-and-metamodule/26001\"
#define SUBMIT_LAB_BUG_URL \"https://github.com/submitaudio/submit-vcv-modules/issues\"
#define SUBMIT_LAB_CHANGELOG_URL \"https://github.com/submitaudio/submit-vcv-modules/releases\"

#include <rack.hpp>

using namespace rack;

extern Plugin* pluginInstance;

struct SubmitModuleWidget : ModuleWidget {
\tvoid appendContextMenu(Menu* menu) override {
\t\tmenu->addChild(new MenuSeparator);
\t\tmenu->addChild(createMenuLabel(\"Submit Lab Beta\"));
\t\tmenu->addChild(createMenuItem(\"Beta information & feedback\", \"\", []() {
\t\t\tsystem::openBrowser(SUBMIT_LAB_INFO_URL);
\t\t}));
\t\tmenu->addChild(createMenuItem(\"Report a beta bug\", \"\", []() {
\t\t\tsystem::openBrowser(SUBMIT_LAB_BUG_URL);
\t\t}));
\t\tmenu->addChild(createMenuItem(\"Beta changelog\", \"\", []() {
\t\t\tsystem::openBrowser(SUBMIT_LAB_CHANGELOG_URL);
\t\t}));
\t\tmenu->addChild(new MenuSeparator);
\t\tmenu->addChild(createMenuItem(\"Submit Audio website\", \"\", []() {
\t\t\tsystem::openBrowser(SUBMIT_URL);
\t\t}));
\t}
};

extern Model* modelCircles;
extern Model* modelSub;
"""


def copy_file(source_root: Path, destination_root: Path, relative: str) -> None:
    source = source_root / relative
    if not source.is_file():
        raise RuntimeError(f"Missing Submit Lab source dependency: {relative}")
    destination = destination_root / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def make_manifest(version: str) -> dict:
    modules = []
    for module_definition in LAB_MODULES:
        module = dict(module_definition)
        module["manualUrl"] = INFO_URL
        modules.append(module)

    return {
        "slug": "SubmitLab",
        "name": "Submit Lab",
        "version": version,
        "license": "GPL-3.0-only",
        "brand": "Submit Lab",
        "author": "Juan José",
        "authorEmail": "",
        "authorUrl": "",
        "pluginUrl": INFO_URL,
        "manualUrl": INFO_URL,
        "sourceUrl": SOURCE_URL,
        "donateUrl": "https://ko-fi.com/submitaudio",
        "changelogUrl": CHANGELOG_URL,
        "modules": modules,
    }


def validate_copy(destination: Path, manifest: dict) -> None:
    if manifest["slug"] != "SubmitLab":
        raise RuntimeError("Submit Lab plugin slug changed unexpectedly")
    if [module["slug"] for module in manifest["modules"]] != LAB_SLUGS:
        raise RuntimeError("Submit Lab manifest module allowlist mismatch")

    cpp_files = sorted(path.name for path in (destination / "src").glob("*.cpp"))
    if cpp_files != ["Circles.cpp", "Sub.cpp", "plugin.cpp"]:
        raise RuntimeError(f"Unexpected Submit Lab sources: {cpp_files}")

    models = []
    referenced_assets = set()
    for source_name in ["Circles.cpp", "Sub.cpp"]:
        text = (destination / "src" / source_name).read_text()
        models.extend(CREATE_MODEL_RE.findall(text))
        referenced_assets.update(ASSET_RE.findall(text))
    if models != [("modelCircles", "Circles"), ("modelSub", "Sub")]:
        raise RuntimeError(f"Submit Lab createModel mismatch: {models}")

    missing_references = sorted(referenced_assets - set(LAB_ASSETS))
    if missing_references:
        raise RuntimeError(f"Unlisted Submit Lab assets: {missing_references}")
    missing_assets = [path for path in LAB_ASSETS if not (destination / path).is_file()]
    if missing_assets:
        raise RuntimeError(f"Missing copied Submit Lab assets: {missing_assets}")

    plugin_cpp = (destination / "src/plugin.cpp").read_text()
    registered = re.findall(r"p->addModel\((model\w+)\);", plugin_cpp)
    if registered != LAB_MODELS:
        raise RuntimeError(f"Submit Lab registration mismatch: {registered}")

    plugin_hpp = (destination / "src/plugin.hpp").read_text()
    declared = EXTERN_MODEL_RE.findall(plugin_hpp)
    if declared != LAB_MODELS:
        raise RuntimeError(f"Submit Lab declarations mismatch: {declared}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("destination", type=Path)
    parser.add_argument("--version", default="2.0.0")
    args = parser.parse_args()

    if not VERSION_RE.fullmatch(args.version):
        print("Submit Lab version must use VCV Rack 2.x.x format", file=sys.stderr)
        return 2

    source = Path(__file__).resolve().parents[1]
    destination = args.destination.expanduser().resolve()
    if destination.exists():
        print(f"Refusing to overwrite existing destination: {destination}", file=sys.stderr)
        return 2

    destination.mkdir(parents=True)
    for relative in [
        "LICENSE",
        "src/Circles.cpp",
        "src/Sub.cpp",
        *LAB_ASSETS,
    ]:
        copy_file(source, destination, relative)

    manifest = make_manifest(args.version)
    (destination / "plugin.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (destination / "Makefile").write_text(MAKEFILE)
    (destination / "src/plugin.cpp").write_text(PLUGIN_CPP)
    (destination / "src/plugin.hpp").write_text(PLUGIN_HPP)

    validate_copy(destination, manifest)

    print(f"Prepared Submit Lab {args.version}: {destination}")
    print("Modules: Circles, SUB")
    print("Plugin slug: SubmitLab")
    print("No Git actions were performed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
