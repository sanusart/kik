# Kik Kick Synthesizer - AI Agent Guide

A JUCE-based VST3/AU audio plugin for synthesizing kicks and bass drums with a web-based UI.

## Architecture Overview

### Core Stack
- **DSP Engine**: `PluginProcessor.cpp` - Real-time audio synthesis with JUCE DSP module
- **Parameter Management**: `AudioProcessorValueTreeState` (APVTS) - All plugin parameters
- **UI Layer**: `PluginEditor.cpp` + `Resources/ui/index.html` - Native WebBrowserComponent hosting HTML/CSS/JS
- **Build System**: JUCE Projucer (`.jucer` file) → Xcode project → macOS plugin binary

### Plugin Characteristics
- **Type**: Synthesizer (pluginIsSynth=true, WantsMidiInput=true)
- **Format**: VST3 (Category: Drum) + AU
- **Audio Format**: Stereo out, 1x/2x/4x oversampling support
- **Manufacturer**: Sanus artificium (pluginManufacturerCode="Sanu")

## Essential Developer Workflows

### Building the Plugin
```bash
# 1. Open kik.jucer in JUCE Projucer
# 2. Set JUCE module path: expects JUCE at ../../../JUCE (relative to kik dir)
# 3. Export to Xcode: Projucer generates Builds/MacOSX/kik.xcodeproj
# 4. Build in Xcode: Product → Build or Scheme selector
```

**Key Build Details**:
- JUCE expects modules at `../../JUCE/modules` (relative to project root)
- Web UI resources (`Resources/ui/`) are embedded at build time (`resource="1"` in .jucer)
- Debug/Release configs available; Release enables binary copy/plugin installation

### Real-Time Audio Processing
**Critical Constraint**: All audio runs in `processBlock()` with real-time deadlock safety:
- Use atomic types (`std::atomic<>`) for lock-free parameter reads from UI thread
- Use `CriticalSection` + `ScopedLock` ONLY for non-realtime data (preset names, loaded waveforms)
- Example: `float pitch = *apvts.getRawParameterValue("pitch");` is atomic-safe

### Web UI Communication
The native `WebBrowserComponent` bridge allows bidirectional C++↔JavaScript events:

```cpp
// C++ listens for Web events (Editor::timerCallback at 20Hz sends updates)
browser->emitEventIfBrowserIsVisible("parameterUpdate", juce::var obj);

// Web listens via native integration
window.addEventListener("parameterUpdate", (evt) => { /* update UI */ });

// Web → C++ events fired via:
window.webkit.messageHandlers.parameterChange.postMessage({param: "pitch", value: 100});
```

**Event Flow**:
1. UI loaded → `uiReady` event triggers `sendInitValuesToWeb()` with all parameter values
2. User adjusts slider in Web UI → `parameterChange` event → `onParameterChange()` calls APVTS setter
3. DSP changes (meter updates, waveform preview) → 20Hz timer polls and sends updates back to Web

## Project-Specific Conventions

### Parameter Definition & Access
All parameters defined in `createParameterLayout()` (PluginProcessor.cpp:39). Three access patterns:

```cpp
// 1. Fast atomic read (real-time safe)
float pitch = *apvts.getRawParameterValue("pitch");

// 2. Type-cast for choice/int parameters
int waveform = (int)*apvts.getRawParameterValue("waveform");

// 3. Value-setting with host notification (UI/non-realtime)
if (auto* p = apvts.getParameter("keyNote"))
    if (auto* pi = dynamic_cast<juce::AudioParameterInt*>(p))
        pi->setValueNotifyingHost(pi->convertTo0to1(noteValue));
```

### Range Enforcement Pattern
Strictly enforce min/max with `jlimit()`:
```cpp
lpCut = jlimit(800.0f, 20000.0f, lpCut);  // Seen throughout DSP code
```
This prevents filter coefficient bugs and ensures DSP stability within tested ranges.

### Waveform Synthesis & Anti-Aliasing
Band-limited waveforms use **polyBLEP** discontinuity attenuation:
- `sine`: Direct phase modulation
- `triangle/square/saw`: Use `polyBlep(phase, phaseIncrement)` to smooth band-edge artifacts
- Example: `saw_with_polyblep = (2.0 * phase - 1.0) - polyBlep(phase, dt)`

### Pitch Envelope Math
Two-part pitch decay: exponential sweep with floor
```cpp
double pitchEnv = pitch * exp(-decayRate * time);  // Falls from pitch → 20% of pitch
pitchEnv = std::max(pitchEnv, pitch * 0.2);      // Clamp minimum
```

