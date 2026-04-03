#include <optional>
#include "LiveFlowAudioProcessorEditor.h"
#include "LiveFlowAudioProcessor.h"
#include "core/PluginParameters.h"
#include <BinaryData.h>

namespace liveflow
{
juce::WebBrowserComponent::Options LiveFlowAudioProcessorEditor::createWebOptions()
{
    auto paramCallback = [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
    {
        if (args.size() >= 2)
        {
            auto paramId = args[0].toString();
            auto value = static_cast<float>(args[1]);
            if (auto* param = processor.getValueTreeState().getParameter(paramId))
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost(param->convertTo0to1(value));
                param->endChangeGesture();
            }
        }
        completion (juce::var{});
    };

    auto stateCallback = [this] (const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion completion)
    {
        sendInitialStateParams();
        completion (juce::var{});
    };

    auto resProvider = [] (const auto& url) -> std::optional<juce::WebBrowserComponent::Resource>
    {
        juce::URL parsedUrl (url);
        juce::String file = parsedUrl.getSubPath();
        if (file.isEmpty() || file == "/") file = "index.html";
        
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            juce::String originalName = BinaryData::getNamedResourceOriginalFilename(BinaryData::namedResourceList[i]);
            if (originalName.endsWithIgnoreCase (file) || file.endsWithIgnoreCase(originalName))
            {
                int size = 0;
                if (auto* data = BinaryData::getNamedResource(BinaryData::namedResourceList[i], size))
                {
                    auto ext = file.substring(file.lastIndexOfChar('.')).toLowerCase();
                    auto mime = (ext == ".html" ? "text/html" : (ext == ".js" ? "text/javascript" : (ext == ".css" ? "text/css" : (ext == ".svg" ? "image/svg+xml" : "application/octet-stream"))));
                    const std::byte* byteData = reinterpret_cast<const std::byte*>(data);
                    return juce::WebBrowserComponent::Resource { std::vector<std::byte>(byteData, byteData + size), mime };
                }
            }
        }
        return std::nullopt;
    };

    auto options = juce::WebBrowserComponent::Options()
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2()
                                     .withUserDataFolder (juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("LiveFlowWebData")))
        .withNativeFunction ("juceSetParameter", paramCallback)
        .withNativeFunction ("juceRequestState", stateCallback);

    return options.withResourceProvider (resProvider);
}

LiveFlowAudioProcessorEditor::LiveFlowAudioProcessorEditor (LiveFlowAudioProcessor& processorToEdit)
    : juce::AudioProcessorEditor (&processorToEdit),
      processor (processorToEdit),
      webBrowser (createWebOptions())
{
    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (760, 520, 1200, 800);
    setSize (880, 600);

    addAndMakeVisible (webBrowser);

#if JUCE_DEBUG
    webBrowser.goToURL ("http://localhost:5173");
#else
    webBrowser.goToURL (webBrowser.getResourceProviderRoot());
#endif

    startTimerHz (30);
}

LiveFlowAudioProcessorEditor::~LiveFlowAudioProcessorEditor()
{
    stopTimer();
}

void LiveFlowAudioProcessorEditor::timerCallback()
{
    const auto snap = processor.getVisualizationState().capture();
    
    // Create a JSON object representing the DSP snapshot
    juce::DynamicObject::Ptr jsonState = new juce::DynamicObject();
    jsonState->setProperty("totalGainDb", snap.gainDb);
    jsonState->setProperty("vocalLufs", snap.vocalLufs);
    jsonState->setProperty("backingLufs", snap.backingLufs);
    jsonState->setProperty("voicePresence", snap.voicePresence);
    jsonState->setProperty("heldVocalLevel", snap.heldVocalLevel);
    jsonState->setProperty("anchorPoint", snap.anchorPoint);
    jsonState->setProperty("sidechainConnected", snap.sidechainConnected);

    // Trail ring buffer (90 points of {vocalLufs, backingLufs})
    juce::Array<juce::var> trailArray;
    for (size_t i = 0; i < snap.trail.size(); ++i)
    {
        juce::Array<juce::var> point;
        point.add (snap.trail[i].vocalLufs);
        point.add (snap.trail[i].backingLufs);
        trailArray.add (juce::var (point));
    }
    jsonState->setProperty("trail", trailArray);
    jsonState->setProperty("trailHeadIndex", snap.trailHeadIndex);

    // Gain history ring buffer (360 values)
    juce::Array<juce::var> gainArray;
    for (size_t i = 0; i < snap.gainHistory.size(); ++i)
        gainArray.add (snap.gainHistory[i]);
    jsonState->setProperty("gainHistory", gainArray);
    jsonState->setProperty("gainHeadIndex", snap.gainHeadIndex);

    // Call JS to update state
    auto jsonString = juce::JSON::toString(juce::var(jsonState));
    webBrowser.evaluateJavascript("if (window.updateDSPState) window.updateDSPState(" + jsonString + ");");
}

void LiveFlowAudioProcessorEditor::sendInitialStateParams()
{
    auto& apvts = processor.getValueTreeState();
    juce::DynamicObject::Ptr jsonParams = new juce::DynamicObject();
    
    for (const auto* paramName : {
        param::balance, param::dynamics, param::speed, param::range, 
        param::anchorManual, param::noiseGate, param::duckFloor, 
        param::boostCeiling, param::expansionMode, param::anchorAuto,
        param::lookAhead, param::presenceRelease
    })
    {
        if (auto* p = apvts.getRawParameterValue(paramName))
            jsonParams->setProperty(paramName, p->load());
    }

    auto jsonString = juce::JSON::toString(juce::var(jsonParams.get()));
    webBrowser.evaluateJavascript("if (window.updateDSPParams) window.updateDSPParams(" + jsonString + ");");
}

void LiveFlowAudioProcessorEditor::resized()
{
    webBrowser.setBounds (getLocalBounds());
}
} // namespace liveflow
