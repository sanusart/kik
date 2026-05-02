/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
void applyHpLpInPlace (float* samples, int numSamples, double sampleRate, float hpHz, float lpHz)
{
    if (samples == nullptr || numSamples <= 0 || sampleRate <= 0.0)
        return;

    hpHz = juce::jlimit (25.0f, 600.0f, hpHz);
    lpHz = juce::jlimit (800.0f, 20000.0f, lpHz);
    const float nyquist = (float) (sampleRate * 0.48);
    lpHz = std::min (lpHz, nyquist);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, numSamples), 1 };
    juce::dsp::IIR::Filter<float> hp, lp;
    hp.prepare (spec);
    lp.prepare (spec);
    hp.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass ((float) sampleRate, hpHz, 0.707f);
    lp.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass ((float) sampleRate, lpHz, 0.707f);

    float* const channelPtrs[] = { samples };
    juce::dsp::AudioBlock<float> block (channelPtrs, 1, (size_t) numSamples);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    hp.process (ctx);
    lp.process (ctx);
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"pitch", 1}, "Pitch", 40.0f, 250.0f, 196.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"pitchDecay", 1}, "Pitch Decay", 0.05f, 0.5f, 0.23f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID {"keyNote", 1}, "Key Note", 0, 11, 7));
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID {"keyOctave", 1}, "Key Octave", 0, 5, 3));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"ampAttack", 1}, "Attack", 0.0f, 0.05f, 0.001f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"ampDecay", 1}, "Decay", 0.05f, 1.0f, 0.3f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"ampSustain", 1}, "Sustain", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"ampRelease", 1}, "Release", 0.05f, 1.0f, 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"drive", 1}, "Drive", 0.5f, 3.0f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"color", 1}, "Color", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"click", 1}, "Click", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"clickPitch", 1}, "Click Pitch", 500.0f, 8000.0f, 4000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"depth", 1}, "Sub Depth", 0.0f, 1.0f, 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"hpCutoff", 1},
        "HP Cutoff",
        juce::NormalisableRange<float> (25.0f, 600.0f, 0.01f, 0.45f),
        35.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID {"lpCutoff", 1},
        "LP Cutoff",
        juce::NormalisableRange<float> (800.0f, 20000.0f, 0.01f, 0.32f),
        20000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"gain", 1}, "Gain", 0.0f, 1.5f, 1.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"enhancer", 1}, "Enhancer", 0.0f, 1.0f, 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID {"oversample", 1}, "Oversample", juce::StringArray {"1x", "2x", "4x"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"bpm", 1}, "BPM", 80.0f, 250.0f, 120.0f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID {"loopEnabled", 1}, "Loop", false));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID {"waveform", 1}, "Waveform", juce::StringArray {"Sine", "Triangle", "Saw", "Square", "Loaded"}, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID {"waveMorph", 1}, "Wave Morph", 0.0f, 1.0f, 0.0f));
    return { params.begin(), params.end() };
}

namespace FactoryPresetData
{
    /** Waveform: 0 sine, 1 triangle, 2 saw, 3 square. Values clamped when applied. */
    struct Entry
    {
        const char* name;
        int waveform;
        float pitch, pitchDecay, ampAttack, ampDecay, ampSustain, ampRelease;
        float drive, color, click, clickPitch, depth;
        float hpCutoff, lpCutoff;
    };

