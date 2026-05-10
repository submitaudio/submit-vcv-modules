## The Submit Mixer System

Chain, Squeeze, Shape and Master are designed to work together as a complete mixing and mastering signal chain. Connect them via the CHAIN jacks for a seamless workflow — from individual channel mixing to final master output.

**Chain** handles your stereo channels with pre gain, HPF and FX sends. You can daisy-chain multiple Chain modules together to build a larger mixer. Each channel has its own level, pan, mute and two FX send outputs that can feed shared effects processors.

**Squeeze** generates sidechain CV that feeds directly into Chain's compressor input. Because all Chain modules share the same sidechain bus, you only need one Squeeze to control the dynamics of the entire mix. Connect a kick drum or master bus signal for classic pumping compression.

**Shape** adds SSL-style EQ character to your mix. Connect one Shape module to the FX send of multiple Chain modules using multiple cables from the same output — one Shape for the whole mix without needing separate modules per channel.

**Master** adds warmth, transient control and limiting as the final stage. It sits at the end of the chain and brings everything together before hitting the output.

**Gain** is a line level booster for bringing external signals up to modular level before entering the chain. Especially useful on the 4ms MetaModule.

### Signal Flow
### Example patch

1. Place two Chain modules for 4 stereo channels
2. Feed Squeeze with a trigger or audio from a drum module and connect COMP OUT to COMP/CV on Chain for sidechain ducking
3. Add one Shape as a shared FX send for all channels
4. Connect everything to Master
5. Use Gain when a signal is too low — specially made for the MetaModule

---

# Submit Audio — User Manual

> Submit Audio is a collection of modules for VCV Rack, designed for electronic music production with a focus on quality sound and intuitive workflow.

---

