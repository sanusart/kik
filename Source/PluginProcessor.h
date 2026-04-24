/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

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
    float pitch = 200.0f;
    float pitchDecay = 0.23f;

    float ampAttack = 0.001f;
    float ampDecay = 0.3f;
    float ampSustain = 0.0f;
    float ampRelease = 0.2f;

    float drive = 1.0f;
    float click = 0.0f;
    float color = 0.5f;
    float depth = 0.5f;
    float gain = 1.0f;
    
    float peakLevel = 0.0f;
    
    bool shouldTrigger = false;
    bool loopEnabled = false;
    int samplesSinceTrigger = 0;
    float bpm = 120.0f;
    
    std::vector<float> previewWaveform;
    bool previewDirty = true;
    void updatePreview();
    
private:
    void generateKick (float* output, int numSamples, double sampleRate);

    juce::AudioBuffer<float> loadedWaveform;
    bool hasLoadedWaveform = false;
    juce::CriticalSection loadedWaveformLock;

    bool isPrepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KikAudioProcessor)
};