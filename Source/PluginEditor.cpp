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
    setSize (500, 600);
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
    bpmSlider->setBounds (margin + 140, 17, 70, 24);
    bpmSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 40, 20);
    bpmSlider->addListener (this);
    addAndMakeVisible (bpmSlider);

    auto* bpmLabel = new juce::Label ("bpmLbl", "BPM");
    bpmLabel->setBounds (margin + 215, 17, 30, 24);
    bpmLabel->setFont (juce::FontOptions (10));
    bpmLabel->setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (bpmLabel);

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

    auto rowY = 115;
    auto colW = (w - 10) / 2;

    pitchStartSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxRight);
    pitchStartSlider->setRange (40, 250);
    pitchStartSlider->setValue (audioProcessor.pitch);
    pitchStartSlider->setBounds (margin, rowY, colW, 28);
    pitchStartSlider->setTextBoxStyle (juce::Slider::TextBoxRight, true, 50, 18);
    pitchStartSlider->addListener (this);
    addAndMakeVisible (pitchStartSlider);

    auto* pitchLabel = new juce::Label ("pitchLbl", "PITCH");
    pitchLabel->setBounds (margin, rowY - 16, colW - 50, 14);
    pitchLabel->setFont (juce::FontOptions (11, juce::Font::bold));
    pitchLabel->setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (pitchLabel);

    auto* pitchVal = new juce::Label ("pitchVal", "Hz");
    pitchVal->setBounds (margin, rowY + 30, colW, 14);
    pitchVal->setFont (juce::FontOptions (9));
    pitchVal->setColour (juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible (pitchVal);

    pitchDecaySlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxRight);
    pitchDecaySlider->setRange (0.05, 0.5);
    pitchDecaySlider->setValue (audioProcessor.pitchDecay);
    pitchDecaySlider->setBounds (margin + colW + 10, rowY, colW, 28);
    pitchDecaySlider->setTextBoxStyle (juce::Slider::TextBoxRight, true, 50, 18);
    pitchDecaySlider->addListener (this);
    addAndMakeVisible (pitchDecaySlider);

    auto* pitchDecLabel = new juce::Label ("pitchDecLbl", "DECAY");
    pitchDecLabel->setBounds (margin + colW + 10, rowY - 16, colW - 50, 14);
    pitchDecLabel->setFont (juce::FontOptions (11, juce::Font::bold));
    pitchDecLabel->setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (pitchDecLabel);

    rowY = 165;
    auto qw = (w - 24) / 4;

    ampAttackSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    ampAttackSlider->setRange (0, 0.05);
    ampAttackSlider->setValue (audioProcessor.ampAttack);
    ampAttackSlider->setBounds (margin, rowY, qw, 24);
    ampAttackSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 40, 16);
    ampAttackSlider->addListener (this);
    addAndMakeVisible (ampAttackSlider);

    ampDecaySlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    ampDecaySlider->setRange (0.05, 1.0);
    ampDecaySlider->setValue (audioProcessor.ampDecay);
    ampDecaySlider->setBounds (margin + qw + 6, rowY, qw, 24);
    ampDecaySlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 40, 16);
    ampDecaySlider->addListener (this);
    addAndMakeVisible (ampDecaySlider);

    ampSustainSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    ampSustainSlider->setRange (0, 1);
    ampSustainSlider->setValue (audioProcessor.ampSustain);
    ampSustainSlider->setBounds (margin + qw * 2 + 12, rowY, qw, 24);
    ampSustainSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 40, 16);
    ampSustainSlider->addListener (this);
    addAndMakeVisible (ampSustainSlider);

    ampReleaseSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxBelow);
    ampReleaseSlider->setRange (0.05, 1.0);
    ampReleaseSlider->setValue (audioProcessor.ampRelease);
    ampReleaseSlider->setBounds (margin + qw * 3 + 18, rowY, qw, 24);
    ampReleaseSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 40, 16);
    ampReleaseSlider->addListener (this);
    addAndMakeVisible (ampReleaseSlider);

    auto* ampRelLabel = new juce::Label ("ampRelLbl", "REL");
    ampRelLabel->setBounds (margin + qw * 3 + 18, rowY + 26, qw, 12);
    ampRelLabel->setFont (juce::FontOptions (9));
    ampRelLabel->setColour (juce::Label::textColourId, juce::Colours::white);
    ampRelLabel->setJustificationType (juce::Justification::centred);
    addAndMakeVisible (ampRelLabel);

    auto* ampTitle = new juce::Label ("ampTitle", "AMP");
    ampTitle->setBounds (margin, rowY - 16, w - 150, 14);
    ampTitle->setFont (juce::FontOptions (11, juce::Font::bold));
    ampTitle->setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (ampTitle);

    rowY = 245;
    driveSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxRight);
    driveSlider->setRange (0.5, 3);
    driveSlider->setValue (audioProcessor.drive);
    driveSlider->setBounds (margin, rowY, colW, 24);
    driveSlider->setTextBoxStyle (juce::Slider::TextBoxRight, true, 50, 18);
    driveSlider->addListener (this);
    addAndMakeVisible (driveSlider);

    auto* driveLabel = new juce::Label ("driveLbl", "DRIVE");
    driveLabel->setBounds (margin, rowY - 16, colW - 50, 14);
    driveLabel->setFont (juce::FontOptions (11, juce::Font::bold));
    driveLabel->setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (driveLabel);

    colorSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxRight);
    colorSlider->setRange (0, 1);
    colorSlider->setValue (audioProcessor.color);
    colorSlider->setBounds (margin + colW + 10, rowY, colW, 24);
    colorSlider->setTextBoxStyle (juce::Slider::TextBoxRight, true, 50, 18);
    colorSlider->addListener (this);
    addAndMakeVisible (colorSlider);

    auto* colorLabel = new juce::Label ("colorLbl", "COLOR");
    colorLabel->setBounds (margin + colW + 10, rowY - 16, colW - 50, 14);
    colorLabel->setFont (juce::FontOptions (11, juce::Font::bold));
    colorLabel->setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (colorLabel);

    rowY = 305;
    clickSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxRight);
    clickSlider->setRange (0, 1);
    clickSlider->setValue (audioProcessor.click);
    clickSlider->setBounds (margin, rowY, colW, 24);
    clickSlider->setTextBoxStyle (juce::Slider::TextBoxRight, true, 50, 18);
    clickSlider->addListener (this);
    addAndMakeVisible (clickSlider);

    auto* clickLabel = new juce::Label ("clickLbl", "CLICK");
    clickLabel->setBounds (margin, rowY - 16, colW - 50, 14);
    clickLabel->setFont (juce::FontOptions (11, juce::Font::bold));
    clickLabel->setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (clickLabel);

    depthSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxRight);
    depthSlider->setRange (0, 1);
    depthSlider->setValue (audioProcessor.depth);
    depthSlider->setBounds (margin + colW + 10, rowY, colW, 24);
    depthSlider->setTextBoxStyle (juce::Slider::TextBoxRight, true, 50, 18);
    depthSlider->addListener (this);
    addAndMakeVisible (depthSlider);

    auto* depthLabel = new juce::Label ("depthLbl", "DEPTH");
    depthLabel->setBounds (margin + colW + 10, rowY - 16, colW - 50, 14);
    depthLabel->setFont (juce::FontOptions (11, juce::Font::bold));
    depthLabel->setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (depthLabel);

    rowY = 365;
    gainSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxRight);
    gainSlider->setRange (0, 1.5);
    gainSlider->setValue (audioProcessor.gain);
    gainSlider->setBounds (margin, rowY, w, 24);
    gainSlider->setTextBoxStyle (juce::Slider::TextBoxRight, true, 60, 18);
    gainSlider->addListener (this);
    addAndMakeVisible (gainSlider);

    auto* gainLabel = new juce::Label ("gainLbl", "GAIN");
    gainLabel->setBounds (margin, rowY - 16, w - 60, 14);
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
    int meterLevel = (int)((peak < 0.001f ? 0 : (y + 5 + meterH - peak * meterH)));
    meterLevel = juce::jlimit (y + 5, y + 5 + meterH, meterLevel);
    
    if (db >= -0.5f)
        g.setColour (juce::Colours::red);
    else if (db >= -6.0f)
        g.setColour (juce::Colours::orange);
    else if (db >= -12.0f)
        g.setColour (juce::Colours::yellow);
    else
        g.setColour (juce::Colours::green);
    
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