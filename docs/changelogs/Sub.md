# SUB changelog

## v2.20.0 (2026-08-27)

- Promoted the selected beta implementation to the canonical `Sub` module and removed the older local SUB implementations.
- Expanded OCTAVE to nine discrete positions from -4 to +4 octaves.
- Changed TUNE to a -100 to +100 cent upper-layer adjustment while keeping the dedicated sub layer locked to pitch.
- Separated the analog-style upper bass voice from a dedicated warm FM/phase sub layer one octave lower.
- Added SUB and SUB CV mixing with stable upper-layer behaviour at zero.
- Replaced the former volume-style Drive control with compensated warm asymmetric saturation and Drive CV.
- Smoothed short Accent signals with an 8 ms attack and 40 ms release while keeping the fundamental sub level stable.
- Corrected and centred the complete panel and all controls on an exact 12 HP canvas without stretching.
- Integrated the final Sub panel and exact component positions from the Illustrator component drawing.
- Added a continuously variable `TUNE` control from -12 to +12 semitones using `SubmitKnobMedium`.
- Changed the four-position OCTAVE control to `SubmitKnobTiny`; it remains discretely stepped and keeps its original parameter ID for patch compatibility.
- Kept `SubmitKnobSmall` on the other six rotary controls.
- Added the six panel CV inputs for Filter, Resonance, Envelope Amount, Decay, Sub Mix and Drive while preserving all existing IDs; their final modulation behaviour remains part of the upcoming sound/DSP round.
- Kept the legacy envelope output ID reserved but no longer exposed it on the final panel.

- Added SUB as a separate local beta; older SUB versions remains unchanged.
- Added the OP-Z Analog-inspired engine: saw, sub oscillator and subtle attack-noise through a filter envelope.
- Added `MIX` for saw/sub/noise balance and `ENV AMT` for filter-envelope depth.
- Kept the design focused on one dedicated Analog bass engine instead of multiple synthesis modes.
- Extended `LENGTH` to four seconds and linked it to both the amplitude tail and a much longer filter-envelope contour.
- Local beta only; not included in a release.
