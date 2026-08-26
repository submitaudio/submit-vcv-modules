# Submit Lab 2.0.1 Beta 3

Submit Lab is the public beta channel for experimental Submit Audio modules.
It installs alongside the normal Submit plugin and does not replace existing
Submit modules.

## Included modules

- **Circles** - An always-quantized circular eight-step sequencer with musical
  flow modes, four scale-chain slots and a slower dedicated sub track.
- **SUB** - A monophonic bass voice with an analog-style upper layer, a deep
  octave-down sub layer, warm drive, accent response and six CV modulation
  inputs.

No other Submit Audio modules are included in this beta.

## Beta 3 changes

### Circles

- Added a dedicated **Dice Trigger** input for generative patches.
- Dice changes are prepared immediately, while the current melody keeps playing.
- The pending Dice light remains on for eight subsequent sequencer advances.
- After those eight advances, the new melody starts on the following step.
- The eight-step countdown follows the sequencer at every SPEED setting and also
  works consistently with Forward, Reverse, Pendulum, Drunk and Random flow.
- The transpose input tooltip now describes its root-transpose role more clearly.
- When an external clock stops, Trigger, Gate, EOC and Sub Gate are now closed
  after the expected clock timeout. Pitch outputs continue holding their last
  stable voltage.

### SUB

- The SUB synth voice and sound engine are unchanged in this update.

## Patch compatibility

The plugin identity and existing module, parameter, input, output and light IDs
remain unchanged. The new Dice Trigger input is appended to the Circles beta
layout. Submit Lab Beta 1 and Beta 2 patches remain readable.

## Downloads

Choose the package matching your platform:

- `mac-arm64` - Apple Silicon Macs
- `win-x64` - 64-bit Windows
- `lin-x64` - 64-bit Linux

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
