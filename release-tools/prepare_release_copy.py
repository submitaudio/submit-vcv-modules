#!/usr/bin/env python3
"""Create a buildable Submit release copy without touching the local beta setup.

This script deliberately performs no Git actions and never changes the source tree.
The destination must not exist yet.
"""

import argparse
import json
from pathlib import Path
import re
import shutil
import sys


PUBLIC_SLUGS = [
    "Drift",
    "Chrono",
    "Impact",
    "Chain",
    "SumM4",
    "SumS4",
    "Tag",
    "Set",
    "Pulse",
    "Squeeze",
    "Shape",
    "Master",
    "Gain",
    "Sweep",
    "Loop",
    "Clang",
    "React",
    "Sync",
    "Flip",
    "Orbit",
    "Circles",
    "Sub",
]

CREATE_MODEL_RE = re.compile(
    r'Model\*\s+(model\w+)\s*=\s*createModel<[^;]+>\("([^"]+)"\)\s*;'
)
PANEL_RE = re.compile(r'"(res/[A-Za-z0-9_.-]+\.svg)"')
ADD_MODEL_RE = re.compile(r"(?m)^\s*p->addModel\((model\w+)\);\s*$")
EXTERN_MODEL_RE = re.compile(r"(?m)^extern Model\* (model\w+);\s*$")


def ignored(_directory: str, names: list[str]) -> set[str]:
    ignored_names = {
        ".git",
        "build",
        "dist",
        "plugin.dylib",
        "backups",
        "metamodule",
        "samples",
        "DONATIONS.md",
        "__pycache__",
    }
    if Path(_directory).name == "docs":
        ignored_names.add("CURRENT-DEVELOPMENT-HANDOFF.md")
        ignored_names.add("SUBMIT-LAB-BETA.md")
    for name in names:
        if (
            name.endswith((".bak", ".bak2"))
            or ".backup_" in name
            or "_backup_" in name
        ):
            ignored_names.add(name)
    return ignored_names


def discover_models(src_dir: Path) -> dict[str, tuple[str, Path]]:
    models: dict[str, tuple[str, Path]] = {}
    for cpp_path in sorted(src_dir.glob("*.cpp")):
        for match in CREATE_MODEL_RE.finditer(cpp_path.read_text()):
            model, slug = match.groups()
            if slug in models:
                raise RuntimeError(f"Duplicate createModel slug: {slug}")
            models[slug] = (model, cpp_path)
    return models


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()

    source = Path(__file__).resolve().parents[1]
    destination = args.destination.expanduser().resolve()
    if destination.exists():
        print(f"Refusing to overwrite existing destination: {destination}", file=sys.stderr)
        return 2

    shutil.copytree(source, destination, ignore=ignored)

    plugin_path = destination / "plugin.json"
    plugin_data = json.loads(plugin_path.read_text())
    modules = {module["slug"]: module for module in plugin_data["modules"]}
    missing = [slug for slug in PUBLIC_SLUGS if slug not in modules]
    if missing:
        raise RuntimeError(f"Missing public plugin.json modules: {missing}")
    plugin_data["modules"] = [modules[slug] for slug in PUBLIC_SLUGS]
    plugin_path.write_text(json.dumps(plugin_data, indent=2) + "\n")

    models = discover_models(destination / "src")
    missing_sources = [slug for slug in PUBLIC_SLUGS if slug not in models]
    if missing_sources:
        raise RuntimeError(f"Missing public createModel sources: {missing_sources}")

    public_models = {models[slug][0] for slug in PUBLIC_SLUGS}
    public_asset_refs: set[str] = set()
    beta_asset_refs: set[str] = set()

    source_slugs: dict[Path, set[str]] = {}
    for slug, (_model, cpp_path) in models.items():
        source_slugs.setdefault(cpp_path, set()).add(slug)

    for cpp_path, slugs in source_slugs.items():
        refs = set(PANEL_RE.findall(cpp_path.read_text()))
        if slugs.intersection(PUBLIC_SLUGS):
            public_asset_refs |= refs
        else:
            beta_asset_refs |= refs
            cpp_path.unlink()

    for asset_ref in sorted(beta_asset_refs - public_asset_refs):
        asset_path = destination / asset_ref
        if asset_path.exists():
            asset_path.unlink()

    private_orphaned_paths = [
        "res/CirclesLcdDesign.svg",
        "res/MachinaLfoNoise.svg",
        "res/MachinaLfoRandom.svg",
        "res/MachinaLfoReverseSaw.svg",
        "res/MachinaLfoSaw.svg",
        "res/MachinaLfoSine.svg",
        "res/MachinaLfoSquare.svg",
        "res/MachinaLfoTriangle.svg",
        "res/MachinaLfoWander.svg",
        "src/GranularEngine.hpp",
    ]
    for private_path in private_orphaned_paths:
        path = destination / private_path
        if path.exists():
            path.unlink()

    plugin_cpp_path = destination / "src/plugin.cpp"
    plugin_cpp = plugin_cpp_path.read_text()
    plugin_cpp = ADD_MODEL_RE.sub(
        lambda match: match.group(0) if match.group(1) in public_models else "",
        plugin_cpp,
    )
    plugin_cpp = re.sub(r"\n{3,}", "\n\n", plugin_cpp).rstrip() + "\n"
    plugin_cpp_path.write_text(plugin_cpp)

    plugin_hpp_path = destination / "src/plugin.hpp"
    plugin_hpp = plugin_hpp_path.read_text()
    plugin_hpp = EXTERN_MODEL_RE.sub(
        lambda match: match.group(0) if match.group(1) in public_models else "",
        plugin_hpp,
    )
    plugin_hpp = re.sub(r"\n{3,}", "\n\n", plugin_hpp).rstrip() + "\n"
    plugin_hpp_path.write_text(plugin_hpp)

    changelog_dir = destination / "docs/changelogs"
    for changelog in changelog_dir.glob("*.md"):
        if changelog.stem not in PUBLIC_SLUGS:
            changelog.unlink()

    final_models = discover_models(destination / "src")
    if set(final_models) != set(PUBLIC_SLUGS):
        raise RuntimeError(
            "Release source list mismatch: "
            f"expected {PUBLIC_SLUGS}, got {sorted(final_models)}"
        )

    registered = ADD_MODEL_RE.findall(plugin_cpp_path.read_text())
    declared = EXTERN_MODEL_RE.findall(plugin_hpp_path.read_text())
    expected_models = [final_models[slug][0] for slug in PUBLIC_SLUGS]
    if registered != expected_models:
        raise RuntimeError(f"Registration mismatch: {registered}")
    if [model for model in declared if model in public_models] != expected_models:
        raise RuntimeError(f"Declaration mismatch: {declared}")

    missing_assets = [
        asset_ref
        for asset_ref in sorted(public_asset_refs)
        if not (destination / asset_ref).exists()
    ]
    if missing_assets:
        raise RuntimeError(f"Missing public assets: {missing_assets}")

    print(f"Prepared Submit {plugin_data['version']} release copy: {destination}")
    print("Public modules:", ", ".join(PUBLIC_SLUGS))
    print("No Git actions were performed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
