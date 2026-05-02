/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>

using namespace juce;

namespace
{
    File getUserPresetFolder()
    {
        auto dir = File::getSpecialLocation (File::userDocumentsDirectory)
                       .getChildFile ("KIK")
                       .getChildFile ("Presets");
        dir.createDirectory();
        return dir;
    }

    File withKikExtension (File f)
    {
        return f.hasFileExtension ("kik") ? f : f.withFileExtension ("kik");
    }
}

juce::String makeInitJS(KikAudioProcessor& ap)
{
    auto obj = std::make_unique<juce::DynamicObject>();
    
    auto getVal = [&](juce::String id) {
        return (double)*ap.apvts.getRawParameterValue(id);
    };

    obj->setProperty("waveform", (int)getVal("waveform") + 1);
    obj->setProperty("pitch", getVal("pitch"));
    obj->setProperty("pitchDecay", getVal("pitchDecay"));
    if (auto* kn = dynamic_cast<juce::AudioParameterInt*> (ap.apvts.getParameter ("keyNote")))
        obj->setProperty ("keyNote", kn->get());
    else
        obj->setProperty ("keyNote", 7);
    if (auto* ko = dynamic_cast<juce::AudioParameterInt*> (ap.apvts.getParameter ("keyOctave")))
        obj->setProperty ("keyOctave", ko->get());
    else
        obj->setProperty ("keyOctave", 3);
    obj->setProperty("ampAttack", getVal("ampAttack"));
    obj->setProperty("ampDecay", getVal("ampDecay"));
    obj->setProperty("ampSustain", getVal("ampSustain"));
    obj->setProperty("ampRelease", getVal("ampRelease"));
    obj->setProperty("drive", getVal("drive"));
    obj->setProperty("color", getVal("color"));
    obj->setProperty("waveMorph", getVal("waveMorph"));
    obj->setProperty("click", getVal("click"));
    obj->setProperty("clickPitch", getVal("clickPitch"));
    obj->setProperty("depth", getVal("depth"));
    obj->setProperty("hpCutoff", getVal("hpCutoff"));
    obj->setProperty("lpCutoff", getVal("lpCutoff"));
    obj->setProperty("gain", getVal("gain"));
    obj->setProperty("enhancer", getVal("enhancer"));
    obj->setProperty("bpm", getVal("bpm"));
    obj->setProperty("loopEnabled", (bool)*ap.apvts.getRawParameterValue("loopEnabled"));
    obj->setProperty("isStandalone", (ap.wrapperType == juce::AudioProcessor::wrapperType_Standalone));
    obj->setProperty("factoryPreset", ap.getFactoryPresetIndexForUi());
    obj->setProperty("presetDisplayName", ap.getPresetDisplayName());

    return juce::JSON::toString(juce::var(obj.release()));
}

void KikAudioProcessorEditor::sendEventToWeb (const juce::String& eventName, const juce::var& data)
{
    if (!browser || !webViewReady)
        return;
    
    juce::var eventData;
    if (data.isObject())
        eventData = data;
    else
        eventData = data;
    
    browser->emitEventIfBrowserIsVisible (eventName, eventData);
}

