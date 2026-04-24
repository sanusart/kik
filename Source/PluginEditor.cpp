/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

KikAudioProcessorEditor::KikAudioProcessorEditor (KikAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (480, 600);
    auto margin = 15;
    auto w = getWidth() - margin * 2;

    kickButton = new juce::TextButton ("KICK");
    kickButton->setBounds (margin, 12, 60, 28);
    kickButton->addListener (this);
    addAndMakeVisible (kickButton);

    loopButton = new juce::ToggleButton ("LOOP");
    loopButton->setBounds (margin + 70, 12, 60, 28);
    loopButton->addListener (this);
    addAndMakeVisible (loopButton);

    bpmSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
    bpmSlider->setRange (80, 250);
    bpmSlider->setValue (audioProcessor.bpm);
    bpmSlider->setBounds (margin + 140, 14, 80, 24);
    bpmSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 45, 20);
    bpmSlider->addListener (this);
    addAndMakeVisible (bpmSlider);

    waveformCombo = new juce::ComboBox ("wave");
    waveformCombo->addItem ("Sine", 1);
    waveformCombo->addItem ("Triangle", 2);
    waveformCombo->addItem ("Saw", 3);
    waveformCombo->addItem ("Square", 4);
    waveformCombo->addItem ("Loaded", 5);
    waveformCombo->setSelectedId (1);
    waveformCombo->setBounds (margin, 50, w, 24);
    waveformCombo->addListener (this);
    addAndMakeVisible (waveformCombo);

    auto y = 80;
    pitchStartSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    pitchStartSlider->setRange (40, 250);
    pitchStartSlider->setValue (audioProcessor.pitch);
    pitchStartSlider->setBounds (margin, y, w * 0.45f, 50);
    pitchStartSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 50, 18);
    pitchStartSlider->addListener (this);
    addAndMakeVisible (pitchStartSlider);

    pitchDecaySlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    pitchDecaySlider->setRange (0.05, 0.5);
    pitchDecaySlider->setValue (audioProcessor.pitchDecay);
    pitchDecaySlider->setBounds (margin + w * 0.5f, y, w * 0.45f, 50);
    pitchDecaySlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 50, 18);
    pitchDecaySlider->addListener (this);
    addAndMakeVisible (pitchDecaySlider);

    auto* pitchDecLabel = new juce::Label ("pitchDecLbl", "Decay");
    pitchDecLabel->setBounds (margin + w * 0.5f, y - 14, w * 0.45f, 12);
    pitchDecLabel->setFont (juce::FontOptions (10));
    pitchDecLabel->setJustificationType (juce::Justification::centred);
    addAndMakeVisible (pitchDecLabel);

    auto* pitchLabel = new juce::Label ("pitchLbl", "Pitch");
    pitchLabel->setBounds (margin, y - 14, w * 0.45f, 12);
    pitchLabel->setFont (juce::FontOptions (10));
    pitchLabel->setJustificationType (juce::Justification::centred);
    addAndMakeVisible (pitchLabel);

    y = 160;
    auto quarterW = w * 0.22f;
    ampAttackSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    ampAttackSlider->setRange (0, 0.05);
    ampAttackSlider->setValue (audioProcessor.ampAttack);
    ampAttackSlider->setBounds (margin, y, quarterW, 50);
    ampAttackSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 40, 18);
    ampAttackSlider->addListener (this);
    addAndMakeVisible (ampAttackSlider);

    ampDecaySlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    ampDecaySlider->setRange (0.05, 1.0);
    ampDecaySlider->setValue (audioProcessor.ampDecay);
    ampDecaySlider->setBounds (margin + quarterW + 8, y, quarterW, 50);
    ampDecaySlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 40, 18);
    ampDecaySlider->addListener (this);
    addAndMakeVisible (ampDecaySlider);

    ampSustainSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    ampSustainSlider->setRange (0, 1);
    ampSustainSlider->setValue (audioProcessor.ampSustain);
    ampSustainSlider->setBounds (margin + quarterW * 2 + 16, y, quarterW, 50);
    ampSustainSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 40, 18);
    ampSustainSlider->addListener (this);
    addAndMakeVisible (ampSustainSlider);

    ampReleaseSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    ampReleaseSlider->setRange (0.05, 1.0);
    ampReleaseSlider->setValue (audioProcessor.ampRelease);
    ampReleaseSlider->setBounds (margin + quarterW * 3 + 24, y, quarterW, 50);
    ampReleaseSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 40, 18);
    ampReleaseSlider->addListener (this);
    addAndMakeVisible (ampReleaseSlider);

    y = 250;
    driveSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    driveSlider->setRange (0.5, 3);
    driveSlider->setValue (audioProcessor.drive);
    driveSlider->setBounds (margin, y, w * 0.45f, 50);
    driveSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 50, 18);
    driveSlider->addListener (this);
    addAndMakeVisible (driveSlider);

    colorSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    colorSlider->setRange (0, 1);
    colorSlider->setValue (audioProcessor.color);
    colorSlider->setBounds (margin + w * 0.5f, y, w * 0.45f, 50);
    colorSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 50, 18);
    colorSlider->addListener (this);
    addAndMakeVisible (colorSlider);

    y = 310;
    clickSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    clickSlider->setRange (0, 1);
    clickSlider->setValue (audioProcessor.click);
    clickSlider->setBounds (margin, y, w * 0.45f, 50);
    clickSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 50, 18);
    clickSlider->addListener (this);
    addAndMakeVisible (clickSlider);

    depthSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    depthSlider->setRange (0, 1);
    depthSlider->setValue (audioProcessor.depth);
    depthSlider->setBounds (margin + w * 0.5f, y, w * 0.45f, 50);
    depthSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 50, 18);
    depthSlider->addListener (this);
    addAndMakeVisible (depthSlider);

    y = 370;
    gainSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    gainSlider->setRange (0, 1.5);
    gainSlider->setValue (audioProcessor.gain);
    gainSlider->setBounds (margin, y, w, 50);
    gainSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 60, 18);
    gainSlider->addListener (this);
    addAndMakeVisible (gainSlider);
}

