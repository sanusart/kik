/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/** Web UI root fills the editor; keep these in sync with any layout change. */
namespace KikEditorLayout
{
    inline constexpr int width = 520;
    /** Tall enough for current strips (envelope + output card); raise if UI grows. */
    inline constexpr int height = 688;
}

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

    uint32_t lastSeenPresetStateVersion = 0;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::String resourcesPath;

    void onParameterChange (const juce::String& param, double value);
    void saveStateToFile();
    void loadStateToFile();
    void sendInitValuesToWeb();
    void sendMeterToWeb();
    void sendWaveformToWeb();
    void sendEventToWeb (const juce::String& eventName, const juce::var& data);
    void handleJuceEvent (const juce::var& eventData);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KikAudioProcessorEditor)
};
