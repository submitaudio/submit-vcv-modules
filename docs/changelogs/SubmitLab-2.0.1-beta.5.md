# Submit Lab 2.0.1 Beta 5

Submit Lab is the public beta channel for experimental Submit Audio modules.
It installs alongside the normal Submit plugin and does not replace existing Submit modules.

## Included modules

- **Circles** - A circular eight-step sequencer with musical flow modes, four scale-chain slots and a slower dedicated sub track.
- **SUB** - A monophonic bass voice with an analog-style upper layer, a deep octave-down sub layer, warm drive and accent response.

No other Submit Audio modules are included in this beta.

## Beta 5 changes

### Circles

- Improved the Random flow so it cannot immediately return to the previously played step.
- Random still distributes its choices evenly across the remaining available steps.
- This removes short A-B-A ping-pong patterns while keeping Drunk completely unchanged.

### SUB

- No DSP changes in this beta. SUB remains included as the matching Submit Lab voice.

## Downloads

Choose the package matching your platform:

- `mac-arm64`: Apple Silicon Macs
- `win-x64`: 64-bit Windows
- `lin-x64`: 64-bit Linux

Quit VCV Rack before replacing the previous Submit Lab package, then restart Rack and search for the **Submit Lab** brand.

## Beta notice

This is test software. Sounds, behaviour and presentation may still change.
Please keep important patches backed up.

## Feedback and links

- [Development discussion and sound feedback](https://community.vcvrack.com/t/submit-audio-development-updates-for-vcv-rack-and-metamodule/26001)
- [Report a reproducible beta bug](https://github.com/submitaudio/submit-vcv-modules/issues)
- [Submit Audio website](https://submitaudio.nl)
