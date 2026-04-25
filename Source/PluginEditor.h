/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class KikAudioProcessorEditor : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    KikAudioProcessorEditor (KikAudioProcessor&);
    ~KikAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    KikAudioProcessor& audioProcessor;
    std::unique_ptr<juce::WebBrowserComponent> browser;
    bool useWebView = false;
    bool webViewReady = false;
    bool bridgeInjected = false;

    void onParameterChange (const juce::String& param, double value);
    void sendInitValuesToWeb();
    void sendMeterToWeb();
    void sendWaveformToWeb();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KikAudioProcessorEditor)
};