## Table of Contents
- [Drift](#drift)
- [Chrono](#chrono)
- [Impact](#impact)
- [Chain](#chain)
- [Squeeze](#squeeze)
- [Shape](#shape)
- [Master](#master)
- [Gain](#gain)
- [Sweep](#sweep)
- [Loop](#loop)

---

## Drift

Drift is a compact all-in-one voice module combining oscillator, wavefolder, slope and contour. Inspired by the Make Noise 0-Coast, but designed with its own character and behaviour — not a clone. It generates complex, evolving waveforms that are never exactly the same twice, making it ideal for organic leads, drones, and experimental textures.

The chaos parameter controls how much the oscillator deviates from its base frequency — at low settings it behaves like a regular oscillator, at high settings it produces unpredictable, glitchy sounds. The morph parameter smoothly transitions between sine, triangle, saw and square waveforms.

### Inputs
| Input | Description |
|-------|-------------|
| V/OCT | Pitch CV (1V/oct) |
| FM | Frequency modulation input |
| SYNC | Hard sync — resets the phase |

### Outputs
| Output | Description |
|--------|-------------|
| OUT | Main audio output |

### Parameters
| Parameter | Description |
|-----------|-------------|
| PITCH | Base frequency in Hz |
| CHAOS | Amount of chaotic deviation (0 = stable, max = fully chaotic) |
| MORPH | Waveform morph from sine to square |

### Tips
- Use a slow LFO on the CHAOS input for evolving, organic textures
- Low CHAOS settings work great for classic oscillator sounds
- SYNC input can be used to lock Drift to another oscillator
- Enable the slope and set it to self-oscillate — play with Fall and Time for evolving textures

---

## Chrono

Chrono is a tape delay inspired by classic tape echoes, focused on movement and character. It brings the warmth, imperfection and rhythmic flexibility of vintage tape machines into your modular setup.

Six tape head combinations give you a wide range of rhythmic echo patterns. The TAPE control adds authentic tape character ranging from subtle hiss to wow and flutter with dropouts. DRIVE adds tape-style saturation for warmth and grit. SURGE freezes and blooms the delay for dramatic build-ups, while BREAK creates a tape stop effect that slows and stops the echoes.

Clock sync with divisions and head offset keeps everything locked to your tempo. Stereo spread widens the delay image, and full CV and gate control lets you automate everything in your patch.

### Inputs
| Input | Description |
|-------|-------------|
| IN L | Left stereo input |
| IN R | Right stereo input |
| CLK | Clock input for sync |
| SURGE | Freeze and bloom trigger |
| BREAK | Tape stop trigger |

### Outputs
| Output | Description |
|--------|-------------|
| OUT L | Left stereo output |
| OUT R | Right stereo output |

### Parameters
| Parameter | Description |
|-----------|-------------|
| TIME | Delay time |
| MIX | Dry/wet mix |
| DRIVE | Tape saturation amount |
| TAPE | Tape character: hiss, wow, flutter and dropouts |
| HEADS | Tape head combination selector (6 rhythmic variations) |
| SPREAD | Stereo spread of the delay taps |
| SURGE | Freeze and bloom the delay |
| BREAK | Tape stop effect |

### Tips
- Use CLK sync for tight rhythmic delays locked to your sequencer
- TAPE adds life and movement — even small amounts make a difference
- SURGE is great for build-ups and transitions
- Press SURGE for a surprising bloom effect
- Combine different HEAD settings with SPREAD for wide stereo echoes

---

## Impact

Impact is a powerful kick drum synthesizer with two distinct synthesis modes — Pure (additive synthesis) and Harsh (FM synthesis). It features a full set of controls for shaping the perfect kick drum, from deep sub hits to aggressive industrial punches.

**Pure mode** uses additive synthesis with up to 6 harmonics, giving a clean, musical kick sound. **Harsh mode** uses FM synthesis for more aggressive, metallic kick sounds.

The PUNCH parameter controls the pitch envelope — high values give a characteristic "click" at the start of the kick. MORPH shapes the waveform, while FOLD adds harmonic distortion. Three noise types (Dust, Crunch, Rumble) add different textural elements to the kick.

### Inputs
| Input | Description |
|-------|-------------|
| TRIG | Trigger input — fires the kick |
| V/OCT | Pitch CV (1V/oct) |
| PUNCH CV | Punch envelope modulation |
| MORPH CV | Waveform morph modulation |
| DECAY CV | Decay time modulation |
| NOISE CV | Noise tail length modulation |
| FOLD CV | Wavefolder modulation |
| ACC | Accent input — boosts volume and punch |

### Outputs
| Output | Description |
|--------|-------------|
| OUT L | Left stereo output |
| OUT R | Right stereo output |

### Parameters
| Parameter | Description |
|-----------|-------------|
| PITCH | Base frequency (30-100 Hz) |
| DECAY | Amplitude envelope decay time |
| PUNCH | Pitch envelope depth — higher = more click |
| MORPH | Waveform morph from sine to square |
| HARM | Harmonics amount (Pure mode) / FM ratio (Harsh mode) |
| FOLD | Wavefolder amount — adds harmonic distortion |
| NOISE | Noise amount mixed with the kick body |
| SPREAD | Frequency spread between harmonics |
| TAIL | Noise envelope decay time |
| DRUM | Switch between Pure and Harsh synthesis mode |
| NOISE TYPE | Dust (crackling), Crunch (crunchy), Rumble (low rumble) |
| TRY ME | Built-in trigger button for testing |

### Attenuverters
Each CV input has an attenuverter for precise modulation control. Negative values invert the modulation.

### Tips
- Start with PUNCH around 0.6 and DECAY around 0.4 for a classic kick
- Use ACC input from a sequencer for accented beats (808 style)
- FOLD adds grit — great for industrial and techno kicks
- Crunch noise type works well for clicky, transient-heavy kicks
- Try Harsh mode with high HARM values for metallic, industrial kicks

---

## Chain

Chain is a flexible 4-channel mixer and signal router. Each channel has an individual level control and mute button, making it easy to build submixes and route signals in your patch.

### Inputs
| Input | Description |
|-------|-------------|
| IN 1-4 | Audio inputs for each channel |
| CV 1-4 | Voltage controlled level for each channel |

### Outputs
| Output | Description |
|--------|-------------|
| MIX L | Stereo left mix output |
| MIX R | Stereo right mix output |

### Parameters
| Parameter | Description |
|-----------|-------------|
| LEVEL 1-4 | Individual channel level |
| MUTE 1-4 | Mute button per channel |

### Tips
- Use CV inputs with envelopes to create dynamic volume changes
- Chain multiple Chain modules for larger mixing setups
- Mute buttons are great for live performance — mute/unmute on the fly

---

## Squeeze

Squeeze is a compact sidechain envelope generator designed to work directly with Chain's COMP/CV insert. It takes a gate or audio signal and generates a control voltage envelope that drives the compressor inside Chain — perfect for classic ducking effects and rhythmic pumping.

The input is auto-detected: connect a gate for precise triggering or an audio signal like a kick drum for envelope following. Three contour curves (Linear, Exponential, Logarithmic) shape how the envelope responds, from snappy transients to smooth pumping.

### Inputs
| Input | Description |
|-------|-------------|
| GATE IN | Gate or audio input (auto-detect) |

### Outputs
| Output | Description |
|--------|-------------|
| COMP OUT | Envelope CV output — connect to Chain COMP/CV |

### Parameters
| Parameter | Description |
|-----------|-------------|
| ATTACK | Envelope attack time |
| RELEASE | Envelope release time |
| AMOUNT | Envelope depth |
| CONTOUR | Curve shape: Linear, Exponential, Logarithmic |

### Tips
- Connect a kick drum trigger to GATE IN and COMP OUT to Chain COMP/CV for classic sidechain ducking
- Use Exponential curve for punchy, fast-release pumping
- Use Logarithmic curve for smooth, musical compression
- One Squeeze can drive multiple Chain modules simultaneously

---

## Shape

Shape is a 6-band stereo equalizer based on the classic SSL console EQ. It covers the full frequency spectrum with high pass, low shelf, two parametric mid bands, high shelf and low pass.

### Inputs
| Input | Description |
|-------|-------------|
| IN L | Left stereo input |
| IN R | Right stereo input |

### Outputs
| Output | Description |
|--------|-------------|
| OUT L | Left stereo output |
| OUT R | Right stereo output |

### Parameters
| Parameter | Description |
|-----------|-------------|
| HIGH PASS | High pass filter cutoff frequency |
| LOW SHELF | Low frequency shelf boost/cut in dB |
| LOW MID | Low mid parametric band boost/cut |
| HIGH MID | High mid parametric band boost/cut |
| HIGH SHELF | High frequency shelf boost/cut in dB |
| LOW PASS | Low pass filter cutoff frequency |

### Tips
- Use HIGH PASS to remove low frequency rumble from pads and synths
- Boost HIGH SHELF for air and presence on vocals and leads
- Cut LOW MID around 300-500 Hz to reduce muddiness in a dense mix
- Place Shape after Chain for bus EQ on your mix

## Master

Master is a stereo master bus processor designed to sit at the end of your signal chain. It provides final level control with a built-in limiter to prevent clipping on the output.

### Inputs
| Input | Description |
|-------|-------------|
| IN L | Left stereo input |
| IN R | Right stereo input |

### Outputs
| Output | Description |
|--------|-------------|
| OUT L | Left stereo output |
| OUT R | Right stereo output |

### Parameters
| Parameter | Description |
|-----------|-------------|
| LEVEL | Master output level |
| LIMIT | Limiter ceiling — signals above this level are limited |

### Tips
- Set LIMIT to 0dB to prevent any clipping on the output
- Use LEVEL to adjust the overall loudness of your patch
- Always place Master at the very end of your signal chain before the audio interface

---

## Gain

Gain is a line level booster designed to bring iPhone, laptop and external gear up to modular level. Especially useful on the 4ms MetaModule where external inputs and sample players produce signals that are too quiet for the standard mixer level. Features soft clipping to prevent harsh distortion and an FX send for routing to effects processors.

### Inputs
| Input | Description |
|-------|-------------|
| IN | Low-level audio input |

### Outputs
| Output | Description |
|--------|-------------|
| OUT | Boosted audio output |

### Parameters
| Parameter | Description |
|-----------|-------------|
| GAIN | Boost amount |

### Tips
- Place Gain before Chain when using Loop or other sample players on the MetaModule
- Use it to match signal levels between different sources
- Do not use as a VCA as it has no CV input

---

## Sweep

Sweep is a state variable filter with simultaneous low pass, band pass, and high pass outputs. The resonance can be pushed into self-oscillation, turning Sweep into a sine wave oscillator.

### Inputs
| Input | Description |
|-------|-------------|
| IN | Audio input |
| CV | Cutoff frequency modulation |
| RES CV | Resonance modulation |

### Outputs
| Output | Description |
|--------|-------------|
| LP | Low pass output |
| BP | Band pass output |
| HP | High pass output |

### Parameters
| Parameter | Description |
|-----------|-------------|
| CUTOFF | Filter cutoff frequency |
| RES | Resonance — higher values emphasize the cutoff frequency |
| DRIVE | Input drive before the filter |

### Tips
- Use an envelope on CV for classic filter sweeps
- High RES values (near maximum) cause self-oscillation — use as a sine oscillator
- Drive adds warmth and harmonics before filtering
- BP output is great for isolating frequency bands in a mix
- Use HP to remove low frequencies from pads and synths to clean up the low end

---

## Loop

Loop is a sample loop player with a built-in waveform display. It loads WAV files into RAM for glitch-free playback, with full control over tempo, loop length, playback position, and direction.

What makes it interesting is that it automatically reads the BPM from the filename of your WAV file (e.g. 120bpm-myloop.wav) and calculates the bar length from there. When you plug in a clock, it adjusts the playback speed to stay perfectly in sync — no manual BPM entry needed.

For live use it has a CUE/LIVE switch with fade in and out, so you can preview a sample on your headphones before sending it to the main outputs. There is also a bar shift function that lets you jump to a different section of the sample quantized to the end of the loop, so it always lands cleanly on the beat.

The display shows the waveform of the loaded sample with a yellow playhead indicator showing the current position. The filename is shown at the top and BPM/bars information at the bottom.

**BPM auto-detection:** Loop reads the BPM directly from the filename (e.g. 120bpm-loop.wav). When a clock is connected it adjusts playback speed automatically to stay in sync.

**Bar shift:** Changes the loop start point without changing the loop length. With an 8-bar loop, setting SHIFT to 3 starts playback from bar 3 and loops back to bar 3. The transition always happens at the end of the current loop — perfect for live remixing.

**Reverse:** When REV is enabled, Loop waits until the end of the current bar before reversing direction. This keeps the reverse synchronized to the musical grid.

**CUE mode:** Routes audio to the CUE output with a smooth fade. Use it to preview loops on headphones before bringing them into the main mix.

### Inputs
| Input | Description |
|-------|-------------|
| CLK | Clock input for tempo sync |
| RESET | Reset playback to the loop start point |
| SPEED CV | Playback speed modulation |
| BARS CV | Loop length modulation |
| SHIFT CV | Bar shift (start position) modulation |
| BPM CV | BPM modulation |
| REV CV | Reverse trigger |

### Outputs
| Output | Description |
|--------|-------------|
| OUT L | Left stereo output |
| OUT R | Right stereo output |
| CUE | Cue output (active when CUE mode is on) |

### Parameters
| Parameter | Description |
|-----------|-------------|
| BARS | Number of bars to loop (0 = auto detect from BPM and file length) |
| SHIFT | Bar shift — offsets the loop start point |
| BPM | Tempo in BPM (0 = auto detect from filename) |
| SPEED | Playback speed — center is 1x, turn right for faster, left for slower |
| CLK | Enable/disable clock sync |
| REV | Enable/disable reverse playback (syncs to bar boundary) |
| CUE | Enable/disable cue mode |
| RESET | Reset playback to loop start |

### Display
| Element | Description |
|---------|-------------|
| Waveform | White waveform of the loaded sample |
| Playhead | Yellow vertical line showing current position |
| Filename | Name of the loaded file (top of display) |
| BPM/Bars | Tempo and loop length info (bottom of display) |

### Loading samples
Right-click the module and select **Load WAV** to open a file browser.

### Tips
- Name your files with BPM for automatic tempo detection: `120bpm-myloop.wav`
- Set BARS to 0 to let Loop calculate the number of bars automatically
- Use CLK sync for tight timing with your sequencer or drum machine
- SHIFT is great for creating variations — shift by 2 bars for a completely different feel
- REV adds interest — automate it with a sequencer for rhythmic reverse effects
- Use CUE to preview loops before switching — connect to a headphone mix
- Loop works great on the 4ms MetaModule with full display support
