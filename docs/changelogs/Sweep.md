# Sweep changelog

## v2.23.0 (release candidate)

- Removed the broad resonance gain compensation that caused a level drop when moving away from centre. The filter passband retains unity gain.
- Kept exactly 12 o'clock unfiltered. Immediately clockwise, a non-resonant low-cut fades in, remaining independent of RES through 1 o'clock. Resonance then increases smoothly to its full range at 2 o'clock. Left-side LP resonance remains limited to Q=1. This is not a speaker-protection limiter.
- Sped up the momentary Reset motion: the knob now reaches the centre in about 38 ms from either end, retaining the smooth transition to dry and the existing Reset CV control.

## v2.19.0 (2026-08-22)

- Included in Submit 2.19.0 with no module-specific functional changes.

## v2.18.0

- Updated all rotary controls to the current Submit Audio knob design.
- Smoothed the transition around the centre bypass position to prevent occasional clicks.
- Preserved the momentary Reset behaviour that fades the output to Dry.

## v2.16.1

- Module-specific changelog tracking starts with this release cycle.
