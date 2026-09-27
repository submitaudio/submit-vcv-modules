# Sync changelog

## v2.23.0 (release candidate)

- Added patch-stored START CV Mode (Trigger / Gate) in the context menu. Trigger remains the default, including for older patches.
- In Gate mode, a connected START input follows HIGH/LOW to run/stop the clock. Manual Run and Stop overrides last until the next gate transition; Stop takes priority for simultaneous events.
- Kept the front panel, port IDs and Trigger-mode behaviour unchanged.

## v2.21.0 (2026-08-30)

- Added dedicated Start and Stop trigger inputs.
- Made Stop take priority when Start and Stop arrive together.
- Appended the new inputs after the existing ports to preserve patch compatibility.

## v2.20.0 (2026-08-27)

- Added a stored clock input/output rate choice with 1 PPQN as the Submit default and 4 PPQN as a compatibility option.

## v2.19.0 (2026-08-22)

- Switched the tempo display to the bundled Share Tech Mono font for consistent rendering across platforms.

## v2.18.0

- Updated the panel and Tempo control to the current Submit Audio design.
- Preserved the 1 PPQN timing, related clock outputs and deterministic reset behaviour.

## v2.17.0

- Initial public release.
- Added the compact internal/external 1 PPQN clock with phase-related x1, x2, /2 and Reset outputs.
- Included the static-analysis corrections requested during VCV Library integration.