KikAudioProcessorEditor::KikAudioProcessorEditor (KikAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (KikEditorLayout::width, KikEditorLayout::height);
    
    File exeFile = File::getSpecialLocation (File::invokedExecutableFile);
    auto appRoot = exeFile.getParentDirectory().getParentDirectory();
    auto resourcesRoot = appRoot.getChildFile ("Resources");

    std::array<File, 4> resourceCandidates {
        resourcesRoot.getChildFile ("ui"),
        resourcesRoot,
        appRoot.getChildFile ("ui"),
        appRoot
    };

    File resourcesDir;
    File htmlFile;
    for (const auto& candidate : resourceCandidates)
    {
        auto candidateHtml = candidate.getChildFile ("index.html");
        if (candidateHtml.existsAsFile())
        {
            resourcesDir = candidate;
            htmlFile = candidateHtml;
            break;
        }
    }

    resourcesPath = resourcesDir.getFullPathName();
    
    if (htmlFile.existsAsFile()) {
        auto resourceRoot = juce::WebBrowserComponent::getResourceProviderRoot();
        
        browser = std::make_unique<juce::WebBrowserComponent> (juce::WebBrowserComponent::Options{}
            .withResourceProvider ([this](const String& path) -> std::optional<juce::WebBrowserComponent::Resource> {
                String filePath = resourcesPath;
                if (path == "/" || path.isEmpty()) {
                    filePath += "/index.html";
                } else {
                    filePath += (path.startsWith("/") ? path : "/" + path);
                }
                File f(filePath);
                if (f.existsAsFile()) {
                    juce::MemoryBlock mb;
                    f.loadFileAsData(mb);
                    String mimeType = "text/html";
                    if (path.endsWith(".css")) mimeType = "text/css";
                    else if (path.endsWith(".js")) mimeType = "application/javascript";
                    juce::WebBrowserComponent::Resource r;
                    r.data = std::vector<std::byte>((std::byte*)mb.getData(), (std::byte*)mb.getData() + mb.getSize());
                    r.mimeType = mimeType;
                    return r;
                }
                return std::nullopt;
            })
            .withNativeIntegrationEnabled(true)
            .withEventListener ("juceEvent", [this](const juce::var& eventData) {
                handleJuceEvent (eventData);
            })
            .withEventListener ("parameterChange", [this](const juce::var& eventData) {
                if (eventData.isObject()) {
                    juce::String param = eventData.getProperty ("param", "");
                    double value = eventData.getProperty ("value", 0.0);
                    onParameterChange (param, value);
                }
            })
            .withEventListener ("savePreset", [this](const juce::var& eventData) {
                saveStateToFile();
            })
            .withEventListener ("loadPreset", [this](const juce::var& eventData) {
                loadStateToFile();
            })
            .withEventListener ("openURL", [this](const juce::var& eventData) {
                if (eventData.isString())
                    juce::URL(eventData.toString()).launchInDefaultBrowser();
            })
            .withEventListener ("trigger", [this](const juce::var& eventData) {
                audioProcessor.shouldTrigger = true;
            })
            .withEventListener ("uiReady", [this](const juce::var& eventData) {
                webViewReady = true;
                sendInitValuesToWeb();
            }));
        
        addAndMakeVisible (browser.get());
        browser->setBounds (getLocalBounds());
        setOpaque (false);
        
        useWebView = true;
        browser->goToURL (resourceRoot);
    } else {
        Logger::writeToLog ("KIK: UI index.html not found. Checked: "
            + resourceCandidates[0].getFullPathName() + ", "
            + resourceCandidates[1].getFullPathName() + ", "
            + resourceCandidates[2].getFullPathName() + ", "
            + resourceCandidates[3].getFullPathName());
    }
    
    startTimerHz (20);
}

void KikAudioProcessorEditor::handleJuceEvent (const juce::var& eventData)
{
    if (!eventData.isObject())
        return;
    
    juce::String type = eventData.getProperty ("type", "");
    juce::var data = eventData.getProperty ("data", juce::var());
    
    if (type == "parameterChange") {
        juce::String param = data.getProperty ("param", "");
        double value = data.getProperty ("value", 0.0);
        onParameterChange (param, value);
    } else if (type == "savePreset") {
        saveStateToFile();
    } else if (type == "loadPreset") {
        loadStateToFile();
    } else if (type == "openURL") {
        if (data.isString())
            juce::URL(data.toString()).launchInDefaultBrowser();
    } else if (type == "trigger") {
        audioProcessor.shouldTrigger = true;
    } else if (type == "uiReady") {
        webViewReady = true;
        sendInitValuesToWeb();
    }
}

KikAudioProcessorEditor::~KikAudioProcessorEditor()
{
    stopTimer();
}

void KikAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (!useWebView || !browser) {
        g.fillAll (juce::Colours::black);
        return;
    }
    
    g.fillAll (juce::Colours::white);
}

void KikAudioProcessorEditor::resized() 
{
    if (browser)
        browser->setBounds (getLocalBounds());
}

void KikAudioProcessorEditor::timerCallback()
{
    if (useWebView && browser && webViewReady) {
        sendMeterToWeb();
        sendWaveformToWeb();

        const auto presetVer = audioProcessor.getPresetStateVersion();
        if (presetVer != lastSeenPresetStateVersion)
            sendInitValuesToWeb();

        if (audioProcessor.midiTriggered.exchange(false)) {
            sendEventToWeb ("blinkKick", juce::var());
        }
    }
}

void KikAudioProcessorEditor::sendInitValuesToWeb()
{
    if (!browser || !webViewReady)
        return;
    
    audioProcessor.previewDirty = true;
    lastSeenPresetStateVersion = audioProcessor.getPresetStateVersion();
    
    juce::var initData = juce::JSON::parse (makeInitJS(audioProcessor));
    sendEventToWeb ("initUI", initData);
}

void KikAudioProcessorEditor::sendMeterToWeb()
{
    if (!browser || !webViewReady)
        return;
    
    float peakLevel = audioProcessor.peakLevel;
    float db = (peakLevel > 0.0001f) ? (20.0f * std::log10 (peakLevel)) : -60.0f;
    
    juce::var meterData;
    meterData = db;
    sendEventToWeb ("meterUpdate", meterData);
}

