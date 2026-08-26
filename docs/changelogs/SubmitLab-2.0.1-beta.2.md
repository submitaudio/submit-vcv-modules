# Submit Lab 2.0.1 Beta 2

Submit Lab is the public beta channel for experimental Submit Audio modules.
It installs alongside the normal Submit plugin and does not replace existing
Submit modules.

## Included modules

- **Circles** -- An always-quantized circular eight-step sequencer with musical
  flow modes, four scale-chain slots and a slower dedicated sub track.
- **SUB** -- A monophonic bass voice with an analog-style upper layer, a deep
  octave-down sub layer, warm drive, accent response and six CV modulation
  inputs.

No other Submit Audio modules are included in this beta.

## Beta 2 changes

### Circles

- Added a stored **Clock input rate** context setting.
- **1 PPQN (Submit standard)** remains the default for new and existing patches.
- **4 PPQN (compatibility)** follows common external clock sources while keeping
  the displayed BPM, sequence divisions, bar timing, Scale Chain and sub track
  at the intended musical speed.
- The panel **GATE** control now shapes both the main sequencer gate and the slow
  `SUB GATE`.
- `SUB BARS` now controls the interval between sub notes without automatically
  stretching the gate across most of that multi-bar interval. Short pulses and
  long held bass notes remain available directly from the GATE control.

### SUB

- The SUB synth voice and sound engine are unchanged in this update.

## Patch compatibility

The plugin identity, module slugs and parameter, input, output and light IDs are
unchanged. Submit Lab 2.0.0 Beta 1 patches remain readable. Patches without the
new clock setting open in the 1 PPQN Submit standard.

## Downloads

Choose the package matching your platform:

- `mac-arm64` -- Apple Silicon Macs
- `win-x64` -- 64-bit Windows
- `lin-x64` -- 64-bit Linux

Quit VCV Rack before replacing the previous Submit Lab package, then restart
Rack and search for the **Submit Lab** brand.

## Beta notice

This is test software. Sounds, behaviour and presentation may still change.
Please keep important patches backed up. Submit Lab patches use the separate
`SubmitLab` plugin identity and therefore remain distinct from normal Submit
patches.

## Feedback and links

- [Development discussion and sound feedback](https://community.vcvrack.com/t/submit-audio-development-updates-for-vcv-rack-and-metamodule/26001)
- [Report a reproducible beta bug](https://github.com/submitaudio/submit-vcv-modules/issues)
- [Submit Audio website](https://submitaudio.nl)
- [Circles and SUB video preview](https://youtu.be/q0UwvWin3SY)
