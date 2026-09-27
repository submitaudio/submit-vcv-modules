# Chrono changelog

## v2.23.0 (release candidate)

- Fixed Heads routing: the selected three-head mix now feeds the stereo wet output, so the selector also works with Feedback at zero and Surge off.
- Heads panel positions 1 through 6, from top to bottom, now select ALL, TRP, DOT, QTR, DUB, SUB in both the stereo wet output and feedback/Surge paths. Tooltips use the same order. Existing combinations retain their weights, with their selector mapping corrected to match the panel.
- Applied Offset, Spread and Tape wobble to the audible heads, with a 5 ms smoothing time for wet head selection changes.
- Existing patches retain their parameter IDs and stored values, but the corrected head mapping changes the selected combinations at SUB, DUB, TRP and ALL; QTR and DOT retain their combinations. The wet sound also changes as the routing fix becomes audible. José approved Rack listening and CPU stress testing on 2026-09-27.
- Validation: local plugin build passed. The actual-DSP impulse regression passed at 44.1, 48 and 96 kHz in free and clocked modes: all six mixes differ at zero feedback, all three echoes are present, Heads CV matches the selector, Spread separates the outputs and Offset changes echo timing.

## v2.20.0 (2026-08-27)

- Added a stored clock input rate choice with 1 PPQN as the Submit default and 4 PPQN as a compatibility option.

## v2.19.0 (2026-08-22)

- Included in Submit 2.19.0 with no module-specific functional changes.

## v2.18.0

- Updated the panel and rotary controls to the current Submit Audio design.
- Feedback has a subtle 12% mid-range lift while retaining its original minimum and maximum.
- Surge has slightly stronger feedback, drive, and bloom for a more pronounced performance effect, followed by a smooth three-second release.
- The former Break control is now a smooth momentary Dry bypass with one-second fades in both directions.
- In clocked mode, Time can select additional musical ratios from 1/4x through 4x, with the existing default position remaining 1x.
- Existing patches retain the original clocked Time behavior and can enable the new ratios from the context menu.

## v2.16.1

- Clock input documented as 1 PPQN (quarter-note clock).