    /** Index 0 = plugin defaults (keep aligned with createParameterLayout()). Rest = real mix roles. */
    constexpr Entry entries[] = {
        { "Default",
          0, 196.0f, 0.23f, 0.001f, 0.30f, 0.0f, 0.20f, 1.00f, 0.50f, 0.0f, 4000.0f, 0.50f, 35.0f, 20000.0f },
        /* Layer under samples: low, long, almost no click */
        { "Sub Layer",
          0, 47.0f, 0.07f, 0.002f, 0.58f, 0.0f, 0.55f, 0.92f, 0.10f, 0.06f, 1400.0f, 0.88f, 28.0f, 16500.0f },
        /* Full kick for 4x4 club / house — balanced body + beater */
        { "Club Kick",
          0, 55.0f, 0.17f, 0.001f, 0.38f, 0.0f, 0.30f, 1.14f, 0.22f, 0.30f, 3400.0f, 0.58f, 35.0f, 18500.0f },
        /* Short envelope for heavy sidechain / dense stacks */
        { "Tight Sidechain",
          0, 70.0f, 0.38f, 0.001f, 0.13f, 0.0f, 0.10f, 0.72f, 0.12f, 0.38f, 6200.0f, 0.22f, 48.0f, 13500.0f },
        /* Long 808-style tail, sub-heavy */
        { "Trap Long",
          0, 40.0f, 0.055f, 0.005f, 0.82f, 0.0f, 0.62f, 1.08f, 0.03f, 0.11f, 750.0f, 0.95f, 28.0f, 12000.0f },
        /* Cuts through busy mids — click-led, shorter body */
        { "Punch & Click",
          1, 86.0f, 0.36f, 0.001f, 0.16f, 0.0f, 0.13f, 1.42f, 0.36f, 0.78f, 6000.0f, 0.20f, 38.0f, 17500.0f },
        /* Round, slower attack, musical in house / pop */
        { "Warm House",
          1, 53.0f, 0.15f, 0.0025f, 0.46f, 0.0f, 0.36f, 1.22f, 0.32f, 0.20f, 2700.0f, 0.52f, 32.0f, 16000.0f },
        /* Peak-time techno / industrial — square, drive, short */
        { "Distorted Slam",
          3, 71.0f, 0.29f, 0.001f, 0.18f, 0.0f, 0.15f, 2.55f, 0.65f, 0.58f, 5400.0f, 0.26f, 42.0f, 15000.0f },
        /* Breaks / jungle: mid punch, longer release for rolls */
        { "DnB Roller",
          1, 79.0f, 0.25f, 0.001f, 0.39f, 0.0f, 0.38f, 1.68f, 0.42f, 0.50f, 4450.0f, 0.50f, 38.0f, 17000.0f },
        /* Softer transient, darker click — sits back in the mix */
        { "Lo-Fi Thick",
          0, 64.0f, 0.14f, 0.005f, 0.42f, 0.0f, 0.44f, 1.42f, 0.52f, 0.33f, 1750.0f, 0.45f, 55.0f, 7500.0f },
        /* Festival / big-system — long body + tail */
        { "Big Room",
          0, 51.0f, 0.19f, 0.001f, 0.54f, 0.0f, 0.85f, 1.78f, 0.40f, 0.46f, 3800.0f, 0.64f, 30.0f, 14500.0f },
        /* More beater harmonics — band / rock-ish context */
        { "Rock Punch",
          1, 82.0f, 0.22f, 0.003f, 0.33f, 0.0f, 0.26f, 1.52f, 0.34f, 0.45f, 3100.0f, 0.36f, 40.0f, 15500.0f },
        /* Safe starting point when dialling a session quickly */
        { "Mix Ready",
          0, 92.0f, 0.22f, 0.001f, 0.28f, 0.0f, 0.21f, 1.10f, 0.35f, 0.28f, 3900.0f, 0.42f, 38.0f, 17500.0f },
    };

    constexpr int count = (int) (sizeof (entries) / sizeof (entries[0]));
}

juce::String KikAudioProcessor::getPresetDisplayName() const
{
    const juce::ScopedLock sl (presetDisplayLock);
    return presetDisplayName;
}

void KikAudioProcessor::setPresetDisplayName (juce::String name)
{
    const juce::ScopedLock sl (presetDisplayLock);
    presetDisplayName = std::move (name);
}

KikAudioProcessor::KikAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                     ),
       apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
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
    return FactoryPresetData::count;
}

int KikAudioProcessor::getCurrentProgram()
{
    const int idx = factoryPresetIndex.load (std::memory_order_relaxed);
    return (idx >= 0 && idx < FactoryPresetData::count) ? idx : 0;
}

void KikAudioProcessor::setCurrentProgram (int index)
{
    if (index >= 0 && index < getNumPrograms())
        loadPreset (index);
}

const juce::String KikAudioProcessor::getProgramName (int index)
{
    if (index >= 0 && index < FactoryPresetData::count)
        return juce::String (FactoryPresetData::entries[(size_t) index].name);
    return {};
}

void KikAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void KikAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, samplesPerBlock), 1 };
    hpToneFilter.prepare (spec);
    lpToneFilter.prepare (spec);
    hpToneFilter.reset();
    lpToneFilter.reset();
    const float sr = (float) sampleRate;
    const float lpInit = std::min (20000.0f, sr * 0.48f);
    hpToneFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 35.0f, 0.707f);
    lpToneFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, lpInit, 0.707f);
    lastHpCutoffHz = 35.0f;
    lastLpCutoffHz = lpInit;
    
    // Initialize oversampler based on parameter
    int oversampleIdx = (int)*apvts.getRawParameterValue ("oversample");
    if (oversampleIdx != currentOversampleIndex || !oversamplerPrepared) {
        int factor = 1;
        if (oversampleIdx == 1) factor = 2;
        else if (oversampleIdx == 2) factor = 4;
        
        oversampler = std::make_unique<juce::dsp::Oversampling<float>> (1, factor, 
            juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR, true);
        oversampler->initProcessing ((size_t) juce::jmax (1, samplesPerBlock));
        oversamplerPrepared = true;
        currentOversampleIndex = oversampleIdx;
    }
    
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
    float pitch = *apvts.getRawParameterValue ("pitch");
    float pitchDecay = *apvts.getRawParameterValue ("pitchDecay");
    float ampAttack = *apvts.getRawParameterValue ("ampAttack");
    float ampDecay = *apvts.getRawParameterValue ("ampDecay");
    float ampSustain = *apvts.getRawParameterValue ("ampSustain");
    float ampRelease = *apvts.getRawParameterValue ("ampRelease");
    float drive = *apvts.getRawParameterValue ("drive");
    float color = *apvts.getRawParameterValue ("color");
    float click = *apvts.getRawParameterValue ("click");
    float clickPitch = *apvts.getRawParameterValue ("clickPitch");
    float depth = *apvts.getRawParameterValue ("depth");
    float gain = *apvts.getRawParameterValue ("gain");
    float hpCut = apvts.getRawParameterValue ("hpCutoff")->load();
    float lpCut = apvts.getRawParameterValue ("lpCutoff")->load();
    currentSource = (WaveformSource)(int)*apvts.getRawParameterValue ("waveform");

    int kickLength = (int)(0.5 * sampleRate); // 500ms preview max
    if (kickLength > outputSamples) kickLength = outputSamples;
    
    double pPhase = 0.0;
    double pSubPhase = 0.0;
    double pClickPhase = 0.0;
    double pPitchEnv = (double)pitch;
    double pitchEnd = (double)pitch * 0.2;
    double pitchDecayAmount = (double)pitchDecay * 10.0;

    for (int i = 0; i < kickLength; ++i)
    {
        double timeInSeconds = (double)i / sampleRate;
        pPitchEnv = (double)pitch * std::exp (-pitchDecayAmount * timeInSeconds * 10.0);
        pPitchEnv = std::max (pPitchEnv, pitchEnd);
        double phaseIncrement = pPitchEnv / sampleRate;

        double ampEnv = 0.0;
        double attackSecs = std::max(0.001, (double)ampAttack);
        double decaySecs = std::max(0.001, (double)ampDecay);
        double releaseSecs = std::max(0.001, (double)ampRelease);

        if (timeInSeconds < attackSecs)
            ampEnv = timeInSeconds / attackSecs;
        else if (timeInSeconds < attackSecs + decaySecs)
        {
            double decayEnv = 1.0 - (timeInSeconds - attackSecs) / decaySecs;
            ampEnv = decayEnv + (double)ampSustain * (1.0 - decayEnv);
        }
        else if (timeInSeconds < attackSecs + decaySecs + 0.1) // Fake 100ms hold for preview
            ampEnv = (double)ampSustain;
        else
            ampEnv = (double)ampSustain * std::max(0.0, 1.0 - (timeInSeconds - (attackSecs + decaySecs + 0.1)) / releaseSecs);

        double sampleVal = 0.0;
        double fundamental = std::sin (pPhase * 2.0 * juce::MathConstants<double>::pi);

        switch (currentSource)
        {
            case sine: 
                sampleVal = fundamental; 
                break;
            case triangle: 
                sampleVal = 2.0 * std::abs (2.0 * (pPhase - std::floor (pPhase + 0.5))) - 1.0; 
                break;
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
        sampleVal = sampleVal * (1.0 - (double)color * 0.7) + harmonic2 * (double)color * 0.5 + harmonic3 * (double)color * (double)color * 0.2;

        double subOsc = std::sin (pSubPhase * 2.0 * juce::MathConstants<double>::pi);
        sampleVal += subOsc * (double)depth * 0.5;

        double clickVal = 0.0;
        double clickDuration = 0.01;
        if (click > 0.0f && timeInSeconds < clickDuration)
        {
            double clickEnv = std::exp(-timeInSeconds * 600.0); 
            double noise = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
            pClickPhase += (double)clickPitch / sampleRate;
            if (pClickPhase >= 1.0) pClickPhase -= 1.0;
            double clickSine = std::sin(pClickPhase * 2.0 * juce::MathConstants<double>::pi);
            clickVal = (double)click * (clickSine * 0.7 + noise * 0.3) * clickEnv * 0.5;
        }

        sampleVal = std::tanh((sampleVal + clickVal) * (double)drive);
        output[i] = (float)(sampleVal * ampEnv * (double)gain);

        pPhase += phaseIncrement;
        if (pPhase >= 1.0) pPhase -= 1.0;
        pSubPhase += phaseIncrement * 0.5;
        if (pSubPhase >= 1.0) pSubPhase -= 1.0;
    }

    for (int i = kickLength; i < outputSamples; ++i) output[i] = 0.0f;

    applyHpLpInPlace (output, kickLength, sampleRate, hpCut, lpCut);
}

void KikAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (!isPrepared) return;
    juce::ScopedNoDenormals noDenormals;

    float pitch = *apvts.getRawParameterValue ("pitch");
    float pitchDecay = *apvts.getRawParameterValue ("pitchDecay");
    float ampAttack = *apvts.getRawParameterValue ("ampAttack");
    float ampDecay = *apvts.getRawParameterValue ("ampDecay");
    float ampSustain = *apvts.getRawParameterValue ("ampSustain");
    float ampRelease = *apvts.getRawParameterValue ("ampRelease");
    float drive = *apvts.getRawParameterValue ("drive");
    float color = *apvts.getRawParameterValue ("color");
    float click = *apvts.getRawParameterValue ("click");
    float clickPitch = *apvts.getRawParameterValue ("clickPitch");
    float depth = *apvts.getRawParameterValue ("depth");
    float gain = *apvts.getRawParameterValue ("gain");
    float hpCut = juce::jlimit (25.0f, 600.0f, apvts.getRawParameterValue ("hpCutoff")->load());
    float lpCut = juce::jlimit (800.0f, 20000.0f, apvts.getRawParameterValue ("lpCutoff")->load());
    float bpm = *apvts.getRawParameterValue ("bpm");
    bool loopEnabled = (bool)*apvts.getRawParameterValue ("loopEnabled");
    currentSource = (WaveformSource)(int)*apvts.getRawParameterValue ("waveform");

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0) return;
    double sampleRate = getSampleRate();
    if (sampleRate <= 0) return;

    lpCut = std::min (lpCut, (float) (sampleRate * 0.48));

    peakLevel *= 0.95f;
    
    int triggerInterval = (int)(60.0 / (double)bpm * sampleRate);
    
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
        if (metadata.getMessage().isNoteOn())
        {
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
    
    double pitchEnd = (double)pitch * 0.2;
    double pitchDecayAmount = (double)pitchDecay * 10.0;
    double attackSecs = std::max(0.001, (double)ampAttack);
    double decaySecs = std::max(0.001, (double)ampDecay);
    double releaseSecs = std::max(0.001, (double)ampRelease);
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
        
        double currentPitchEnv = (double)pitch * std::exp (-pitchDecayAmount * timeInSeconds * 10.0);
        currentPitchEnv = std::max (currentPitchEnv, pitchEnd);
        double phaseIncrement = currentPitchEnv / sampleRate;

        double ampEnv = 0.0;
        if (timeInSeconds < attackSecs)
            ampEnv = timeInSeconds / attackSecs;
        else if (timeInSeconds < attackSecs + decaySecs)
        {
            double decayEnv = 1.0 - (timeInSeconds - attackSecs) / decaySecs;
            ampEnv = decayEnv + (double)ampSustain * (1.0 - decayEnv);
        }
        else
            ampEnv = (double)ampSustain * std::max(0.0, 1.0 - (timeInSeconds - (attackSecs + decaySecs)) / releaseSecs);

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
        sampleVal = sampleVal * (1.0 - (double)color * 0.7) + harmonic2 * (double)color * 0.5 + harmonic3 * (double)color * (double)color * 0.2;
        
        // Wave Morph: 0=sine, 0.33=saturated, 0.66=clipped, 1.0=asymmetrical
        float waveMorph = *apvts.getRawParameterValue ("waveMorph");
        if (waveMorph > 0.0f) {
            double absVal = std::abs (sampleVal);
            if (waveMorph < 0.33f) {
                // Saturated sine
                double satAmount = waveMorph * 3.0;
                sampleVal = sampleVal * (1.0 + satAmount * 2.0) / (1.0 + absVal * satAmount * 2.0);
            } else if (waveMorph < 0.66f) {
                // Clipped
                double clipAmount = (waveMorph - 0.33) * 3.0;
                double clipped = std::max (-1.0, std::min (1.0, sampleVal));
                sampleVal = sampleVal * (1.0 - clipAmount) + clipped * clipAmount;
            } else {
                // Asymmetrical
                double asymAmount = (waveMorph - 0.66) * 3.0;
                double rectified = sampleVal > 0.0 ? sampleVal : sampleVal * 0.3;
                sampleVal = sampleVal * (1.0 - asymAmount) + rectified * asymAmount;
            }
        }

        double subOsc = std::sin (subPhase * 2.0 * juce::MathConstants<double>::pi);
        sampleVal += subOsc * (double)depth * 0.5;

        double clickVal = 0.0;
        if (click > 0.0f && timeInSeconds < clickDuration)
        {
            double clickEnv = std::exp(-timeInSeconds * 600.0); 
            double noise = juce::Random::getSystemRandom().nextFloat() * 2.0 - 1.0;
            clickPhase += (double)clickPitch / sampleRate;
            if (clickPhase >= 1.0) clickPhase -= 1.0;
            double clickSine = std::sin(clickPhase * 2.0 * juce::MathConstants<double>::pi);
            clickVal = (double)click * (clickSine * 0.7 + noise * 0.3) * clickEnv * 0.5;
        }

        // Anti-aliased saturation
        double sampleVal2 = (sampleVal + clickVal) * (double)drive;
        // Soft clip with oversampling approximation
        double drivenSample = sampleVal2 / (1.0 + std::abs(sampleVal2) * 0.5);
        
        double sampleWithGain = drivenSample * ampEnv * (double)gain;
        
        // Dynamic Enhancer: transient boost + soft limiting
        float enhancer = *apvts.getRawParameterValue ("enhancer");
        if (enhancer > 0.0f) {
            double absSample = std::abs (sampleWithGain);
            double transientBoost = 1.0 + (double)enhancer * 2.0 * std::min (1.0, absSample * 4.0);
            double limited = sampleWithGain * transientBoost;
            // Soft clip to prevent harsh distortion
            limited = std::tanh (limited * (0.8 + (double)enhancer * 0.4));
            sampleWithGain = limited;
        }
        
        tempBlock[i] = (float)sampleWithGain;
        
        currentPhase += phaseIncrement;
        if (currentPhase >= 1.0) currentPhase -= 1.0;
        subPhase += phaseIncrement * 0.5;
        if (subPhase >= 1.0) subPhase -= 1.0;
        
        currentSampleIndex++;
    }

    if (std::abs (hpCut - lastHpCutoffHz) > 0.5f)
    {
        hpToneFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass ((float) sampleRate, hpCut, 0.707f);
        lastHpCutoffHz = hpCut;
    }
    if (std::abs (lpCut - lastLpCutoffHz) > 1.0f)
    {
        lpToneFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass ((float) sampleRate, lpCut, 0.707f);
        lastLpCutoffHz = lpCut;
    }

    {
        float* const channelPtrs[] = { tempBlock.get() };
        juce::dsp::AudioBlock<float> block (channelPtrs, 1, (size_t) numSamples);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        hpToneFilter.process (ctx);
        lpToneFilter.process (ctx);
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
void KikAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void KikAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
            syncKeyNoteFromPitch();
            factoryPresetIndex.store (-1);
            {
                const juce::ScopedLock sl (presetDisplayLock);
                presetDisplayName = "Project";
            }
            presetStateVersion.fetch_add (1u, std::memory_order_relaxed);
        }
}

