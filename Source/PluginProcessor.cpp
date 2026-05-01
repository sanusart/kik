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

double KikAudioProcessor::polyBlep(double t, double dt)
{
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0;
    }
    else if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

void KikAudioProcessor::generatePreview (float* output, int outputSamples, double sampleRate)
{
    int kickLength = (int)(0.5 * sampleRate); // 500ms preview max
    if (kickLength > outputSamples) kickLength = outputSamples;
    
    double pPhase = 0.0;
    double pSubPhase = 0.0;
    double pClickPhase = 0.0;
    double pPitchEnv = pitch;
    double pitchEnd = pitch * 0.2;
    double pitchDecayAmount = pitchDecay * 10.0;

    for (int i = 0; i < kickLength; ++i)
    {
        double timeInSeconds = (double)i / sampleRate;
        pPitchEnv = pitch * std::exp (-pitchDecayAmount * timeInSeconds * 10.0);
        pPitchEnv = std::max (pPitchEnv, pitchEnd);
        double phaseIncrement = pPitchEnv / sampleRate;

        double ampEnv = 0.0;
        double attackSecs = std::max(0.001f, ampAttack);
        double decaySecs = std::max(0.001f, ampDecay);
        double releaseSecs = std::max(0.001f, ampRelease);

        if (timeInSeconds < attackSecs)
            ampEnv = timeInSeconds / attackSecs;
        else if (timeInSeconds < attackSecs + decaySecs)
        {
            double decayEnv = 1.0 - (timeInSeconds - attackSecs) / decaySecs;
            ampEnv = decayEnv + ampSustain * (1.0 - decayEnv);
        }
        else if (timeInSeconds < attackSecs + decaySecs + 0.1) // Fake 100ms hold for preview
            ampEnv = ampSustain;
        else
            ampEnv = ampSustain * std::max(0.0, 1.0 - (timeInSeconds - (attackSecs + decaySecs + 0.1)) / releaseSecs);

        double sampleVal = 0.0;
        double fundamental = std::sin (pPhase * 2.0 * juce::MathConstants<double>::pi);

        switch (currentSource)
        {
            case sine: 
                sampleVal = fundamental; 
                break;
            case triangle: 
            {
                sampleVal = 2.0 * std::abs (2.0 * (pPhase - std::floor (pPhase + 0.5))) - 1.0; 
                // PolyBLAMP not fully needed for preview, standard triangle is okay
                break;
            }
            case saw: 
                sampleVal = 2.0 * pPhase - 1.0;
                sampleVal -= polyBlep(pPhase, phaseIncrement);
                break;
            case square: 
                sampleVal = pPhase < 0.5 ? 1.0 : -1.0;
                sampleVal += polyBlep(pPhase, phaseIncrement);
                sampleVal -= polyBlep(std::fmod(pPhase + 0.5, 1.0), phaseIncrement);
                break;
            case loaded:
                sampleVal = fundamental;
                break;
        }

        double harmonic2 = std::sin (pPhase * 4.0 * juce::MathConstants<double>::pi);
        double harmonic3 = std::sin (pPhase * 6.0 * juce::MathConstants<double>::pi);
        sampleVal = sampleVal * (1.0 - color * 0.7) + harmonic2 * color * 0.5 + harmonic3 * color * color * 0.2;

        double subOsc = std::sin (pSubPhase * 2.0 * juce::MathConstants<double>::pi);
        sampleVal += subOsc * depth * 0.5;

        double clickVal = 0.0;
        double clickDuration = 0.01; // 10ms max duration for the click envelope
        if (click > 0.0f && timeInSeconds < clickDuration)
        {
            double clickEnv = std::exp(-timeInSeconds * 600.0); 
            double noise = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
            pClickPhase += clickPitch / sampleRate;
            if (pClickPhase >= 1.0) pClickPhase -= 1.0;
            double clickSine = std::sin(pClickPhase * 2.0 * juce::MathConstants<double>::pi);
            clickVal = click * (clickSine * 0.7 + noise * 0.3) * clickEnv * 0.5;
        }

        sampleVal = std::tanh((sampleVal + clickVal) * drive);
        output[i] = (float)(sampleVal * ampEnv * gain);

        pPhase += phaseIncrement;
        if (pPhase >= 1.0) pPhase -= 1.0;
        pSubPhase += phaseIncrement * 0.5;
        if (pSubPhase >= 1.0) pSubPhase -= 1.0;
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
    double sampleRate = getSampleRate();
    if (sampleRate <= 0) return;

    peakLevel *= 0.95f;
    
    int triggerInterval = (int)(60.0 / bpm * sampleRate);
    
    if (loopEnabled)
    {
        samplesSinceTrigger += numSamples;
        if (samplesSinceTrigger >= triggerInterval)
        {
            samplesSinceTrigger -= triggerInterval;
            shouldTrigger = true;
        }
    }
    
    
    for (const auto metadata : midiMessages)
    {
        if (metadata.getMessage().isNoteOn()) {
            shouldTrigger = true;
            midiTriggered = true;
        }
    }

    if (shouldTrigger) {
        shouldTrigger = false;
        isPlaying = true;
        currentSampleIndex = 0;
        currentPhase = 0.0;
        subPhase = 0.0;
        clickPhase = 0.0;
    }
    
    if (!isPlaying) return;
    
    float outputPeak = 0.0f;
    juce::HeapBlock<float> tempBlock (numSamples);
    
    double pitchEnd = pitch * 0.2;
    double pitchDecayAmount = pitchDecay * 10.0;
    double attackSecs = std::max(0.001f, ampAttack);
    double decaySecs = std::max(0.001f, ampDecay);
    double releaseSecs = std::max(0.001f, ampRelease);
    double clickDuration = 0.01;

    for (int i = 0; i < numSamples; ++i)
    {
        double timeInSeconds = (double)currentSampleIndex / sampleRate;
        
        // Check if envelope has finished
        if (timeInSeconds > attackSecs + decaySecs + releaseSecs) {
            isPlaying = false;
            tempBlock[i] = 0.0f;
            continue;
        }
        
        double currentPitchEnv = pitch * std::exp (-pitchDecayAmount * timeInSeconds * 10.0);
        currentPitchEnv = std::max (currentPitchEnv, pitchEnd);
        double phaseIncrement = currentPitchEnv / sampleRate;

        double ampEnv = 0.0;
        if (timeInSeconds < attackSecs)
            ampEnv = timeInSeconds / attackSecs;
        else if (timeInSeconds < attackSecs + decaySecs)
        {
            double decayEnv = 1.0 - (timeInSeconds - attackSecs) / decaySecs;
            ampEnv = decayEnv + ampSustain * (1.0 - decayEnv);
        }
        else
            ampEnv = ampSustain * std::max(0.0, 1.0 - (timeInSeconds - (attackSecs + decaySecs)) / releaseSecs);

        double sampleVal = 0.0;
        double fundamental = std::sin (currentPhase * 2.0 * juce::MathConstants<double>::pi);

        switch (currentSource)
        {
            case sine: 
                sampleVal = fundamental; 
                break;
            case triangle: 
                sampleVal = 2.0 * std::abs (2.0 * (currentPhase - std::floor (currentPhase + 0.5))) - 1.0; 
                break;
            case saw: 
                sampleVal = 2.0 * currentPhase - 1.0;
                sampleVal -= polyBlep(currentPhase, phaseIncrement);
                break;
            case square: 
                sampleVal = currentPhase < 0.5 ? 1.0 : -1.0;
                sampleVal += polyBlep(currentPhase, phaseIncrement);
                sampleVal -= polyBlep(std::fmod(currentPhase + 0.5, 1.0), phaseIncrement);
                break;
            case loaded:
                sampleVal = fundamental;
                break;
        }

        double harmonic2 = std::sin (currentPhase * 4.0 * juce::MathConstants<double>::pi);
        double harmonic3 = std::sin (currentPhase * 6.0 * juce::MathConstants<double>::pi);
        sampleVal = sampleVal * (1.0 - color * 0.7) + harmonic2 * color * 0.5 + harmonic3 * color * color * 0.2;

        double subOsc = std::sin (subPhase * 2.0 * juce::MathConstants<double>::pi);
        sampleVal += subOsc * depth * 0.5;

        double clickVal = 0.0;
        if (click > 0.0f && timeInSeconds < clickDuration)
        {
            double clickEnv = std::exp(-timeInSeconds * 600.0); 
            double noise = juce::Random::getSystemRandom().nextFloat() * 2.0 - 1.0;
            clickPhase += clickPitch / sampleRate;
            if (clickPhase >= 1.0) clickPhase -= 1.0;
            double clickSine = std::sin(clickPhase * 2.0 * juce::MathConstants<double>::pi);
            clickVal = click * (clickSine * 0.7 + noise * 0.3) * clickEnv * 0.5;
        }

        // Tanh Saturation for Warm Overdrive
        sampleVal = std::tanh((sampleVal + clickVal) * drive);
        
        tempBlock[i] = (float)(sampleVal * ampEnv * gain);

        currentPhase += phaseIncrement;
        if (currentPhase >= 1.0) currentPhase -= 1.0;
        subPhase += phaseIncrement * 0.5;
        if (subPhase >= 1.0) subPhase -= 1.0;
        
        currentSampleIndex++;
    }

    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        for (int i = 0; i < numSamples; ++i)
        {
            if (isPlaying || tempBlock[i] != 0.0f) {
                channelData[i] = tempBlock[i];
                float absVal = std::abs (channelData[i]);
                if (absVal > outputPeak) outputPeak = absVal;
            }
        }
    }
    
    if (outputPeak > peakLevel) peakLevel = outputPeak;
}

bool KikAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* KikAudioProcessor::createEditor() { return new KikAudioProcessorEditor (*this); }
void KikAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {}
void KikAudioProcessor::setStateInformation (const void* data, int sizeInBytes) { juce::ignoreUnused (data, sizeInBytes); }
void KikAudioProcessor::updatePreview() { previewWaveform.resize (2048); generatePreview (previewWaveform.data(), 2048, 44100.0); previewDirty = false; }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KikAudioProcessor(); }