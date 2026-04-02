#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace liveflow
{
namespace param
{
// ── Core Parameters (Publicly automatable via DAW Host) ──
inline constexpr auto balance = "balance";             // Target Volume LUFS separation
inline constexpr auto dynamics = "dynamics";           // Positive ducks hard, Negative expands
inline constexpr auto expansionMode = "expansionMode"; // Flips duck to upward boost behavior
inline constexpr auto speed = "speed";                 // Reaction transition decay time (ms)
inline constexpr auto range = "range";                 // Max dB ceiling allowed
inline constexpr auto anchorAuto = "anchorAuto";       // Binds vocal tracking EMA
inline constexpr auto anchorManual = "anchorManual";   // Manual center target reference

// Expert parameters
inline constexpr auto boostCeiling = "boostCeiling";
inline constexpr auto duckFloor = "duckFloor";
inline constexpr auto noiseGate = "noiseGate";
inline constexpr auto lookAhead = "lookAhead";
inline constexpr auto presenceRelease = "presenceRelease";

// Smart Track Profiler parameters
inline constexpr auto profileActive = "profileActive";
inline constexpr auto profileMaxMinutes = "profileMaxMinutes";
inline constexpr auto profileNumZones = "profileNumZones";
inline constexpr auto profileNumBands = "profileNumBands";
} // namespace param

struct RuntimeSettings
{
    // Core
    float balanceDb = 3.0f;
    float dynamicsPercent = 0.0f;       // -100..100% bipolar
    bool expansionMode = false;         // true = Expander mode (swap Duck/Boost areas)
    float speedMs = 120.0f;
    float rangeDb = 8.0f;
    bool anchorAutoMode = true;
    float anchorManualDb = -24.0f;

    // Expert
    float boostCeilingDb = 6.0f;
    float duckFloorDb = 10.0f;
    float noiseGateDb = -45.0f;
    float lookAheadMs = 0.0f;
    float presenceReleaseMs = 80.0f;

    // Smart Track Profiler
    bool profileActive = false;
    int profileMaxMinutes = 5;
    int profileNumZones = 3;
    int profileNumBands = 3;
};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
RuntimeSettings loadRuntimeSettings (const juce::AudioProcessorValueTreeState& valueTreeState);
} // namespace liveflow
