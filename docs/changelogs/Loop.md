# Loop changelog

## v2.23.0 (release candidate)

- Added a Play/Stop gate input and momentary button parameter. Each rising edge toggles playback; sustained gates and button presses toggle only once. Simultaneous edges toggle once.
- With Sync enabled and Clock connected, STOP finishes the entire current loop before stopping at its wrap boundary, including reverse playback. Quarter-note edges cannot stop it early. START waits for the next quarter-note edge, including in 4 PPQN compatibility mode. A second request cancels a pending action. With Sync disabled or no Clock connected, start/stop remains immediate.
- Added a 2 ms audio ramp on both MAIN and CUE. A synchronized stop fades at the end of the final cycle and resets the position for the next start. Unclocked stop pauses and resumes at the stopped position; existing Reset behavior remains available.
- Kept clock tracking active while stopped and stored the playing state in patches. Legacy patches retain automatic playback; an already-high gate at patch load does not override the saved state. Existing control IDs remain unchanged.
- Applied the supplied V4 panel as `res/Loop.svg` and retained the component reference as `res/LoopComponents.svg`. Placed React's compact buttons and the corresponding inputs at the V4 positions, with PLAY left and RESET right. The component SVG's two port IDs are swapped, so the implementation follows the visible labels. OUT/R follows its new component position; other controls remain unchanged. PLAY immediately displays the requested state, including gate-controlled changes: a queued STOP turns the button off while the final loop finishes; START or cancelling STOP turns it on.
- V4 panel and control placement inspected in Rack. Revised cycle-end stop tests cover button and gate, forward/reverse, 1/4 PPQN and immediate behavior with Sync off. José approved Rack listening and CPU stress testing on 2026-09-27.
- Validation: local plugin build passed; `tests/LoopTransportTest.cpp` passed against the Rack SDK at 44.1, 48 and 96 kHz for gate/button interaction, fades, pause/resume, clock/reset and saved state. Prepared for the authorized 22-module public release.

## v2.21.0 (2026-08-30)

- Expanded the display with separate BPM and Bars status fields.
- Added a measured external-clock BPM readout and clear manual or automatic Bars status.
- Enlarged the display area while preserving the existing looper controls and patch behaviour.

## v2.20.0 (2026-08-27)

- Added a stored clock input rate choice with 1 PPQN as the Submit default and 4 PPQN as a compatibility option.

- Embedded loaded WAV files in VCV patch storage so saved patches reopen without depending on the original sample location.
- Kept external file references as a backward-compatible fallback for older patches and presets.
- Fixed Reset and Trigger/Reset so they restart playback immediately when Sync is disabled or no Clock cable is connected.
- Preserved clock-quantized reset on the next pulse when Sync is enabled and Clock is connected.

## v2.19.0 (2026-08-22)

- Switched the display to the bundled Share Tech Mono font for consistent rendering across platforms.

## v2.18.0

- Updated the panel and rotary controls to the current Submit Audio design.
- Removed the unintended outline around the display area.
- Preserved sample playback, clock synchronisation, reverse, cue and saved patch behaviour.

## v2.16.1

- Clock input documented as 1 PPQN (quarter-note clock).

## v2.15.0

- Reverse playback synchronized to the bar boundary.

## v2.14.0

- Added Loop as an official module.
