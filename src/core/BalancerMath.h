#pragma once

#include <cmath>

#include <juce_core/juce_core.h>

namespace liveflow::math
{
// ── Display constants ──
inline constexpr float kMinMeterDb = -60.0f;
inline constexpr float kMaxMeterDb = 6.0f;
inline constexpr float kSigmoidSoftWidth = 8.0f;

// ── Normalisation ──

inline float dbToNormalised (const float dbValue,
                             const float minDb = kMinMeterDb,
                             const float maxDb = kMaxMeterDb) noexcept
{
    return juce::jlimit (0.0f, 1.0f, juce::jmap (dbValue, minDb, maxDb, 0.0f, 1.0f));
}

// ── Smoothing ──

inline float smoothTowards (const float currentValue,
                            const float targetValue,
                            const float coefficient) noexcept
{
    return coefficient * currentValue + (1.0f - coefficient) * targetValue;
}

inline float timeMsToCoefficient (const double sampleRate, const float timeMs) noexcept
{
    const auto clampedMs = juce::jmax (1.0e-3f, timeMs);
    return std::exp (-1.0f / (0.001f * clampedMs * static_cast<float> (sampleRate)));
}

// ── Sigmoid soft presence detection ──
// Returns 0.0 (no voice) to 1.0 (full voice), continuous transition
// replaces the old hard gate threshold

inline float sigmoid (const float x) noexcept
{
    return 1.0f / (1.0f + std::exp (-x));
}

inline float computeVoicePresence (const float envelopeDb,
                                   const float noiseFloorDb,
                                   const float softWidth = kSigmoidSoftWidth) noexcept
{
    return sigmoid ((envelopeDb - noiseFloorDb) / juce::jmax (1.0f, softWidth));
}

/**
 * @brief Computes the necessary offset dB required to achieve the user's targeted balance.
 * 
 * This is the central brain of the volume ducking algorithm.
 * 
 * @param heldVocalLufs The current smoothed loudness of the vocalist.
 * @param backingLufs The current loudness of the backing track to be ducked.
 * @param balanceDb The requested distance between the vocal and backing average.
 * @param dynamicsPercent How aggressively the backing should be suppressed when the vocal gets unusually loud.
 * @param anchorPointDb The resting "Auto-Anchor" anchor baseline reference level.
 * @return The target offset in Decibels (raw math target, will be smoothed by the Engine later).
 */
inline float computeTargetCurveGainDb (const float heldVocalLufs,
                                       const float backingLufs,
                                       const float balanceDb,
                                       const float dynamicsPercent,
                                       const float anchorPointDb) noexcept
{
    const auto slope = dynamicsPercent * 0.030f;
    const auto dynamicOffset = balanceDb + slope * (heldVocalLufs - anchorPointDb);
    const auto targetBackingLufs = heldVocalLufs - dynamicOffset;
    return targetBackingLufs - backingLufs;
}

// ── Clamp gain to the user's allowed range ──

inline float clampGainDb (const float gainDb,
                          const float duckFloorDb,
                          const float boostCeilingDb) noexcept
{
    return juce::jlimit (-duckFloorDb, boostCeilingDb, gainDb);
}

} // namespace liveflow::math
