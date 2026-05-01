/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

juce::String makeInitJS(KikAudioProcessor& ap)
{
    juce::String js = "if (window.updateUI) { updateUI({";
    js += "waveform:" + juce::String((int)ap.currentSource + 1) + ",";
    js += "pitch:" + juce::String(ap.pitch) + ",";
    js += "pitchDecay:" + juce::String(ap.pitchDecay) + ",";
    js += "ampAttack:" + juce::String(ap.ampAttack) + ",";
    js += "ampDecay:" + juce::String(ap.ampDecay) + ",";
    js += "ampSustain:" + juce::String(ap.ampSustain) + ",";
    js += "ampRelease:" + juce::String(ap.ampRelease) + ",";
    js += "drive:" + juce::String(ap.drive) + ",";
    js += "color:" + juce::String(ap.color) + ",";
    js += "click:" + juce::String(ap.click) + ",";
    js += "clickPitch:" + juce::String(ap.clickPitch) + ",";
    js += "depth:" + juce::String(ap.depth) + ",";
    js += "gain:" + juce::String(ap.gain) + ",";
    js += "bpm:" + juce::String(ap.bpm) + ",";
    js += "loopEnabled:" + juce::String(ap.loopEnabled ? "true" : "false") + ",";
    js += "isStandalone:" + juce::String((ap.wrapperType == juce::AudioProcessor::wrapperType_Standalone) ? "true" : "false");
    js += "}); }";
    return js;
}

juce::String makeMeterJS(float peakLevel)
{
    float db = (peakLevel > 0.0001f) ? (20.0f * std::log10 (peakLevel)) : -60.0f;
    return "if (window.updateMeter) { updateMeter(" + juce::String(db) + "); }";
}

juce::String makeBridgeJS()
{
    return R"(
        (function() {
            window.__bridgeReady__ = true;
        })()
    )";
}

juce::String makeCheckReadyJS()
{
    return R"(
        (function() {
            if (typeof window.updateUI === 'function' && typeof window.juce !== 'undefined') {
                return 'READY';
            }
            return 'NOT_READY';
        })()
    )";
}

KikAudioProcessorEditor::KikAudioProcessorEditor (KikAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (520, 560);
    
    File exeFile = File::getSpecialLocation (File::invokedExecutableFile);
    File resourcesDir = exeFile.getParentDirectory().getParentDirectory().getChildFile("Resources");
    String resourcesPath = resourcesDir.getFullPathName();
    
    File htmlFile = resourcesDir.getChildFile("index.html");
    
    if (htmlFile.existsAsFile()) {
        auto self = this;
        
        auto resourceRoot = juce::WebBrowserComponent::getResourceProviderRoot();
        
        browser = std::make_unique<juce::WebBrowserComponent> (juce::WebBrowserComponent::Options{}
            .withResourceProvider ([resourcesPath](const String& path) -> std::optional<juce::WebBrowserComponent::Resource> {
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
            .withNativeFunction ("JUCE_CALLBACK", 
                [self](const juce::Array<juce::var>& args, auto completion) {
                    Logger::writeToLog ("KIK: native callback args=" + String(args.size()));
                    if (args.size() >= 1) {
                        auto param = args[0].toString();
                        Logger::writeToLog ("KIK: param=" + param);
                        if (args.size() >= 2) {
                            auto value = (double)args[1];
                            self->onParameterChange(param, value);
                        }
                    }
                    completion(juce::var());
                }));    
        
        addAndMakeVisible (browser.get());
        browser->setBounds (getLocalBounds());
        setOpaque (false);
        
        useWebView = true;
        bridgeInjected = false;
        browser->goToURL (resourceRoot);
    }
    
    startTimerHz (20);
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
    if (useWebView && browser) {
        if (webViewReady) {
            sendMeterToWeb();
            sendWaveformToWeb();
            
            if (audioProcessor.midiTriggered.exchange(false)) {
                browser->evaluateJavascript("if(window.blinkKick) blinkKick();", nullptr);
            }
        }
        
        // Always poll for pending parameters
        browser->evaluateJavascript (
            R"((function() {
                var q = window.__paramQueue__;
                var res = '';
                if (q && q.length > 0) {
                    while (q.length > 0) {
                        var item = q.shift();
                        res += item.param + ':' + item.value + '|';
                    }
                }
                return res;
            })())",
            [this](juce::WebBrowserComponent::EvaluationResult result) {
                auto* r = result.getResult();
                if (r != nullptr) {
                    String s = r->toString();
                    if (!s.isEmpty()) {
                        StringArray tokens;
                        tokens.addTokens (s, "|", "");
                        for (auto t : tokens) {
                            int colonPos = t.indexOfChar(':');
                            if (colonPos > 0) {
                                String param = t.substring(0, colonPos);
                                double value = t.substring(colonPos + 1).getDoubleValue();
                                Logger::writeToLog ("KIK: got param=" + param + " value=" + String(value));
                                onParameterChange(param, value);
                            }
                        }
                    }
                }
            });
        
        if (!webViewReady) {
            browser->evaluateJavascript (makeCheckReadyJS(), 
                [this](juce::WebBrowserComponent::EvaluationResult result) {
                    auto* r = result.getResult();
                    if (r != nullptr && r->toString() == "READY") {
                        webViewReady = true;
                        sendInitValuesToWeb();
                    }
                });
        }
    }
}