### Preview Waveform Generation
`generatePreview()` (PluginProcessor.cpp:294) mirrors `processBlock()` DSP but:
- Fixed 500ms max length at 44100 Hz (independent of current sample rate)
- Called when parameters change OR `previewDirty = true`
- Output stored in `previewWaveform` vector, sent to Web UI every 50ms (20Hz timer)

### Preset System
**Factory Presets**: 13 hardcoded entries in `FactoryPresetData::entries[]`:
- Index 0 = "Default" (plugin defaults, kept in sync with `createParameterLayout()`)
- Indices 1-12 = named mixes (Sub Layer, Club Kick, etc.)
- Each entry holds all parameter values; use `loadPreset(index)` to apply

**User Presets**: Stored as `.kik` files in `~/Documents/KIK/Presets/`
- `getStateInformation()/setStateInformation()` handles serialization via APVTS XML
- `saveStateToFile()/loadStateToFile()` UI event handlers manage file I/O
- Preset display name tracked separately (non-atomic with `CriticalSection` lock)

### Timer & Update Polling
Editor runs a 20Hz timer (`startTimerHz(20)`) in `timerCallback()`:
- Detects preset state changes → updates Web UI if needed
- Polls peak meter level (`audioProcessor.peakLevel`) → sends to Web
- Regenerates preview waveform if `previewDirty` flag set
- Sends meter + waveform to Web only when visible (`emitEventIfBrowserIsVisible()`)

## Critical Files & Patterns

| File | Responsibility |
|------|-----------------|
| `PluginProcessor.cpp` | DSP: waveform synthesis, ADSR envelope, filters, drive/color saturation, preview generation, preset loading |
| `PluginEditor.cpp` | Web UI bridge: event listeners, parameter set/get, meter/waveform polling, file I/O for presets |
| `Resources/ui/index.html` | Web UI layout (820×520px, see `KikEditorLayout` namespace) |
| `Resources/ui/ui.js` | Web component logic: slider binding, meter rendering, waveform visualization |
| `kik.jucer` | Project config: plugin type, JUCE module paths, export settings |

## Integration Points & Dependencies

### JUCE Module Dependencies
- `juce_audio_processors` - AudioProcessor base, APVTS
- `juce_dsp` - IIR filters, Oversampling, ProcessSpec/AudioBlock
- `juce_gui_extra` - WebBrowserComponent (native rendering engine)
- `juce_core` - Atomic, File, XML, JSON utilities
- Others: audio_basics, audio_devices, audio_formats, audio_utils, events, graphics, gui_basics, data_structures, cryptography

### External Build Requirements
- macOS + Xcode (minimum version specified in .jucer)
- JUCE framework (version 8.x or configured in .jucer)
- No third-party audio libraries (DSP entirely custom or JUCE)

### Key Data Flows
1. **Audio**: Plugin host sends MidiBuffer → MIDI note-on triggers `shouldTrigger = true` → DSP processes output buffer
2. **Presets**: User clicks preset → Web JS posts `parameterChange` events → C++ updates APVTS → DSP reads new values
3. **Monitoring**: DSP updates `peakLevel` → Editor timer reads → sends to Web → Web meter updates
4. **State Save/Load**: Host queries DSP state → XML serialization → file I/O or plugin state embed

## Common Modification Areas

- **Add DSP Effect**: Modify `processBlock()` signal chain; add APVTS parameter in `createParameterLayout()`
- **Update UI**: Edit `Resources/ui/index.html` + `ui.js`; ensure sliders emit `parameterChange` events with correct param IDs
- **Add Preset**: Insert entry in `FactoryPresetData::entries[]` array with all parameter values
- **Change Audio Latency**: Adjust `getTailLengthSeconds()` if adding processors with latency
- **Oversampling Tuning**: Adjust factor in `prepareToPlay()` based on sample rate (1x/2x/4x choices stored in APVTS)

## Debugging & Testing Tips

- **CPU Profile**: Check real-time performance in host DAW; compare 1x vs 4x oversampling modes
- **Web UI Debugging**: Use `Logger::writeToLog()` in C++ and browser dev tools (if native WebBrowserComponent permits)
- **Parameter Sync**: Monitor APVTS state changes with breakpoints in `onParameterChange()`
- **Peak Meter**: Watch `audioProcessor.peakLevel` in debugger to catch clipping/saturation issues
- **Preset Persistence**: Manually inspect `.kik` XML files in `~/Documents/KIK/Presets/` for correctness

