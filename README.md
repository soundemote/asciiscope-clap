# ⋆⁺₊✧ asciiscope ✧₊⁺⋆

https://mit-license.org/

```rust
// project version 0.1.0
// Soundemote terminal-native signal visuals
// real console output > fake mockups

📺 console renderer : dark terminal phosphor, colored glyph trails
〰️ signal frames    : renderer-neutral visual input
🌀 demo signals     : soemdsp phasors, attractors, envelopes, noise
🌃 visual modes     : bloom, tunnel, particle field
🎛️ future input     : synths, plugins, sandbox probes, live signal buses
```

![example.gif](https://github.com/soundemote/asciiscope/blob/main/example.gif)


Windows Run command:

```text
cmd /k "cd /d C:\Users\argit\Desktop\asciiscope && build\Release\asciiscope.exe"
```

## live controls

```markdown
1 2 3        lock bloom / tunnel / particle mode
0            return to automatic mode rotation
Space        pause or resume
+ -          speed up or slow down
Up Down      speed up or slow down
[ ]          reduce or increase signal density
Left Right   reduce or increase signal density
Mouse wheel  zoom without clearing current trails
z Z          keyboard zoom out / in
< >          shorter or longer trails
c            toggle color
r or x       clear trails
h or ?       show control help
q or Esc     quit
```

## what is this?

Asciiscope is a dark colorful console instrument for signal art.

It turns DSP motion into terminal-native oscilloscope visuals: glowing ASCII
trails, waveform tunnels, chaotic attractor blooms, particle fields, envelope
flashes, and spectral-looking text graphics that can be recorded straight from
Windows Terminal.

This is the Soundemote attention layer:

```markdown
soemdsp          -> math, signal, dsp objects
asciiscope       -> terminal-native visual instrument
asciiscope-clap  -> future plugin window / synth input path
```

## visual identity

```rust
black screen
cyan voltage
violet decay
white-hot transients
blue low-energy ghosts
monospace motion
signal trails that feel like hardware waking up in the dark
```

The console is not a placeholder. The console is the look.

## files

```markdown
### `include/asciiscope/SignalInput.hpp`
neutral visual input boundary
SignalFrame, SignalSource, SignalSample, SignalKind, ISignalInput

### `include/asciiscope/DemoSignalInput.hpp`
generated signal producer using soemdsp objects
currently feeds the standalone demo

### `include/asciiscope/AnimationScene.hpp`
visual scene modes
consumes SignalFrame data, not sandbox/runtime internals

### `include/asciiscope/ConsoleRenderer.hpp`
terminal framebuffer, glyph decay, ANSI fallback, Windows console buffer output

### `src/main.cpp`
demo loop, live controls, frame timing
```

## signal path

```text
DemoSignalInput
    -> SignalFrame
    -> AnimationScene
    -> ConsoleRenderer
    -> Windows Terminal / cmd
```

Future signal path:

```text
soemdsp-sandbox VisualizationBus
    -> SandboxSignalInput
    -> SignalFrame
    -> AnimationScene
    -> ConsoleRenderer
```

Asciiscope should stay independent. It should not become a sandbox client.
It should become a signal-frame visual instrument that a future adapter can feed.

## current modes

```rust
multi-sprott bloom
// chaotic attractor trace, rotating slowly through terminal space

phasor wave tunnel
// circular waveform tunnel, pulse and LFO modulated

pluck/noise particle field
// envelope flashes, noise drift, orbiting signal dust
```

## soemdsp objects currently used

```cpp
soemdsp::oscillator::Phasor
soemdsp::oscillator::MultiSprottC
soemdsp::oscillator::Thomas
soemdsp::utility::NoiseGenerator
soemdsp::modulator::LinearDASR
soemdsp::filter::LinearSmoother
soemdsp::math
```

## dependency

`soemdsp` lives at:

```text
libs/soemdsp
```

This checkout is configured for the local Soundemote source repo:

```powershell
git submodule update --init --recursive
```

If the submodule needs to be reattached:

```powershell
git submodule set-url libs/soemdsp C:/Users/argit/Desktop/soemdsp
git submodule update --init --recursive
```

## build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## run

```powershell
.\build\Release\asciiscope.exe
```

Options:

```powershell
.\build\Release\asciiscope.exe --no-color
.\build\Release\asciiscope.exe --once
```

The footer shows the current mode, speed, density, zoom, trail amount, color
state, pause state, and last adjusted control.

## future

```rust
AudioSignalInput
// live synth or audio interface input

SandboxSignalInput
// consumes future soemdsp-sandbox visualization/probe frames

PluginRenderer
// asciiscope-clap draws the console look inside a plugin editor

WaveformScope
// honest oscilloscope mode for real synth signals

SpectralConsole
// dark colorful pseudo-spectrum made from real audio features
```

## philosophy

```markdown
* real executable output
* terminal visuals first
* visible motion over architecture theater
* dark console, bright signal
* generated demos now, live synths next
* keep the visual core independent
```