void KikAudioProcessor::updatePreview() { previewWaveform.resize (2048); generatePreview (previewWaveform.data(), 2048, 44100.0); previewDirty = false; }

namespace
{
    void setPhysicalParam (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float v)
    {
        if (auto* p = apvts.getParameter (id))
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                rp->setValueNotifyingHost (rp->convertTo0to1 (v));
    }

    void setWaveformIndex (juce::AudioProcessorValueTreeState& apvts, int choiceIndex)
    {
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("waveform")))
            c->setValueNotifyingHost (c->convertTo0to1 ((float) choiceIndex));
    }
}

void KikAudioProcessor::applyPitchFromKeyNote()
{
    int note = 0;
    int oct = 0;
    if (auto* p = apvts.getParameter ("keyNote"))
        if (auto* pi = dynamic_cast<juce::AudioParameterInt*> (p))
            note = pi->get();
    if (auto* p = apvts.getParameter ("keyOctave"))
        if (auto* pi = dynamic_cast<juce::AudioParameterInt*> (p))
            oct = pi->get();

    const int midi = juce::jlimit (12, 83, 12 * (oct + 1) + note);
    float hz = 440.0f * std::pow (2.0f, (float) (midi - 69) / 12.0f);
    hz = juce::jlimit (40.0f, 250.0f, hz);

    if (auto* p = apvts.getParameter ("pitch"))
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->convertTo0to1 (hz));

    previewDirty = true;
}

