# Submit Audio — User Manual

This GitHub manual is the versioned technical reference for the Submit Audio VCV Rack plugin. The friendly, illustrated manual remains available on the [Submit Audio website](https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/).

The public release modules are Drift, Chrono, Impact, Chain, Sum M4, Sum S4, Set, Pulse, Tag, Squeeze, Shape, Master, Gain, Sweep, Loop, Clang, React, Sync, Flip and Orbit. Beta modules are not included until they are approved for a public release.

## Contents

- [Submit Mixer System](#submit-mixer-system)
- [Clock standard](#clock-standard)
- [Drift](#drift)
- [Chrono](#chrono)
- [Impact](#impact)
- [Chain](#chain)
- [Sum M4](#sum-m4)
- [Sum S4](#sum-s4)
- [Set](#set)
- [Pulse](#pulse)
- [Tag](#tag)
- [Squeeze](#squeeze)
- [Shape](#shape)
- [Master](#master)
- [Gain](#gain)
- [Sweep](#sweep)
- [Loop](#loop)
- [Clang](#clang)
- [React](#react)
- [Sync](#sync)
- [Flip](#flip)
- [Orbit](#orbit)
- [Support and bug reports](#support-and-bug-reports)

## Submit Mixer System

Chain, Sum M4, Sum S4, Squeeze, Shape, Master and Gain are designed to work together as a modular mixing system.

1. Use Chain for stereo channels, pre-gain, pan, filtering, mute and FX sends.
2. Use Squeeze to generate sidechain CV and patch it to Chain's COMP/CV input.
3. Use Shape as a shared stereo EQ in an FX loop.
4. Use Master as the final stereo bus processor.
5. Use Gain to bring external line-level signals up to modular level, especially on MetaModule.

Chain, Sum M4 and Sum S4 modules can be daisy-chained to expand the number of channels. The send and return buses on Chain allow one effect module to process several channels.

## Clock standard

Submit Audio uses **1 PPQN** as the standard clock: one pulse per quarter note. Modules that need faster rhythmic events derive those events internally.

React and older clocked patches retain their documented legacy compatibility where applicable. Check the module's context menu for any available clock compatibility setting.

## Drift

Drift is a West Coast-inspired voice with two oscillator outputs, an overtone/multiply section, a slope generator and a contour envelope. It is inspired by the Make Noise 0-Coast while retaining its own sound and controls.

### Controls

- **Octave** and **Fine** set the oscillator pitch.
- **Overtone** adds harmonic content.
- **Multiply** expands the overtone relationship from zero to maximum.
- **Rise**, **Fall**, **Time** and **Curve** shape the slope generator.
- **Onset**, **Sustain**, **Decay** and **Exp** shape the contour envelope.
- **Timbre** balances the voice; the Timbre switch enables its alternate behaviour.
- The Rise switch selects the active Rise behaviour. Rise OFF keeps the standard voice response; Rise ON exposes the full Rise/Fall/Time interaction.

### Inputs

`V/OCT`, `FM`, `OVR`, `MLT`, `TRIG`, `GATE`, `SLP`, `DCY`, `CTR`, `DYN`, fundamental CV, overtone-balance CV, external input and Timbre CV.

### Outputs

`TRI`, `SQR`, `EOC`, `EON`, `SLP`, `ENV` and `LINE OUT`.

### Patch ideas

- Patch `TRIG` to start the slope and `ENV` to an external VCA for a plucked voice.
- Patch `SLP` back to oscillator or filter modulation for evolving West Coast tones.
- Use a slow CV on `OVR` or `MLT` for metallic movement while keeping the fundamental stable.

## Chrono

Chrono is a clock-synchronised stereo delay with tape movement, saturation and six rhythmic head modes.

### Controls

- **Time**, **Feedback**, **Mix**, **Drive** and **Tape** control the delay and tape character.
- **Heads** selects `SUB`, `DUB`, `QTR`, `DOT`, `TRP` or `ALL` rhythmic head combinations.
- **Division** and **Offset** place the heads against the incoming clock.
- **Spread** controls stereo width.
- **Surge** freezes and blooms the delay; **Break** creates a tape-stop style slowdown.

### Inputs and outputs

Inputs: stereo audio, Time CV, Feedback CV, Mix CV, Drive CV, Tape CV, Heads CV, 1 PPQN Clock, Offset CV, Spread CV, Surge Gate and Break Gate.

Outputs: `Audio Out L` and `Audio Out R`.

### Patch ideas

Use the 1 PPQN clock for locked rhythmic echoes. Increase Tape and Drive gradually for movement, then automate Surge or Break for transitions.

## Impact

Impact is a stereo kick and percussion synthesizer with Pure and Harsh engines.

### Controls

- **Pitch**, **Decay**, **Punch**, **Morph**, **Harm**, **Fold**, **Noise**, **Snap** and **Noise Length** shape the sound.
- **Noise Type** selects Rumble, Crunch or Dust.
- **Mode** selects Harsh or Pure synthesis.
- Each of Decay, Punch, Morph, Noise and Fold has an attenuverter for its CV input.
- **Try Me** triggers the module from the panel.

### Inputs and outputs

Inputs: `Trigger`, `Pitch CV (1V/oct)`, Punch CV, Morph CV, Decay CV, Noise Length CV, Fold CV and Accent.

Outputs: `Audio Out L` and `Audio Out R`.

### Patch ideas

Start with Pure mode, moderate Punch and short Decay for a clean kick. Switch to Harsh, raise Harm and add Fold for metallic techno percussion. Crunch adds a sharper transient; Rumble extends the low tail.

## Chain

Chain is a stereo mixer and router with two channel strips and shared FX buses.

### Controls

Each channel has pre-gain, volume, pan, mute, a 40 Hz high-pass switch and two FX sends.

### Inputs

Channel 1 and 2 stereo inputs, COMP/CV inputs, mute CV inputs, chain inputs, send-chain inputs and return inputs.

### Outputs

Stereo chain output plus left/right outputs for FX Send 1 and FX Send 2.

### Patch ideas

Daisy-chain Chain modules for more channels. Connect Squeeze COMP OUT to a Chain COMP/CV input for sidechain pumping. Route a shared Shape or Sweep module through a send/return pair.

## Sum M4

Sum M4 is a compact four-channel mono mixer and a small-format extension of the Submit Chain family.

### Controls, inputs and outputs

- Each channel has a level control, constant-power pan and clickless mute.
- Inputs 1–4 accept mono audio.
- `Chain In L/R` adds an incoming stereo Chain mix without changing its level.
- `Out L/R` carries the combined stereo mix to another Chain-family module or a final output stage.

### First patch

Connect up to four mono voices, set their levels and stereo positions, and patch `Out L/R` to the next Chain-family module. Use `Chain In L/R` to extend an existing mixer chain.

## Sum S4

Sum S4 is a compact four-channel stereo mixer and a small-format extension of the Submit Chain family.

### Controls, inputs and outputs

- Each channel has a level control and clickless mute.
- Inputs 1–4 provide left and right connections; an unpatched right input follows its left input for mono-to-stereo use.
- `Chain In L/R` adds an incoming stereo Chain mix without changing its level.
- `Out L/R` carries the combined stereo mix.

### First patch

Connect stereo sources to the four channel pairs, or use only the left input for a centred mono source. Patch `Out L/R` to another Chain-family module or the final output stage.

## Set

Set combines a performance timer with a clock-derived pulse output.

### Controls, inputs and output

- **Run** starts or pauses the timer. Press it again to continue from the stored time.
- **Reset** resets only the displayed timer to zero.
- The pulse selector chooses a quarter-note, half-note or whole-note pulse derived from the 1 PPQN clock.
- `Clock` accepts the 1 PPQN source.
- `Run` starts or resumes the timer from an external trigger.
- `RST` resets the pulse phase and active pulse state without changing the timer.
- `Pulse` outputs the selected musical pulse.

### First patch

Patch a 1 PPQN clock to `Clock`, choose a pulse interval and send `Pulse` to a rhythmic destination. Use the panel Run button as a performance stopwatch; pausing and resuming does not disturb the elapsed time.

## Pulse

Pulse is a slim visual utility for checking a trigger and the level of a mono final mix.

### Inputs and display

- `Pulse` flashes the large yellow display when a trigger arrives.
- `Mix` drives the vertical level indication.
- The display turns red when the monitored signal reaches the peak-warning threshold.

### First patch

Connect an important rhythmic trigger to `Pulse` and a mono mix or bus signal to `Mix` for an immediate visual reference during performance.

## Tag

Tag is an editable 2 HP label for organising larger patches.

### Editing and arrow direction

- Click directly in the LCD text field and type a label of up to 38 characters.
- Click the arrow to cycle between right, no arrow and left.
- The text and arrow direction are stored with the patch.

Place Tag beside the modules or cable group it describes, then point the arrow toward that patch section.

## Squeeze

Squeeze generates sidechain control voltage for Chain's compressor input. Gate and audio signals can be used independently or at the same time. When both are connected, Squeeze follows the stronger envelope so the two sources cooperate without doubling the control voltage.

### Controls

- **Attack** and **Release** shape both gate and audio detection. **Amount** sets the maximum ducking depth.
- **Contour** selects Lin, Exp or Log behaviour.

### Inputs and output

Inputs: Gate In and Audio In.

Output: `Comp Out`.

## Shape

Shape is a high-quality SSL 4K-style stereo equalizer for the Submit signal chain. It combines the classic four-band channel-EQ layout with dedicated high-pass and low-pass filters.

### Controls

- **High Pass** runs from Off to 16–350 Hz with an 18 dB/octave slope.
- **Low Shelf** is centred at 80 Hz with ±15 dB of gain.
- **Low Mid** is centred at 400 Hz with ±15 dB of proportional-Q gain.
- **High Mid** is centred at 2.5 kHz with ±15 dB of proportional-Q gain.
- **High Shelf** is centred at 10 kHz with ±15 dB of gain.
- **Low Pass** runs from Off to 22–3 kHz with a 12 dB/octave slope.

All controls are smoothed to prevent zipper noise. The right input is normalled from the left input for mono-to-stereo operation.

### Inputs and outputs

Inputs: `Audio In L` and `Audio In R`.

Outputs: `Audio Out L` and `Audio Out R`.

## Master

Master is a stereo bus processor for the final stage of the Submit chain.

### Controls

- **Output** sets the final gain.
- **Transient** adds or removes transient emphasis.
- **Glue** applies bus cohesion.
- **Width** controls stereo width.
- **Limit** controls final limiting.

### Inputs and outputs

Inputs: `Audio In L` and `Audio In R`.

Outputs: `Audio Out L` and `Audio Out R`.

## Gain

Gain is a line-level utility with gain, volume, mute and FX routing. It is designed for external gear and MetaModule line-level connections.

### Controls

- **Gain** provides up to 10x input gain.
- **FX Send** routes signal to the shared FX bus.
- **Volume** sets the output level.
- **Mute** silences the channel.

### Inputs and outputs

Inputs: Line Level L/R, COMP/CV, Mute CV, Chain In L/R, FX Chain In L/R and FX Return L/R.

Outputs: Chain Out L/R and FX Chain Out L/R.

## Sweep

Sweep is a stereo DJ-style filter for the Submit chain.

### Controls and inputs

- **Sweep** moves the filter between low-pass and high-pass behaviour.
- **Resonance** adds emphasis around the cutoff.
- **Reset** returns the filter to its reset position.

Inputs: Sweep CV, Resonance CV, Reset CV and Chain In L/R.

Outputs: Chain Out L/R.

## Loop

Loop is a stereo sample looper with waveform display, clock sync, BPM parsing, reverse playback and cue output.

### Controls

- **Bars** sets loop length; zero uses automatic detection.
- **BPM** sets tempo; zero uses the file or clock information.
- **Speed** controls playback speed and direction.
- **Sync** enables clock synchronisation.
- **Reverse** reverses playback.
- **Cue** sends the cue signal to the mono cue output.
- **Reset** returns playback to the start. With Sync enabled and Clock connected, the reset occurs on the next clock pulse. Without an active clock-sync connection, it occurs immediately.
- **Bar Shift** moves the playback window by bars.

### Inputs and outputs

Inputs: Clock, Trigger/Reset, Speed CV, Bars CV, BPM CV, Bar Shift CV and Reverse CV. Trigger/Reset follows the same immediate or clock-quantized behaviour as the Reset button.

Outputs: Main L, Main R and Cue Mono.

### Loading samples

Right-click the module and choose **Load WAV...**. Loop shows the waveform and derives BPM information when it is present in the file or filename. Reverse playback is aligned to the bar boundary when synchronised.

## Clang

Clang is a techno percussion synthesizer with Physical and FM engines, eight models and eight sound variations.

### Controls

- **Engine** selects Physical or FM synthesis.
- **Model** selects the percussion model.
- **Sounds** selects the sound variation.
- **Decay**, **Tune**, **Body**, **Motion**, **Noise** and **Tune Variation** shape the result.

### Inputs and output

Inputs: Trigger, Model CV, Sounds CV and Motion CV.

Output: `Audio Out`.

## React

React is a four-voice rhythm sequencer for kick, snare, hihat and percussion.

### Controls

- **Pattern** morphs between twelve patterns.
- **Snare**, **Hihat** and **Perc** select voice variations.
- **Genre** selects one of eight genre variations.
- **Drop** removes or restores pattern material.
- **Variation** selects an alternate performance variation.

### Inputs and outputs

Inputs: Clock, Reset, Morph, Drop and Variation.

Outputs: `Kick`, `Snare`, `Hihat` and `Perc` triggers.

New instances use 1 PPQN. Existing patches retain the 4 PPQN legacy mode where stored in the patch.

## Sync

Sync is a compact internal/external clock for the Submit system. This module is part of the VCV 2.17.0 candidate.

### Controls and inputs

- **Tempo** sets the internal BPM.
- **Run** starts and stops the internal clock.
- **Reset** resets the clock phase.
- `External clock` selects external timing when connected.
- `Reset` accepts an external reset pulse.

### Outputs

- `Clock (x1)` — one pulse per quarter note (1 PPQN).
- `Double-speed clock` — ×2 clock.
- `Half-speed clock` — ÷2 clock.
- `Reset pulse` — reset output.

## Flip

Flip is a stereo clock-synchronised reverse performance effect. This module is part of the VCV 2.17.0 candidate.

### Controls and inputs

- **Flip** triggers a rhythmic reverse action.
- **Freeze** holds the captured material.
- **Length** selects 1/2, 1/4 or 1/8 rhythmic length.
- **Dry/Wet** is retained for patch compatibility.

Inputs: stereo audio, 1 PPQN clock, Gate In and Freeze Gate.

Outputs: stereo audio.

## Orbit

Orbit analyses two input rhythms and derives their musical relationship. This module is part of the VCV 2.17.0 candidate.

### Controls and inputs

- **Position** selects the point between Rhythm A and Rhythm B to analyse.
- **Window** sets the relationship window.
- **Memory** smooths the relationship analysis.
- **Hold** freezes the current outputs.
- **Range** scales the tension range in beats.

Inputs: Rhythm A, Rhythm B, 1 PPQN Clock and Reset.

### Outputs

- `Aligned events` — events where A and B align.
- `Events between A and B` — events occurring between the two rhythms.
- `Detected rhythmic gaps` — detected gaps in the relationship.
- `Rhythmic tension CV` — tension derived from the relationship.

## Support and bug reports

- [Submit Audio website](https://www.submitaudio.nl)
- [Online manual](https://www.submitaudio.nl/vcv-rack-modules-metamodule-plugins/)
- [Per-module changelogs](CHANGELOG.md#module-changelogs)
- [GitHub issues](https://github.com/submitaudio/submit-vcv-modules/issues)
- [Latest GitHub release](https://github.com/submitaudio/submit-vcv-modules/releases/latest)

Submit Audio is released under [GPL-3.0-only](LICENSE).
