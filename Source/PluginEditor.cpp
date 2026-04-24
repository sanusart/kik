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
    auto margin = 20;
    auto w = getWidth() - margin * 2;

    kickButton = new juce::TextButton ("KICK");
    kickButton->setBounds (margin, 15, 60, 28);
    kickButton->addListener (this);
    addAndMakeVisible (kickButton);

    loopButton = new juce::ToggleButton ("LOOP");
    loopButton->setBounds (margin + 70, 15, 60, 28);
    loopButton->addListener (this);
    addAndMakeVisible (loopButton);

    bpmSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
    bpmSlider->setRange (80, 250);
    bpmSlider->setValue (audioProcessor.bpm);
    bpmSlider->setBounds (margin + 140, 17, 80, 24);
    bpmSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 50, 18);
    bpmSlider->addListener (this);
    addAndMakeVisible (bpmSlider);

    waveformCombo = new juce::ComboBox ("wave");
    waveformCombo->addItem ("Sine", 1);
    waveformCombo->addItem ("Triangle", 2);
    waveformCombo->addItem ("Saw", 3);
    waveformCombo->addItem ("Square", 4);
    waveformCombo->addItem ("Loaded", 5);
    waveformCombo->setSelectedId (1);
    waveformCombo->setBounds (margin, 55, w, 28);
    waveformCombo->addListener (this);
    addAndMakeVisible (waveformCombo);

    auto rowY = 110;
    auto colW = (w - 10) / 2;

    auto makeRow = [&](const char* name, juce::Slider*& slider, float minVal, float maxVal, float& param, float offset) {
        slider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        slider->setRange (minVal, maxVal);
        slider->setValue (param);
        slider->setBounds (margin + offset, rowY, colW, 24);
        slider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 55, 18);
        slider->addListener (this);
        addAndMakeVisible (slider);
        
        auto* label = new juce::Label (name, name);
        label->setBounds (margin + offset + 55, rowY - 14, 50, 14);
        label->setFont (juce::FontOptions (11, juce::Font::bold));
        label->setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (label);
    };

    auto makeRowNoVal = [&](const char* name, juce::Slider*& slider, float minVal, float maxVal, float& param, float offset) {
        slider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        slider->setRange (minVal, maxVal);
        slider->setValue (param);
        slider->setBounds (margin + offset, rowY, colW, 24);
        slider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 55, 18);
        slider->addListener (this);
        addAndMakeVisible (slider);
        
        auto* label = new juce::Label (name, name);
        label->setBounds (margin + offset + colW - 30, rowY - 14, 50, 14);
        label->setFont (juce::FontOptions (11, juce::Font::bold));
        label->setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (label);
    };

    makeRow ("PITCH", pitchStartSlider, 40, 250, audioProcessor.pitch, 0);
    makeRow ("DECAY", pitchDecaySlider, 0.05, 0.5, audioProcessor.pitchDecay, colW + 10);

    rowY = 170;
    auto qw = (w - 30) / 4;
    auto makeAmp = [&](const char* name, juce::Slider*& slider, float minVal, float maxVal, float& param, float offset) {
        slider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        slider->setRange (minVal, maxVal);
        slider->setValue (param);
        slider->setBounds (margin + offset, rowY, qw, 24);
        slider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 45, 16);
        slider->addListener (this);
        addAndMakeVisible (slider);
        
        auto* label = new juce::Label (name, name);
        label->setBounds (margin + offset, rowY - 12, 35, 12);
        label->setFont (juce::FontOptions (10));
        label->setColour (juce::Label::textColourId, juce::Colours::white);
        label->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    makeAmp ("ATK", ampAttackSlider, 0, 0.05, audioProcessor.ampAttack, 0);
    makeAmp ("DEC", ampDecaySlider, 0.05, 1.0, audioProcessor.ampDecay, qw + 5);
    makeAmp ("SUS", ampSustainSlider, 0, 1, audioProcessor.ampSustain, qw * 2 + 10);
    makeAmp ("REL", ampReleaseSlider, 0.05, 1.0, audioProcessor.ampRelease, qw * 3 + 15);

    auto* ampTitle = new juce::Label ("ampTitle", "AMP");
    ampTitle->setBounds (margin, rowY - 20, 50, 14);
    ampTitle->setFont (juce::FontOptions (11, juce::Font::bold));
    ampTitle->setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (ampTitle);

    rowY = 250;
    makeRow ("DRIVE", driveSlider, 0.5, 3, audioProcessor.drive, 0);
    makeRow ("COLOR", colorSlider, 0, 1, audioProcessor.color, colW + 10);

    rowY = 305;
    makeRow ("CLICK", clickSlider, 0, 1, audioProcessor.click, 0);
    makeRow ("DEPTH", depthSlider, 0, 1, audioProcessor.depth, colW + 10);

    rowY = 360;
    gainSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
    gainSlider->setRange (0, 1.5);
    gainSlider->setValue (audioProcessor.gain);
    gainSlider->setBounds (margin, rowY, w, 24);
    gainSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 55, 18);
    gainSlider->addListener (this);
    addAndMakeVisible (gainSlider);

    auto* gainLabel = new juce::Label ("gainLbl", "GAIN");
    gainLabel->setBounds (margin + 60, rowY - 14, 50, 14);
    gainLabel->setFont (juce::FontOptions (11, juce::Font::bold));
    gainLabel->setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (gainLabel);
}

