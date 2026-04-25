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
    setSize (520, 620);
    
    auto resourceDir = juce::File::getSpecialLocation (juce::File::invokedExecutableFile)
                        .getParentDirectory().getChildFile ("Resources");
    auto htmlFile = resourceDir.getChildFile ("ui").getChildFile ("index.html");
    
    if (htmlFile.existsAsFile()) {
        browser = std::make_unique<juce::WebBrowserComponent> (juce::WebBrowserComponent::Options{});
        addAndMakeVisible (browser.get());
        
        useWebView = true;
        browser->goToURL (htmlFile.getFullPathName());
    } else {
        useWebView = false;
        auto margin = 25;
        auto w = getWidth() - margin * 2;

        kickButton = new juce::TextButton ("KICK");
        kickButton->setBounds (margin, margin, 70, 32);
        kickButton->addListener (this);
        addAndMakeVisible (kickButton);

        loopButton = new juce::ToggleButton ("LOOP");
        loopButton->setBounds (margin + 80, margin, 70, 32);
        loopButton->addListener (this);
        addAndMakeVisible (loopButton);

        bpmSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        bpmSlider->setRange (80, 250);
        bpmSlider->setValue (audioProcessor.bpm);
        bpmSlider->setBounds (margin + 160, margin + 4, 80, 24);
        bpmSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 50, 18);
        bpmSlider->addListener (this);
        addAndMakeVisible (bpmSlider);

        auto y = margin + 55;
        waveformCombo = new juce::ComboBox ("wave");
        waveformCombo->addItem ("Sine", 1);
        waveformCombo->addItem ("Triangle", 2);
        waveformCombo->addItem ("Saw", 3);
        waveformCombo->addItem ("Square", 4);
        waveformCombo->addItem ("Loaded", 5);
        waveformCombo->setSelectedId (1);
        waveformCombo->setBounds (margin, y, w, 28);
        waveformCombo->addListener (this);
        addAndMakeVisible (waveformCombo);

        auto sectionY = margin + 105;
        auto colW = (w - 15) / 2;
        auto pitchY = sectionY;

        pitchStartSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        pitchStartSlider->setRange (40, 250);
        pitchStartSlider->setValue (audioProcessor.pitch);
        pitchStartSlider->setBounds (margin, pitchY + 20, colW, 28);
        pitchStartSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 60, 20);
        pitchStartSlider->addListener (this);
        addAndMakeVisible (pitchStartSlider);

        pitchDecaySlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        pitchDecaySlider->setRange (0.05, 0.5);
        pitchDecaySlider->setValue (audioProcessor.pitchDecay);
        pitchDecaySlider->setBounds (margin + colW + 10, pitchY + 20, colW, 28);
        pitchDecaySlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 60, 20);
        pitchDecaySlider->addListener (this);
        addAndMakeVisible (pitchDecaySlider);

        auto ampY = pitchY + 80;
        auto qw = (w - 30) / 4;
        
        ampAttackSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        ampAttackSlider->setRange (0, 0.05);
        ampAttackSlider->setValue (audioProcessor.ampAttack);
        ampAttackSlider->setBounds (margin, ampY + 20, qw, 28);
        ampAttackSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 50, 20);
        ampAttackSlider->addListener (this);
        addAndMakeVisible (ampAttackSlider);

        ampDecaySlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        ampDecaySlider->setRange (0.05, 1.0);
        ampDecaySlider->setValue (audioProcessor.ampDecay);
        ampDecaySlider->setBounds (margin + qw + 5, ampY + 20, qw, 28);
        ampDecaySlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 50, 20);
        ampDecaySlider->addListener (this);
        addAndMakeVisible (ampDecaySlider);

        ampSustainSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        ampSustainSlider->setRange (0, 1);
        ampSustainSlider->setValue (audioProcessor.ampSustain);
        ampSustainSlider->setBounds (margin + qw * 2 + 10, ampY + 20, qw, 28);
        ampSustainSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 50, 20);
        ampSustainSlider->addListener (this);
        addAndMakeVisible (ampSustainSlider);

        ampReleaseSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        ampReleaseSlider->setRange (0.05, 1.0);
        ampReleaseSlider->setValue (audioProcessor.ampRelease);
        ampReleaseSlider->setBounds (margin + qw * 3 + 15, ampY + 20, qw, 28);
        ampReleaseSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 50, 20);
        ampReleaseSlider->addListener (this);
        addAndMakeVisible (ampReleaseSlider);

        auto toneY = ampY + 100;
        driveSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        driveSlider->setRange (0.5, 3);
        driveSlider->setValue (audioProcessor.drive);
        driveSlider->setBounds (margin, toneY + 20, colW, 28);
        driveSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 60, 20);
        driveSlider->addListener (this);
        addAndMakeVisible (driveSlider);

        colorSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        colorSlider->setRange (0, 1);
        colorSlider->setValue (audioProcessor.color);
        colorSlider->setBounds (margin + colW + 10, toneY + 20, colW, 28);
        colorSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 60, 20);
        colorSlider->addListener (this);
        addAndMakeVisible (colorSlider);

        auto clickY = toneY + 80;
        clickSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        clickSlider->setRange (0, 1);
        clickSlider->setValue (audioProcessor.click);
        clickSlider->setBounds (margin, clickY + 20, colW, 28);
        clickSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 60, 20);
        clickSlider->addListener (this);
        addAndMakeVisible (clickSlider);

        depthSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        depthSlider->setRange (0, 1);
        depthSlider->setValue (audioProcessor.depth);
        depthSlider->setBounds (margin + colW + 10, clickY + 20, colW, 28);
        depthSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 60, 20);
        depthSlider->addListener (this);
        addAndMakeVisible (depthSlider);

        auto gainY = clickY + 80;
        gainSlider = new juce::Slider (juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxLeft);
        gainSlider->setRange (0, 1.5);
        gainSlider->setValue (audioProcessor.gain);
        gainSlider->setBounds (margin, gainY + 20, w, 28);
        gainSlider->setTextBoxStyle (juce::Slider::TextBoxLeft, true, 60, 20);
        gainSlider->addListener (this);
        addAndMakeVisible (gainSlider);
    }
    
    startTimer (50);
}

