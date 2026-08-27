# Circles changelog

## v2.20.0 (2026-08-27)

- Added a stored `Clock input rate` context setting with 1 PPQN as the Submit default and 4 PPQN as a compatibility option.
- The panel `GATE` control now shapes both the main gate and the slow `SUB GATE`; `SUB BARS` controls note spacing without automatically creating multi-bar bass drones.
- Promoted the selected beta implementation to the canonical `Circles` module and removed the older local Circles implementation.
- Integrated the final 30 HP Circles panel and its exact GrPhX component positions.
- Applied the new `SubmitKnobPetite` component to every rotary control on Circles.
- Restored the rotating red step indication on the V3 step-enable buttons: enabled waiting steps are yellow and the current step is red.
- Added an LCD step indicator: all original segments remain physically fixed and four adjacent LEDs switch in exact synchrony with the active red sequencer step, including non-linear FLOW modes.
- The LCD now uses the complete original `Led_ring` compound path from the Illustrator component drawing, preserving all 48 individual shapes and positions exactly; only their fixed dim/lit state changes.
- Smoothed the LCD chase continuously across the full step period: outgoing and incoming fixed LED shapes crossfade while the four-segment light band travels around the ring without pauses or six-position jumps.
- Rebuilt the LCD from the two supplied Illustrator layers: the standard view now shows active step, note, Scale Chain slot, root/scale and measured BPM; turning a rotary control temporarily shows its name and value for 1.35 seconds.
- Removed the four Rack screw widgets because the final panel artwork already contains its own screws.
- Added eight stored step-enable buttons. Disabled steps remain in the eight-step cycle as rests and emit no trigger or gate.
- Replaced the visible EOC connection with a `NEXT` input that advances to the next enabled Scale Chain slot; the legacy EOC output ID remains reserved for patch compatibility.
- Added a separate `SUB V/OCT` and slow `SUB GATE` derived from one selected sequence step.
- Added a four-slot Scale Chain with 1–8 bars per active scale.
- Scale changes occur on bar boundaries and retune the selected sub source musically.
- Added independent stored `SUB STEP` choices for scale slots A–D, selected with the new `EDIT` control.
- The sub pitch now changes both source step and scale on a bar boundary, then remains latched for the full slow sub gate.
- Added `SUB BARS` to set an independent 1–8 bar interval and gate duration for the slow sub voice.
- Added Scale A–D LEDs: the active scale is bright, enabled inactive slots are dim, and unused slots are off.
- Added `SUB SHIFT` (0–7) to launch the sub on any circle step while keeping it locked to the sequence clock; default is Step 1.
- Replaced the linear scale-count workflow with individual A–D scale enable buttons, so any combination of scale slots can be chained.
- Moved all scale status into the illuminated scale buttons: off means skipped, yellow means selected and waiting, and red means the currently active scale. Removed the separate scale LEDs.
- Added eight dedicated Step CV inputs around the note circle. Each CV is sampled when its step starts, added to that step's note setting, and then quantized in the active scale.
- Added five-mode `FLOW`: Forward, Reverse, Pendulum, Drunk and Random. It changes the main note order while the clock, Scale Chain and slow sub remain bar-synchronised.
- Added `SPEED` for the main circle: 1/4, 1/8, 1/16 or 1/32. It changes only the main notes, trigger and gate; the Scale Chain, `SUB BARS` and sub timing keep their original bar grid.
- Added `FLOW CV`: with a cable connected, 0–10 V selects Forward, Reverse, Pendulum, Drunk or Random; without a cable, the `FLOW` knob remains in control.
- Added an illuminated panel `DICE` button that generates an eight-step scale-quantized motif while leaving Scale Chain, sub, FLOW, SPEED and gates unchanged. Dice character can be set to Subtle, Musical or Wild in the context menu; generated knob positions and the selected character are saved with the patch, and generation is undoable as one action.