KikAudioProcessorEditor::~KikAudioProcessorEditor() {}

void KikAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    if (audioProcessor.previewDirty) audioProcessor.updatePreview();
    
    auto& waveform = audioProcessor.previewWaveform;
    if (waveform.empty()) return;
    
    auto w = getWidth() - 30;
    auto h = 70;
    auto y = 440;
    auto margin = 15;
    auto meterX = margin + w + 8;
    auto meterW = 12;
    
    g.setColour (juce::Colours::darkgrey);
    g.fillRect ((float)margin, (float)y, (float)w, (float)h);
    
    g.setColour (juce::Colours::cyan);
    
    int numSamples = (int)waveform.size();
    for (int i = 0; i < w; ++i)
    {
        int idx = i * numSamples / w;
        float sample = waveform[idx];
        int yPos = y + h/2 - (int)(sample * h * 0.45f);
        g.fillRect ((float)(margin + i), (float)yPos, 1.0f, 1.0f);
    }
    
    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (8));
    float dbY = y + 8;
    g.drawFittedText ("0", margin, (int)dbY, 20, 10, juce::Justification::left, 1);
    g.drawFittedText ("-6", margin, (int)(y + h*0.33f), 20, 10, juce::Justification::left, 1);
    g.drawFittedText ("-12", margin, (int)(y + h*0.55f), 25, 10, juce::Justification::left, 1);
    g.drawFittedText ("-24", margin, (int)(y + h*0.8f), 25, 10, juce::Justification::left, 1);
    
    g.setColour (juce::Colours::darkgrey);
    g.fillRect ((float)meterX, (float)y, (float)meterW, (float)h);
    
    float peak = audioProcessor.peakLevel;
    float peakY = y + h - (peak * h);
    
    if (peak >= 1.0f)
        g.setColour (juce::Colours::red);
    else if (peak >= 0.9f)
        g.setColour (juce::Colours::orange);
    else
        g.setColour (juce::Colours::yellow);
    
    g.fillRect ((float)meterX, (float)peakY, (float)meterW, (float)3);
    
    for (int i = 0; i < 4; ++i)
    {
        float lineY = y + h * (1.0f - (float)i * 0.25f);
        g.setColour (juce::Colours::grey.withAlpha (0.5f));
        g.drawLine ((float)meterX, lineY, (float)(meterX + meterW), lineY, 1.0f);
    }
}

void KikAudioProcessorEditor::resized() {}

void KikAudioProcessorEditor::sliderValueChanged (juce::Slider* slider)
{
    if (slider == pitchStartSlider) { audioProcessor.pitch = slider->getValue(); audioProcessor.previewDirty = true; }
    else if (slider == pitchDecaySlider) { audioProcessor.pitchDecay = slider->getValue(); audioProcessor.previewDirty = true; }
    else if (slider == driveSlider) { audioProcessor.drive = slider->getValue(); audioProcessor.previewDirty = true; }
    else if (slider == clickSlider) { audioProcessor.click = slider->getValue(); audioProcessor.previewDirty = true; }
    else if (slider == colorSlider) { audioProcessor.color = slider->getValue(); audioProcessor.previewDirty = true; }
    else if (slider == depthSlider) { audioProcessor.depth = slider->getValue(); audioProcessor.previewDirty = true; }
    else if (slider == gainSlider) { audioProcessor.gain = slider->getValue(); audioProcessor.previewDirty = true; }
    else if (slider == bpmSlider) audioProcessor.bpm = slider->getValue();
    else if (slider == ampAttackSlider) audioProcessor.ampAttack = slider->getValue();
    else if (slider == ampDecaySlider) audioProcessor.ampDecay = slider->getValue();
    else if (slider == ampSustainSlider) audioProcessor.ampSustain = slider->getValue();
    else if (slider == ampReleaseSlider) audioProcessor.ampRelease = slider->getValue();
    repaint();
}

void KikAudioProcessorEditor::comboBoxChanged (juce::ComboBox* comboBox)
{
    if (comboBox == waveformCombo)
    {
        auto id = waveformCombo->getSelectedId();
        if (id == 1) audioProcessor.currentSource = KikAudioProcessor::sine;
        else if (id == 2) audioProcessor.currentSource = KikAudioProcessor::triangle;
        else if (id == 3) audioProcessor.currentSource = KikAudioProcessor::saw;
        else if (id == 4) audioProcessor.currentSource = KikAudioProcessor::square;
        else if (id == 5) audioProcessor.currentSource = KikAudioProcessor::loaded;
        audioProcessor.previewDirty = true;
        repaint();
    }
}

void KikAudioProcessorEditor::buttonClicked (juce::Button* button)
{
    if (button == kickButton) audioProcessor.shouldTrigger = true;
    else if (button == loopButton)
    {
        audioProcessor.loopEnabled = loopButton->getToggleState();
        audioProcessor.samplesSinceTrigger = 0;
    }
}