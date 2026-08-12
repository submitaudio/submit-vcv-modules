# React changelog

## Unreleased

## v2.18.0

- Updated the panel and rotary controls to the current Submit Audio design.
- Improved 1 PPQN clock phase alignment so each real clock edge anchors the quarter-note boundary.
- Made Reset deterministic in 1 PPQN mode and prevented free-running subdivisions from producing an early downbeat.
- Existing saved patches without the new clock setting continue to use the legacy timing mode.

## v2.16.1

- New instances use 1 PPQN (quarter-note clock).
- Existing patches retain 4 PPQN legacy compatibility.
- Corrected clock phase alignment and improved timing stability for synchronized rhythm output.
