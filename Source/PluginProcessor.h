/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>

class KikAudioProcessor  : public juce::AudioProcessor
{
public:
    KikAudioProcessor();
    ~KikAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    enum WaveformSource { sine, triangle, saw, square, loaded };
    
    WaveformSource currentSource = sine;
    
    float peakLevel = 0.0f;
    
    bool shouldTrigger = false;
    int samplesSinceTrigger = 0;
    
    std::vector<float> previewWaveform;
    bool previewDirty = true;
    void updatePreview();
    
    void loadPreset(int index);

    /** Sets Start pitch (Hz) from Key + Octave parameters (A4 = 440 Hz). */
    void applyPitchFromKeyNote();

    /** Updates Key + Octave from current Start pitch (nearest semitone). */
    void syncKeyNoteFromPitch();

    /** Bumped when factory presets or full state load changes parameters — editor polls to refresh Web UI. */
    uint32_t getPresetStateVersion() const noexcept { return presetStateVersion.load (std::memory_order_relaxed); }

    /** Active built-in preset index, or -1 when parameters came from host/project/file load. */
    int getFactoryPresetIndexForUi() const noexcept { return factoryPresetIndex.load (std::memory_order_relaxed); }

    /** Shown in the UI preset control (factory title, file/session label, etc.). */
    juce::String getPresetDisplayName() const;
    void setPresetDisplayName (juce::String name);

    std::atomic<bool> midiTriggered { false };
    
    juce::AudioProcessorValueTreeState apvts;
    
private:
    void generatePreview (float* output, int numSamples, double sampleRate);
    double polyBlep (double t, double dt);

    juce::AudioBuffer<float> loadedWaveform;
    bool hasLoadedWaveform = false;
    juce::CriticalSection loadedWaveformLock;

    bool isPrepared = false;

    std::atomic<uint32_t> presetStateVersion { 0 };
    std::atomic<int> factoryPresetIndex { -1 };

    mutable juce::CriticalSection presetDisplayLock;
    juce::String presetDisplayName { "Default" };

    // DSP State Variables
    bool isPlaying = false;
    double currentPhase = 0.0;
    double subPhase = 0.0;
    double clickPhase = 0.0;
    int currentSampleIndex = 0;

    juce::dsp::IIR::Filter<float> hpToneFilter, lpToneFilter;
    float lastHpCutoffHz = -1.0f;
    float lastLpCutoffHz = -1.0f;
    
    // Oversampling state
    int oversampleMode = 0; // 0=1x, 1=2x, 2=4x
    int currentOversampleIndex = 0;
    bool oversamplerPrepared = false;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KikAudioProcessor)
};