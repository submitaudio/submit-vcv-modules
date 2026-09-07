# Drift changelog

## v2.22.0 (2026-09-07)

- Jose approved replacing the original implementation with the revised voice
  under the existing name and slug `Drift` (2026-09-07). The separate `DriftV2`
  registration and original `src/Drift13.cpp` implementation are removed.
- The implementation and V9 panel retain their development filenames
  `src/DriftV2.cpp` and `res/DriftV2.svg`: revised Overtone/Multiply,
  audio-rate CV, AC-coupled FM,
  audio-rate Slope, Contour CV, panel amount controls and Drone.
- All original parameter and port IDs are retained, with new IDs appended.
  Old Drift patches resolve to the new voice, but DSP and timing changes mean
  identical sound is not guaranteed. Local patches saved as `DriftV2` require
  explicit conversion to `Drift`; there is no remaining `DriftV2` model.
- Approved starting values and removal of the Beta V2 panel label are retained.
  Released as part of Submit 2.22.0 in the VCV Library.

## v2.19.0 (2026-08-22)

- Included in Submit 2.19.0 with no module-specific functional changes.

## v2.18.0

- Updated the panel and rotary controls to the current Submit Audio design.
- Refined the 0-Coast-inspired oscillator and wavefolder response while retaining the established sound character.
- Corrected Multiply to follow its full 0-to-maximum range and removed the unwanted roughness around its centre position.
- Restored the musical Rise, Fall and Time interaction and expanded useful control across the knob ranges.
- Made the Slope CV input clearly affect the slope curve and allowed the CV input to control Rise when Cycle is off.
- Added click-resistant smoothing to the main overtone, multiply and balance paths.
- Updated factory defaults to the approved musical starting patch.
- Existing controls, ports, slug and saved patches remain compatible.

## v2.16.1

- Module-specific changelog tracking starts with this release cycle.
