#include "PluginParameters.h"

namespace
{
juce::NormalisableRange<float> skewedRange (const float start,
                                            const float end,
                                            const float interval,
                                            const float skew) noexcept
{
    auto range = juce::NormalisableRange<float> (start, end, interval);
    range.setSkewForCentre (skew);
    return range;
}

juce::String withUnit (const float value, const juce::String& suffix, const int decimals = 0)
{
    return juce::String (value, decimals) + suffix;
}

juce::String signedDb (const float value)
{
    const auto prefix = value > 0.05f ? "+" : "";
    return prefix + juce::String (value, 1) + " dB";
}

juce::String signedPercent (const float value)
{
    const auto prefix = value > 0.5f ? "+" : "";
    return prefix + juce::String (value, 0) + "%";
}
} // namespace

namespace liveflow
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using Parameter = juce::AudioParameterFloat;
    using BoolParameter = juce::AudioParameterBool;
    using Attributes = juce::AudioParameterFloatAttributes;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    parameters.reserve (16);

    // ── Core Parameters ──

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::balance, 1 },
        "Balance",
        juce::NormalisableRange<float> (-20.0f, 20.0f, 0.1f),
        3.0f,
        Attributes {}
            .withLabel (" dB")
            .withStringFromValueFunction ([] (float value, int) { return signedDb (value); })));

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::dynamics, 1 },
        "Dynamics",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 1.0f),
        0.0f,
        Attributes {}
            .withLabel (" %")
            .withStringFromValueFunction ([] (float value, int) { return signedPercent (value); })));

    parameters.push_back (std::make_unique<BoolParameter> (
        juce::ParameterID { param::expansionMode, 1 },
        "Expansion Mode",
        false));

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::speed, 1 },
        "Speed",
        skewedRange (15.0f, 1500.0f, 1.0f, 200.0f),
        200.0f,
        Attributes {}
            .withLabel (" ms")
            .withStringFromValueFunction ([] (float value, int) { return withUnit (value, " ms"); })));

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::range, 1 },
        "Range",
        juce::NormalisableRange<float> (0.0f, 20.0f, 0.1f),
        2.0f,
        Attributes {}
            .withLabel (" dB")
            .withStringFromValueFunction ([] (float value, int) { return withUnit (value, " dB", 1); })));

    parameters.push_back (std::make_unique<BoolParameter> (
        juce::ParameterID { param::anchorAuto, 1 },
        "Anchor Auto",
        true));

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::anchorManual, 1 },
        "Anchor",
        juce::NormalisableRange<float> (-60.0f, -5.0f, 0.5f),
        -24.0f,
        Attributes {}
            .withLabel (" dB")
            .withStringFromValueFunction ([] (float value, int) { return withUnit (value, " LUFS", 1); })));

    // ── Expert Parameters ──

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::boostCeiling, 1 },
        "Boost Ceiling",
        juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f),
        6.0f,
        Attributes {}
            .withLabel (" dB")
            .withStringFromValueFunction ([] (float value, int) { return withUnit (value, " dB", 1); })));

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::duckFloor, 1 },
        "Duck Floor",
        juce::NormalisableRange<float> (0.0f, 48.0f, 0.1f),
        18.0f,
        Attributes {}
            .withLabel (" dB")
            .withStringFromValueFunction ([] (float value, int) { return withUnit (value, " dB", 1); })));

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::noiseGate, 1 },
        "Noise Gate",
        juce::NormalisableRange<float> (-70.0f, -20.0f, 0.5f),
        -45.0f,
        Attributes {}
            .withLabel (" dB")
            .withStringFromValueFunction ([] (float value, int) { return withUnit (value, " dB", 1); })));

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::lookAhead, 1 },
        "Look Ahead",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.5f),
        0.0f,
        Attributes {}
            .withLabel (" ms")
            .withStringFromValueFunction ([] (float value, int) { return withUnit (value, " ms", 1); })));

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::presenceRelease, 1 },
        "Presence Release",
        skewedRange (30.0f, 300.0f, 1.0f, 120.0f),
        120.0f,
        Attributes {}
            .withLabel (" ms")
            .withStringFromValueFunction ([] (float value, int) { return withUnit (value, " ms"); })));

    // ── Smart Track Profiler Parameters ──

    parameters.push_back (std::make_unique<BoolParameter> (
        juce::ParameterID { param::profileActive, 1 },
        "Profile Active",
        false));

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::profileMaxMinutes, 1 },
        "Max Record",
        juce::NormalisableRange<float> (1.0f, 10.0f, 1.0f),
        5.0f,
        Attributes {}
            .withLabel (" min")
            .withStringFromValueFunction ([] (float value, int) { return withUnit (value, " min"); })));

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::profileNumZones, 1 },
        "Zones",
        juce::NormalisableRange<float> (2.0f, 8.0f, 1.0f),
        3.0f,
        Attributes {}
            .withStringFromValueFunction ([] (float value, int) { return juce::String (static_cast<int> (value)); })));

    parameters.push_back (std::make_unique<Parameter> (
        juce::ParameterID { param::profileNumBands, 1 },
        "EQ Bands",
        juce::NormalisableRange<float> (1.0f, 8.0f, 1.0f),
        3.0f,
        Attributes {}
            .withStringFromValueFunction ([] (float value, int) { return juce::String (static_cast<int> (value)); })));

    return { parameters.begin(), parameters.end() };
}

RuntimeSettings loadRuntimeSettings (const juce::AudioProcessorValueTreeState& valueTreeState)
{
    const auto valueOf = [&valueTreeState] (const auto parameterId)
    {
        return valueTreeState.getRawParameterValue (parameterId)->load();
    };

    return {
        .balanceDb = valueOf (param::balance),
        .dynamicsPercent = valueOf (param::dynamics),
        .expansionMode = valueTreeState.getRawParameterValue (param::expansionMode)->load() > 0.5f,
        .speedMs = valueOf (param::speed),
        .rangeDb = valueOf (param::range),
        .anchorAutoMode = valueOf (param::anchorAuto) > 0.5f,
        .anchorManualDb = valueOf (param::anchorManual),
        .boostCeilingDb = valueOf (param::boostCeiling),
        .duckFloorDb = valueOf (param::duckFloor),
        .noiseGateDb = valueOf (param::noiseGate),
        .lookAheadMs = valueOf (param::lookAhead),
        .presenceReleaseMs = valueOf (param::presenceRelease),
        .profileActive = valueTreeState.getRawParameterValue (param::profileActive)->load() > 0.5f,
        .profileMaxMinutes = static_cast<int> (valueOf (param::profileMaxMinutes)),
        .profileNumZones = static_cast<int> (valueOf (param::profileNumZones)),
        .profileNumBands = static_cast<int> (valueOf (param::profileNumBands))
    };
}
} // namespace liveflow