void KikAudioProcessor::syncKeyNoteFromPitch()
{
    float hz = *apvts.getRawParameterValue ("pitch");
    hz = juce::jlimit (40.0f, 250.0f, hz);

    const int midi = juce::jlimit (12, 83, juce::roundToInt (69.0f + 12.0f * std::log2 (hz / 440.0f)));
    const int note = midi % 12;
    const int octave = midi / 12 - 1;

    if (auto* p = apvts.getParameter ("keyNote"))
        if (auto* pi = dynamic_cast<juce::AudioParameterInt*> (p))
            pi->setValueNotifyingHost (pi->convertTo0to1 ((float) note));

    if (auto* p = apvts.getParameter ("keyOctave"))
        if (auto* pi = dynamic_cast<juce::AudioParameterInt*> (p))
            pi->setValueNotifyingHost (pi->convertTo0to1 ((float) octave));
}

void KikAudioProcessor::loadPreset(int index)
{
    if (index < 0 || index >= FactoryPresetData::count)
        return;

    const auto& P = FactoryPresetData::entries[(size_t) index];

    setWaveformIndex (apvts, juce::jlimit (0, 3, P.waveform));
    setPhysicalParam (apvts, "pitch", juce::jlimit (40.0f, 250.0f, P.pitch));
    setPhysicalParam (apvts, "pitchDecay", juce::jlimit (0.05f, 0.5f, P.pitchDecay));
    setPhysicalParam (apvts, "ampAttack", juce::jlimit (0.0f, 0.05f, P.ampAttack));
    setPhysicalParam (apvts, "ampDecay", juce::jlimit (0.05f, 1.0f, P.ampDecay));
    setPhysicalParam (apvts, "ampSustain", juce::jlimit (0.0f, 1.0f, P.ampSustain));
    setPhysicalParam (apvts, "ampRelease", juce::jlimit (0.05f, 1.0f, P.ampRelease));
    setPhysicalParam (apvts, "drive", juce::jlimit (0.5f, 3.0f, P.drive));
    setPhysicalParam (apvts, "color", juce::jlimit (0.0f, 1.0f, P.color));
    setPhysicalParam (apvts, "click", juce::jlimit (0.0f, 1.0f, P.click));
    setPhysicalParam (apvts, "clickPitch", juce::jlimit (500.0f, 8000.0f, P.clickPitch));
    setPhysicalParam (apvts, "depth", juce::jlimit (0.0f, 1.0f, P.depth));
    setPhysicalParam (apvts, "hpCutoff", juce::jlimit (25.0f, 600.0f, P.hpCutoff));
    setPhysicalParam (apvts, "lpCutoff", juce::jlimit (800.0f, 20000.0f, P.lpCutoff));

    syncKeyNoteFromPitch();

    factoryPresetIndex.store (index);
    {
        const juce::ScopedLock sl (presetDisplayLock);
        presetDisplayName = getProgramName (index);
    }
    presetStateVersion.fetch_add (1u, std::memory_order_relaxed);
    previewDirty = true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KikAudioProcessor(); }