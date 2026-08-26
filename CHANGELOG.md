# Changelog

## Module changelogs

Detailed, module-specific history is maintained in separate pages:

- [Drift](docs/changelogs/Drift.md)
- [Chrono](docs/changelogs/Chrono.md)
- [Impact](docs/changelogs/Impact.md)
- [Chain](docs/changelogs/Chain.md)
- [Sum M4](docs/changelogs/SumM4.md)
- [Sum S4](docs/changelogs/SumS4.md)
- [Set](docs/changelogs/Set.md)
- [Pulse](docs/changelogs/Pulse.md)
- [Tag](docs/changelogs/Tag.md)
- [Squeeze](docs/changelogs/Squeeze.md)
- [Shape](docs/changelogs/Shape.md)
- [Master](docs/changelogs/Master.md)
- [Gain](docs/changelogs/Gain.md)
- [Sweep](docs/changelogs/Sweep.md)
- [Loop](docs/changelogs/Loop.md)
- [Clang](docs/changelogs/Clang.md)
- [React](docs/changelogs/React.md)
- [Sync](docs/changelogs/Sync.md)
- [Flip](docs/changelogs/Flip.md)
- [Orbit](docs/changelogs/Orbit.md)

## v2.19.1 (2026-08-26)

- Loop now embeds loaded WAV files in VCV patch storage, while preserving external file paths as a backward-compatible fallback.
- Loop Reset and Trigger/Reset now restart immediately when Sync is disabled or no Clock cable is connected.
- Loop keeps reset clock-quantized when Sync is enabled and Clock is connected.

## v2.19.0 (2026-08-22)

- Added Sum M4, a compact four-channel mono mixer with level, pan, clickless mute and stereo Chain I/O.
- Added Sum S4, a compact four-channel stereo mixer with mono-normalled inputs, level, clickless mute and stereo Chain I/O.
- Added Set, a clocked performance timer with run/pause, separate timer and pulse-phase resets, and selectable musical pulse output.
- Added Pulse, a slim visual trigger indicator and mono final-mix level and peak monitor.
- Added Tag, an editable patch-section label with right, left or no direction arrow.

## v2.18.0 — 2026-08-12

- Refreshed the complete 15-module VCV Rack collection with the current Submit Audio panel and knob design.
- Drift: refined the 0-Coast-inspired voice, slope behaviour, defaults and CV response while preserving patch compatibility.
- Chrono: strengthened Feedback and Surge, added a smooth momentary Dry transition and optional extended clocked Time ratios.
- Squeeze: improved combined Gate and Audio sidechain detection with smoother, more stable envelope behaviour.
- Shape: rebuilt the equalizer around a high-quality SSL 4K-inspired topology with smoothed, double-precision stereo processing.
- Master: rebuilt the mastering chain with improved transient shaping, glue, width handling and stereo-linked limiting.
- Sweep: removed occasional centre/reset transition clicks while preserving its Dry reset behaviour.
- React: improved 1 PPQN clock phase alignment and deterministic reset timing while retaining legacy patch compatibility.
- Updated VCV Library descriptions, manuals and module-specific changelogs.

## v2.17.0 — 2026-08-04

- Added Sync, Flip and Orbit to the public collection.
- Standardized the new timing modules around the Submit Audio 1 PPQN clock convention.
- Fixed the static-analysis findings reported during VCV Library integration.

## v2.15.3 — 2026-05-28
- Impact: SPREAD replaced by SNAP transient shaping.
- Impact: manual section updated for the new SNAP control.
- Plugin license metadata fixed to valid SPDX identifier `GPL-3.0-only`.

## v2.15.2 — 2026-05-11
- Module descriptions and tags added to `plugin.json`.
- Manual URL added to `plugin.json`.
- README updated with Loop and official module list.

## v2.15.1 — 2026-05-10
- Full user manual added.
- Manual links added for all modules.
- Submit Mixer System section updated.
- Gain documentation updated.

## v2.15.0 — 2026-05-10
- Impact: click/tick on retrigger fixed
- Impact: block-size timing issues resolved for MetaModule
- Loop: reverse playback synchronized to bar boundary
- macOS Apple Silicon build included

## v2.14.0
- Loop added as official module
- Various stability improvements

## v2.13.0
- Initial public release
- Drift, Chrono, Impact, Chain, Squeeze, Shape, Master, Gain, Sweep