KikAudioProcessorEditor::~KikAudioProcessorEditor()
{
    stopTimer();
}

void KikAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (useWebView && browser) {
        return;
    }
    
    g.fillAll (juce::Colours::black);
    
    auto& waveform = audioProcessor.previewWaveform;
    
    auto w = getWidth() - 50;
    auto h = 90;
    auto y = 470;
    auto margin = 25;
    auto meterX = margin + w + 15;
    auto meterW = 16;
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
    g.setFont (juce::Font (9));
    g.drawFittedText ("0", margin + 2, (int)(y + 4), 20, 10, juce::Justification::left, 1);
    g.drawFittedText ("-6", margin + 2, (int)(y + h*0.33f), 25, 10, juce::Justification::left, 1);
    g.drawFittedText ("-12", margin + 2, (int)(y + h*0.55f), 30, 10, juce::Justification::left, 1);
    g.drawFittedText ("-24", margin + 2, (int)(y + h*0.8f), 30, 10, juce::Justification::left, 1);
    
    float peak = audioProcessor.peakLevel;
    float db = (peak > 0.0001f) ? (20.0f * std::log10 (peak)) : -60.0f;
    int meterLevel = (peak < 0.001f ? 0 : (y + 5 + meterH - (int)(peak * meterH)));
    meterLevel = juce::jlimit (y + 5, y + 5 + meterH, meterLevel);
    
    if (db >= -0.5f) g.setColour (juce::Colours::red);
    else if (db >= -6.0f) g.setColour (juce::Colours::orange);
    else if (db >= -12.0f) g.setColour (juce::Colours::yellow);
    else g.setColour (juce::Colours::green);
    
    g.fillRect ((float)meterX, (float)meterLevel, (float)meterW, (float)(y + 5 + meterH - meterLevel));
    
    char buf[20];
    std::snprintf (buf, sizeof(buf), "%.1f dB", db);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (11));
    g.drawFittedText (buf, meterX - 5, y + h + 5, meterW + 20, 16, juce::Justification::centred, 1);
}

void KikAudioProcessorEditor::resized() 
{
    if (browser) browser->setBounds (getBounds());
}

void KikAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.previewDirty) {
        audioProcessor.updatePreview();
        repaint();
    }
}

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