KikAudioProcessorEditor::~KikAudioProcessorEditor() {}

void KikAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    if (audioProcessor.previewDirty) audioProcessor.updatePreview();
    
    auto& waveform = audioProcessor.previewWaveform;
    
    auto w = getWidth() - 40;
    auto h = 80;
    auto y = 420;
    auto margin = 20;
    auto meterX = margin + w + 10;
    auto meterW = 14;
    auto meterH = h - 10;
    
    g.setColour (juce::Colours::darkgrey);
    g.fillRect ((float)margin, (float)y, (float)w, (float)h);
    
    if (!waveform.empty())
    {
        g.setColour (juce::Colours::cyan);
        int numSamples = (int)waveform.size();
        for (int i = 0; i < w; ++i)
        {
            int idx = i * numSamples / w;
            float sample = waveform[idx];
            int yPos = y + h/2 - (int)(sample * h * 0.45f);
            g.fillRect ((float)(margin + i), (float)yPos, 1.0f, 1.0f);
        }
    }
    
    g.setColour (juce::Colours::grey);
    g.setFont (juce::FontOptions (9));
    g.drawFittedText ("0dB", margin, (int)(y + 2), 25, 10, juce::Justification::left, 1);
    g.drawFittedText ("-6dB", margin, (int)(y + h*0.33f), 30, 10, juce::Justification::left, 1);
    g.drawFittedText ("-12dB", margin, (int)(y + h*0.55f), 35, 10, juce::Justification::left, 1);
    g.drawFittedText ("-24dB", margin, (int)(y + h*0.8f), 35, 10, juce::Justification::left, 1);
    
    g.setColour (juce::Colours::black);
    g.fillRect ((float)meterX, (float)(y + 5), (float)meterW, (float)(meterH));
    
    float peak = audioProcessor.peakLevel;
    float db = (peak > 0.0001f) ? (20.0f * std::log10 (peak)) : -60.0f;
    int meterLevel = (peak < 0.001f ? 0 : (y + 5 + meterH - (int)(peak * meterH)));
    meterLevel = juce::jlimit (y + 5, y + 5 + meterH, meterLevel);
    
    if (db >= -0.5f) g.setColour (juce::Colours::red);
    else if (db >= -6.0f) g.setColour (juce::Colours::orange);
    else if (db >= -12.0f) g.setColour (juce::Colours::yellow);
    else g.setColour (juce::Colours::green);
    
    g.fillRect ((float)meterX, (float)meterLevel, (float)meterW, (float)(y + 5 + meterH - meterLevel));
    
    char buf[16];
    std::snprintf (buf, sizeof(buf), "%.1f dB", db);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (10));
    g.drawFittedText (buf, meterX, y + h + 5, meterW + 10, 14, juce::Justification::centred, 1);
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