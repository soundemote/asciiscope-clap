# asciiscope-clap

https://mit-license.org/
```rust
// project version 0.1.0
// Soundemote signal visuals inside CLAP/JUCE hosts
// plugin window now, live synth visuals next

plugin identity : Asciiscope CLAP
foundation      : baconpaul sidequest-startingpoint
format path     : CLAP first, VST3/AUv2 wrappers from clap-wrapper
visual goal     : dark colorful console-style signal art
next step       : JUCE visual component fed by demo/VU-derived data
```
![example.gif](https://github.com/soundemote/asciiscope/blob/main/example.gif)

## what is this?

Asciiscope CLAP is the plugin-side path for Asciiscope.

The terminal app stays independent and terminal-first. This repo adapts the
same signal-visual idea into a plugin editor that can eventually react to synths,
tracks, buses, and host automation.

```text
asciiscope       -> terminal-native visual instrument
asciiscope-clap  -> plugin editor / host input path
soemdsp          -> future shared DSP and signal concepts
```

## current status

This repo is still intentionally close to the startingpoint architecture.

```markdown
done  : visible product/plugin identity rename to Asciiscope CLAP
done  : tiny JUCE visual component with demo/VU-derived motion
done  : audio-to-editor visual snapshot boundary
done  : rolling UI-side waveform history for block snapshots
done  : simple Scope Mode and Palette plugin parameters
kept  : existing engine/editor/classes/namespace shape
next  : richer snapshot-fed modes
later : Asciiscope scene/render adapter
```

## build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The project currently builds CLAP, VST3, AUv2 wrapper targets, and a standalone
target through the existing startingpoint/clap-wrapper setup.

## architecture notes

Preserved for now:

```markdown
baconpaul::sidequest_ns namespace
clapimpl::SideQuest<multiOut>
ui::PluginEditor
ui::MainPanel
60 Hz editor IdleTimer
existing scalar audio/UI queue
current audio port shape
```

Near-term insertion point:

```text
Engine::process()
    -> lightweight visual snapshot, published with non-blocking try_lock
    -> PluginEditor::idle()
    -> AsciiscopeVisualComponent::repaint()
    -> AsciiscopeVisualComponent::paint(juce::Graphics&)
```

Do not make the terminal app become a plugin project. The plugin should adapt to
Asciiscope concepts.

`IVisualSurface` is still deferred. The current priority is learning what the
JUCE editor needs from snapshot-fed drawing before shaping a shared surface API.

## future

```rust
AsciiscopeVisualComponent
// small JUCE component with the dark console look

DemoVisualInput
// non-audio first visual feed for proving editor rendering

AudioVisualSnapshot
// lock-safe handoff from audio thread to editor thread, now present in minimal form

RollingWaveformHistory
// UI-side fixed buffer that turns small audio blocks into a visible trace

SignalFrameAdapter
// maps plugin data toward Asciiscope SignalFrame concepts

LiveScopeMode
// oscilloscope / spectral / particle modes driven by real synth signals
```