void KikAudioProcessorEditor::sendInitValuesToWeb()
{
    if (!browser)
        return;
    audioProcessor.previewDirty = true; // Force preview generation on load
    browser->evaluateJavascript (makeInitJS(audioProcessor), nullptr);
}

void KikAudioProcessorEditor::sendMeterToWeb()
{
    if (!browser)
        return;
    browser->evaluateJavascript (makeMeterJS(audioProcessor.peakLevel), nullptr);
}

void KikAudioProcessorEditor::sendWaveformToWeb()
{
    if (!browser)
        return;
        
    if (audioProcessor.previewDirty) {
        audioProcessor.updatePreview();
        
        juce::String js = "if (window.updateWaveform) { updateWaveform([";
        
        int numPoints = 450;
        if (audioProcessor.previewWaveform.size() > 0) {
            for (int i = 0; i < numPoints; ++i) {
                int idx = (i * audioProcessor.previewWaveform.size()) / numPoints;
                if (idx >= audioProcessor.previewWaveform.size()) idx = audioProcessor.previewWaveform.size() - 1;
                
                js += juce::String(audioProcessor.previewWaveform[idx]);
                if (i < numPoints - 1) js += ",";
            }
        }
        
        js += "]); }";
        browser->evaluateJavascript (js, nullptr);
    }
}

void KikAudioProcessorEditor::onParameterChange (const juce::String& param, double value)
{
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
    
    if (param == "pitch") {
        audioProcessor.pitch = (float)value;
        audioProcessor.previewDirty = true;
    }
    else if (param == "pitchDecay") {
        audioProcessor.pitchDecay = (float)value;
        audioProcessor.previewDirty = true;
    }
    else if (param == "ampAttack") {
        audioProcessor.ampAttack = (float)value;
    }
    else if (param == "ampDecay") {
        audioProcessor.ampDecay = (float)value;
    }
    else if (param == "ampSustain") {
        audioProcessor.ampSustain = (float)value;
    }
    else if (param == "ampRelease") {
        audioProcessor.ampRelease = (float)value;
    }
    else if (param == "drive") {
        audioProcessor.drive = (float)value;
        audioProcessor.previewDirty = true;
    }
    else if (param == "color") {
        audioProcessor.color = (float)value;
        audioProcessor.previewDirty = true;
    }
    else if (param == "click") {
        audioProcessor.click = (float)value;
        audioProcessor.previewDirty = true;
    }
    else if (param == "clickPitch") {
        audioProcessor.clickPitch = (float)value;
        audioProcessor.previewDirty = true;
    }
    else if (param == "depth") {
        audioProcessor.depth = (float)value;
        audioProcessor.previewDirty = true;
    }
    else if (param == "gain") {
        audioProcessor.gain = (float)value;
    }
    else if (param == "bpm") {
        audioProcessor.bpm = (float)value;
    }
    else if (param == "waveform") {
        auto id = (int)value;
        audioProcessor.currentSource = (KikAudioProcessor::WaveformSource)(id - 1);
        audioProcessor.previewDirty = true;
    }
    else if (param == "trigger") {
        audioProcessor.shouldTrigger = true;
    }
    else if (param == "loop") {
        audioProcessor.loopEnabled = (value != 0);
        audioProcessor.samplesSinceTrigger = 0;
    }
}