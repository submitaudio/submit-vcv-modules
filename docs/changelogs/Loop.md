# Loop changelog

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
