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

Drift is a chaotic oscillator inspired by strange attractors and nonlinear dynamics. It generates complex, evolving waveforms that are never exactly the same twice, making it ideal for organic leads, drones, and experimental textures.

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

---

## Chrono

Chrono is a versatile clock divider and multiplier. It takes an incoming clock signal and generates multiple divided and multiplied versions, making it essential for polyrhythmic patterns and complex rhythmic structures.

A unique SURGE feature allows you to temporarily double the clock speed, adding rhythmic interest and variation to your patterns. The BREAK feature can stop specific outputs for fill effects.

### Inputs
| Input | Description |
|-------|-------------|
| CLK | Master clock input |
| RESET | Reset all divisions to start |

### Outputs
| Output | Description |
|--------|-------------|
| /2 | Clock divided by 2 |
| /4 | Clock divided by 4 |
| /8 | Clock divided by 8 |
| /16 | Clock divided by 16 |
| x2 | Clock multiplied by 2 |

### Parameters
| Parameter | Description |
|-----------|-------------|
| SURGE | Temporarily doubles clock speed |
| BREAK | Temporarily stops selected outputs |

### Tips
- Connect /4 to your kick and /8 to your hi-hat for classic drum patterns
- Use SURGE for build-ups and drops
- RESET at the start of a bar keeps everything tight and in sync

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

Squeeze is a dynamics processor combining compression and limiting. It reduces the dynamic range of audio signals, making them sit better in a mix and adding punch and glue to drum buses, synth pads, and full mixes.

The sidechain input allows external signals to trigger the compression, enabling classic ducking effects (e.g. a kick drum ducking a bassline).

### Inputs
| Input | Description |
|-------|-------------|
| IN L | Left audio input |
| IN R | Right audio input |
| SC | Sidechain input for external triggering |

### Outputs
| Output | Description |
|--------|-------------|
| OUT L | Left compressed output |
| OUT R | Right compressed output |

### Parameters
| Parameter | Description |
|-----------|-------------|
| THRESHOLD | Level at which compression starts |
| RATIO | Compression ratio (higher = more compression) |
| ATTACK | How quickly compression engages |
| RELEASE | How quickly compression releases |
| MAKEUP | Makeup gain to compensate for gain reduction |

### Tips
- Fast attack and slow release works well for drums
- Use the SC input with a kick drum to create pumping sidechain compression
- High ratio (8:1 or more) turns Squeeze into a limiter

---

## Shape

Shape is a waveshaper and distortion module that adds harmonic content and character to audio signals. From subtle saturation to extreme distortion, Shape can dramatically alter the timbre of any sound.

### Inputs
| Input | Description |
|-------|-------------|
| IN | Audio input |
| CV | Waveshaper amount CV |

### Outputs
| Output | Description |
|--------|-------------|
| OUT | Shaped audio output |

### Parameters
| Parameter | Description |
|-----------|-------------|
| SHAPE | Waveshaper curve — controls the type and amount of distortion |
| DRIVE | Input drive — boosts the signal before shaping |
| MIX | Dry/wet mix between clean and shaped signal |

### Tips
- Low SHAPE with high DRIVE gives warm saturation
- High SHAPE values create aggressive, clipping distortion
- Use MIX to blend in just the right amount of harmonic content
- Great on drums, basses, and synths for adding grit and presence

---

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

Gain is a simple but essential voltage controlled amplifier (VCA). It controls the amplitude of audio signals using either the knob or a CV input, making it ideal for envelopes, tremolo effects, and general level control.

### Inputs
| Input | Description |
|-------|-------------|
| IN | Audio input |
| CV | Gain control voltage (0-10V) |

### Outputs
| Output | Description |
|--------|-------------|
| OUT | Amplified or attenuated output |

### Parameters
| Parameter | Description |
|-----------|-------------|
| GAIN | Base gain level |

### Tips
- Connect an ADSR envelope to the CV input for classic VCA envelopes
- Use a slow LFO on CV for tremolo effects
- Stack multiple Gain modules for more complex amplitude shaping

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

The display shows the waveform of the loaded sample with a yellow playhead indicator showing the current position. The filename is shown at the top and BPM/bars information at the bottom.

**BPM auto-detection:** If the filename contains the BPM (e.g. `120bpm-loop.wav` or `loop-120BPM.wav`), Loop will automatically detect and set the tempo.

**Bar shift:** Changes the loop start point without changing the loop length. For example, with an 8-bar loop, setting SHIFT to 3 starts playback from bar 3 and loops back to bar 3 — perfect for remixing and rearranging loops.

**Reverse:** When REV is enabled, Loop waits until the end of the current bar before reversing direction. This keeps the reverse synchronized to the musical grid.

**CUE mode:** When CUE is active, the audio is routed to the CUE output instead of the main outputs. Useful for previewing loops before bringing them into the mix.

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
