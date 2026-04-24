# Kik - Kick Synthesizer

A JUCE-based VST3/AU plugin for synthesizing kicks and bass drums.

## Features

- **Waveform Selection**: Sine, Triangle, Saw, Square, Loaded sample
- **Pitch**: Start frequency + decay rate
- **Amplitude Envelope**: ADSR controls
- **Tone Controls**:
  - **Drive**: Amplitude multiplier
  - **Color**: Harmonics (2x, 3x, 4x)
- **Transient**:
  - **Click**: High-frequency transient
  - **Depth**: Sub-bass at half pitch
- **Output**: Gain + peak meter
- **Playback**: Manual trigger or LOOP at BPM (80-250)

## Building

1. Open `kik.jucer` in Projucer
2. Set JUCE module path (expects JUCE at `../../../JUCE`)
3. Export to Xcode and build

## Requirements

- JUCE 8.x (or your preferred version)
- Xcode for macOS

## Controls

| Control | Range | Description |
|---------|-------|-------------|
| Pitch | 40-250 Hz | Starting frequency |
| Decay | 0.01-0.3 | Pitch decay rate |
| Attack | 0-0.05s | Amp attack time |
| Decay | 0.05-1.0s | Amp decay time |
| Sustain | 0-1 | Amp sustain level |
| Release | 0.05-1.0s | Amp release time |
| Drive | 0.5-3 | Amplitude multiplier |
| Color | 0-1 | Harmonics intensity |
| Click | 0-1 | Transient amount |
| Depth | 0-1 | Sub-bass amount |
| Gain | 0-1.5 | Output gain |
| BPM | 80-250 | Loop tempo |