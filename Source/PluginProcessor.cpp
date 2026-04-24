/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

KikAudioProcessor::KikAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                     )
#endif
{
    currentSource = sine;
    isPrepared = false;
}

KikAudioProcessor::~KikAudioProcessor()
{
}

const juce::String KikAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool KikAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool KikAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool KikAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double KikAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int KikAudioProcessor::getNumPrograms()
{
    return 1;
}

int KikAudioProcessor::getCurrentProgram()
{
    return 0;
}

void KikAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String KikAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void KikAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void KikAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    isPrepared = true;
}

void KikAudioProcessor::releaseResources()
{
    isPrepared = false;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool KikAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void KikAudioProcessor::generateKick (float* output, int outputSamples, double sampleRate)
{
    int kickLength = (int)(0.05 * sampleRate);
    if (kickLength > outputSamples) kickLength = outputSamples;
    
    float phase = 0.0f;
    float subPhase = 0.0f;
    float pitchEnv = pitch;
    float pitchEnd = pitch * 0.2f;
    float pitchDecayAmount = pitchDecay * 10.0f;
    float fadeInSamples = std::min (30.0f, (float)kickLength * 0.1f);
    float fadeOutStart = (float)kickLength * 0.85f;
    float clickPhase = 0.0f;

    for (int i = 0; i < kickLength; ++i)
    {
        float envPosition = (float)i / (float)kickLength;
        pitchEnv = pitch * std::exp (-pitchDecayAmount * envPosition);
        pitchEnv = std::max (pitchEnv, pitchEnd);
        float phaseIncrement = pitchEnv / (float)sampleRate;

        float ampEnv = 0.0f;
        float attackSamples = ampAttack * sampleRate;
        float decaySamples = ampDecay * sampleRate;
        float releaseSamples = ampRelease * sampleRate;

        if (i < 10)
            ampEnv = (float)i / 10.0f;
        else if (i < attackSamples + 10)
        {
            ampEnv = (float)(i - 10) / attackSamples;
            ampEnv = std::min (ampEnv, 1.0f);
        }
        else if (i < attackSamples + 10 + decaySamples)
        {
            float decayEnv = 1.0f - (float)(i - attackSamples - 10) / decaySamples;
            ampEnv = decayEnv + ampSustain * (1.0f - decayEnv);
        }
        else if (i >= kickLength - releaseSamples)
            ampEnv = ampSustain * (float)(kickLength - i) / releaseSamples;
        else
            ampEnv = ampSustain;

        float sampleVal = 0.0f;
        float fundamental = std::sin (phase * 2.0f * juce::MathConstants<float>::pi);

        switch (currentSource)
        {
            case sine: sampleVal = fundamental; break;
            case triangle: sampleVal = 2.0f * std::abs (2.0f * (phase - std::floor (phase + 0.5f))) - 1.0f; break;
            case saw: sampleVal = 2.0f * (phase - std::floor (phase + 0.5f)); break;
            case square: sampleVal = phase < 0.5f ? 1.0f : -1.0f; break;
            case loaded:
            {
                int wSamples = 0;
                bool hasWave = false;
                { juce::ScopedLock lock (loadedWaveformLock); hasWave = hasLoadedWaveform; wSamples = loadedWaveform.getNumSamples(); }
                if (hasWave && wSamples > 0)
                {
                    int sampleIdx = std::min ((int)((float)i / kickLength * wSamples), wSamples - 1);
                    { juce::ScopedLock lock (loadedWaveformLock); sampleVal = loadedWaveform.getSample (0, sampleIdx); }
                }
                else sampleVal = fundamental;
                break;
            }
        }

        float harmonic2 = std::sin (phase * 4.0f * juce::MathConstants<float>::pi);
        float harmonic3 = std::sin (phase * 6.0f * juce::MathConstants<float>::pi);
        float harmonic4 = std::sin (phase * 8.0f * juce::MathConstants<float>::pi);
        sampleVal = sampleVal * (1.0f - color * 0.7f) + harmonic2 * color * 0.3f + harmonic3 * color * color * 0.15f + harmonic4 * color * color * color * 0.1f;

        float subOsc = std::sin (subPhase * 2.0f * juce::MathConstants<float>::pi);
        sampleVal = sampleVal + subOsc * depth * 0.3f;

        float fadeIn = (i < fadeInSamples) ? (float)i / fadeInSamples : 1.0f;
        sampleVal *= fadeIn;

        float fadeOut = (i > fadeOutStart) ? 1.0f - (float)(i - fadeOutStart) / (kickLength - fadeOutStart) : 1.0f;
        sampleVal *= fadeOut;

        float clickVal = 0.0f;
        float clickDuration = (float)kickLength * 0.02f;
        if (click > 0.0f && i < clickDuration)
        {
            float clickEnv = 1.0f - (float)i / clickDuration;
            float clickFreq = (1500.0f + 2500.0f * clickEnv) / (float)sampleRate;
            clickPhase += clickFreq;
            if (clickPhase >= 1.0f) clickPhase -= 1.0f;
            clickVal = click * std::sin (clickPhase * 2.0f * juce::MathConstants<float>::pi) * clickEnv * 0.5f;
        }

        sampleVal = sampleVal * drive + clickVal;
        output[i] = sampleVal * ampEnv * gain;
        
        if (std::abs (output[i]) > peakLevel) peakLevel = std::abs (output[i]);

        phase += phaseIncrement;
        if (phase >= 1.0f) phase -= 1.0f;
        subPhase += phaseIncrement * 0.5f;
        if (subPhase >= 1.0f) subPhase -= 1.0f;
    }

    for (int i = kickLength; i < outputSamples; ++i) output[i] = 0.0f;
}

void KikAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (!isPrepared) return;
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0) return;
    auto sampleRate = getSampleRate();
    if (sampleRate <= 0) return;

    peakLevel *= 0.95f;
    
    auto triggerInterval = (int)(60.0 / bpm * sampleRate);
    
    if (loopEnabled)
    {
        samplesSinceTrigger += numSamples;
        if (samplesSinceTrigger >= triggerInterval)
        {
            samplesSinceTrigger = 0;
            shouldTrigger = true;
        }
    }
    
    if (!shouldTrigger) return;
    shouldTrigger = false;
    
    {
        juce::HeapBlock<float> tempBlock (numSamples);
        generateKick (tempBlock.get(), numSamples, sampleRate);
        for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer (channel);
            for (int i = 0; i < numSamples; ++i) channelData[i] = tempBlock[i];
        }
    }
}

bool KikAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* KikAudioProcessor::createEditor() { return new KikAudioProcessorEditor (*this); }
void KikAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {}
void KikAudioProcessor::setStateInformation (const void* data, int sizeInBytes) { juce::ignoreUnused (data, sizeInBytes); }
void KikAudioProcessor::updatePreview() { previewWaveform.resize (2048); generateKick (previewWaveform.data(), 2048, 44100.0); previewDirty = false; }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KikAudioProcessor(); }