/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class KikAudioProcessorEditor : public juce::AudioProcessorEditor,
                              public juce::Slider::Listener,
                              public juce::ComboBox::Listener,
                              public juce::Button::Listener
{
public:
    KikAudioProcessorEditor (KikAudioProcessor&);
    ~KikAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged (juce::Slider* slider) override;
    void comboBoxChanged (juce::ComboBox* comboBox) override;
    void buttonClicked (juce::Button* button) override;

private:
    KikAudioProcessor& audioProcessor;

    juce::Slider* pitchStartSlider = nullptr;
    juce::Slider* pitchDecaySlider = nullptr;
    juce::Slider* ampAttackSlider = nullptr;
    juce::Slider* ampDecaySlider = nullptr;
    juce::Slider* ampSustainSlider = nullptr;
    juce::Slider* ampReleaseSlider = nullptr;
    juce::Slider* driveSlider = nullptr;
    juce::Slider* clickSlider = nullptr;
    juce::Slider* bpmSlider = nullptr;
    juce::Slider* colorSlider = nullptr;
    juce::Slider* depthSlider = nullptr;
    juce::Slider* gainSlider = nullptr;
    juce::ComboBox* waveformCombo = nullptr;
    juce::TextButton* kickButton = nullptr;
    juce::ToggleButton* loopButton = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KikAudioProcessorEditor)
};