void KikAudioProcessorEditor::sendWaveformToWeb()
{
    if (!browser || !webViewReady)
        return;
        
    if (audioProcessor.previewDirty) {
        audioProcessor.updatePreview();
        
        juce::Array<juce::var> waveformArray;
        int numPoints = 450;
        if (audioProcessor.previewWaveform.size() > 0) {
            for (int i = 0; i < numPoints; ++i) {
                int idx = (i * audioProcessor.previewWaveform.size()) / numPoints;
                if (idx >= audioProcessor.previewWaveform.size()) idx = audioProcessor.previewWaveform.size() - 1;
                waveformArray.add (audioProcessor.previewWaveform[idx]);
            }
        }
        
        sendEventToWeb ("waveformUpdate", waveformArray);
    }
}

void KikAudioProcessorEditor::saveStateToFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Save KIK preset (.kik)",
                                                   getUserPresetFolder(),
                                                   "*.kik");

    auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == File())
            return;

        file = withKikExtension (std::move (file));

        MemoryBlock mb;
        audioProcessor.getStateInformation (mb);

        if (! file.replaceWithData (mb.getData(), mb.getSize()))
        {
            Logger::writeToLog ("KIK: failed to save preset to " + file.getFullPathName());
            return;
        }

        auto savedName = file.getFileNameWithoutExtension();

        MessageManager::callAsync ([this, savedName]()
                                   {
                                       audioProcessor.setPresetDisplayName (savedName);
                                       sendInitValuesToWeb();
                                   });
    });
}

void KikAudioProcessorEditor::loadStateToFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Load KIK preset (.kik)",
                                                   getUserPresetFolder(),
                                                   "*.kik");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (! file.existsAsFile())
            return;

        MemoryBlock mb;
        if (! file.loadFileAsData (mb))
        {
            Logger::writeToLog ("KIK: failed to read preset " + file.getFullPathName());
            return;
        }

        MemoryBlock dataCopy (mb);
        auto loadedName = file.getFileNameWithoutExtension();

        MessageManager::callAsync ([this, dataCopy, loadedName]()
                                   {
                                       audioProcessor.setStateInformation (dataCopy.getData(), (int) dataCopy.getSize());
                                       audioProcessor.setPresetDisplayName (loadedName.isEmpty() ? "File" : loadedName);
                                       sendInitValuesToWeb();
                                   });
    });
}

void KikAudioProcessorEditor::onParameterChange (const juce::String& param, double value)
{
    if (param == "savePreset" || param == "savePresetFile")
    {
        saveStateToFile();
        return;
    }

    if (param == "loadPreset" || param == "loadPresetFile")
    {
        loadStateToFile();
        return;
    }

    if (param == "preset") {
        audioProcessor.loadPreset((int)value);
        sendInitValuesToWeb();
        return;
    }

    if (param == "__ready__") {
        webViewReady = true;
        sendInitValuesToWeb();
        return;
    }
    
    if (param == "__bridge_ready__") {
        Logger::writeToLog ("KIK: JavaScript bridge is working!");
        return;
    }
    
    if (param == "__init__") {
        sendInitValuesToWeb();
        return;
    }
    
    if (param == "__meter__") {
        sendMeterToWeb();
        return;
    }

    if (param == "trigger") {
        if (value > 0) audioProcessor.shouldTrigger = true;
        return;
    }

    if (param == "keyNote")
    {
        if (auto* pi = dynamic_cast<juce::AudioParameterInt*> (audioProcessor.apvts.getParameter (param)))
        {
            const int v = juce::jlimit (pi->getRange().getStart(), pi->getRange().getEnd(), juce::roundToInt (value));
            pi->setValueNotifyingHost (pi->convertTo0to1 ((float) v));
        }
        audioProcessor.applyPitchFromKeyNote();
        if (webViewReady)
            sendInitValuesToWeb();
        return;
    }

    if (param == "keyOctave")
    {
        if (auto* pi = dynamic_cast<juce::AudioParameterInt*> (audioProcessor.apvts.getParameter (param)))
        {
            const int v = juce::jlimit (pi->getRange().getStart(), pi->getRange().getEnd(), juce::roundToInt (value));
            pi->setValueNotifyingHost (pi->convertTo0to1 ((float) v));
        }
        audioProcessor.applyPitchFromKeyNote();
        if (webViewReady)
            sendInitValuesToWeb();
        return;
    }

    // Handle standard parameters via APVTS
    if (auto* p = audioProcessor.apvts.getParameter (param))
    {
        float val = (float)value;
        if (param == "waveform") val -= 1.0f; // Map 1-5 to 0-4
        
        p->setValueNotifyingHost (p->getNormalisableRange().convertTo0to1 (val));

        if (param == "pitch")
        {
            audioProcessor.syncKeyNoteFromPitch();
            if (webViewReady)
                sendInitValuesToWeb();
        }

        // Trigger preview update for relevant parameters
        if (param == "pitch" || param == "pitchDecay" || param == "drive" || 
            param == "color" || param == "click" || param == "clickPitch" || 
            param == "depth" || param == "hpCutoff" || param == "lpCutoff" || param == "waveform")
        {
            audioProcessor.previewDirty = true;
        }
    }
}