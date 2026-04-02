#pragma once

#include <array>
#include <atomic>

namespace liveflow
{
/**
 * @class VisualizationState
 * @brief Lock-Free Memory Bridge connecting the critical Audio Thread and the slow GUI Message Thread.
 *
 * It uses pure C++11 std::atomics to transport real-time volume states, LUFS timelines, 
 * and diagnostic arrays across threads without EVER locking a mutex or allocating memory.
 * 
 * Crucially, pushing states relies on `std::memory_order_relaxed` for single values because 
 * exact frame-sync across isolated metrics is unnecessary for visual drawing at 60 FPS.
 * On structural arrays (like the Ring Buffer History lines), it relies on `memory_order_release` 
 * and `memory_order_acquire` for the Head Indices, enforcing proper memory fencing and preventing
 * the UI thread from observing half-written structures.
 */
class VisualizationState
{
public:
    static constexpr int gainHistorySize = 360;
    static constexpr int trailSize = 90;
    static constexpr int diagnosticChannels = 8;

    struct TrailPoint
    {
        float vocalLufs = -70.0f;
        float backingLufs = -70.0f;
    };

    /**
     * @brief A read-only snapshot constructed by `capture()` for the GUI paint method.
     */
    struct Snapshot
    {
        struct Diagnostics
        {
            std::array<float, diagnosticChannels> rawChannelDb {};
            std::array<float, 2> mappedSidechainDb {};
            std::array<float, 2> wrapperInputBusDb {};
            std::array<int, 2> wrapperInputBusChannels {};
            std::array<bool, 2> wrapperInputBusHasBuffers { false, false };
            std::array<bool, 2> wrapperInputBusSilentAll { false, false };
            int processBufferChannels = 0;
            int processBlockSamples = 0;
            int totalInputChannels = 0;
            int totalOutputChannels = 0;
            int inputBusCount = 0;
            int outputBusCount = 0;
            int wrapperNumInputs = 0;
            int wrapperNumOutputs = 0;
            int wrapperNumSamples = 0;
            int mainInputChannels = 0;
            int mainOutputChannels = 0;
            int sidechainChannels = 0;
            int sidechainMap0 = -1;
            int sidechainMap1 = -1;
            bool sidechainBusPresent = false;
            bool sidechainBusEnabled = false;
        };

        // Current real-time scalar states
        float vocalLufs = -70.0f;
        float backingLufs = -70.0f;
        float gainDb = 0.0f;
        float voicePresence = 0.0f;
        float heldVocalLevel = -60.0f;
        float anchorPoint = -24.0f;
        bool sidechainConnected = false;

        // Smooth continuous 360-degree timeline history 
        std::array<float, gainHistorySize> gainHistory {};
        int gainHeadIndex = 0;

        // Target scatterplot history 
        std::array<TrailPoint, trailSize> trail {};
        int trailHeadIndex = 0;

        Diagnostics diagnostics;
    };

    /**
     * @brief Blanks out all internal atomic values to their offline rest states.
     */
    void reset() noexcept;

    /**
     * @brief Called EXCLUSIVELY by the Audio Thread to push fresh DSP metrics. Lock-Free.
     */
    void pushFrame (float vocalLufs,
                    float backingLufs,
                    float gainDb,
                    float voicePresence,
                    float heldVocalLevel,
                    float anchorPoint,
                    bool sidechainConnected) noexcept;

    /**
     * @brief Transmits raw pin-level structural diagnostics info.
     */
    void updateDiagnostics (const Snapshot::Diagnostics&) noexcept;

    /**
     * @brief Called EXCLUSIVELY by the generic Message/GUI Thread to copy off a thread-safe snapshot for rendering.
     */
    [[nodiscard]] Snapshot capture() const noexcept;

private:
    // Current state atomics (Relaxed Ordering is sufficient)
    std::atomic<float> currentVocalLufs { -70.0f };
    std::atomic<float> currentBackingLufs { -70.0f };
    std::atomic<float> currentGainDb { 0.0f };
    std::atomic<float> currentVoicePresence { 0.0f };
    std::atomic<float> currentHeldVocalLevel { -60.0f };
    std::atomic<float> currentAnchorPoint { -24.0f };
    std::atomic<bool> currentSidechainConnected { false };

    // Gain timeline ring buffer (Synchronized via gainHeadIndex acquire/release fencing)
    std::array<std::atomic<float>, gainHistorySize> gainHistory {};
    std::atomic<int> gainHeadIndex { 0 };

    // State point trail ring buffer (Synchronized via trailHeadIndex acquire/release fencing)
    std::array<std::atomic<float>, trailSize> trailVocalLufs {};
    std::array<std::atomic<float>, trailSize> trailBackingLufs {};
    std::atomic<int> trailHeadIndex { 0 };

    // Miscellaneous Host Diagnostics (Left relaxed to reduce CPU stall)
    std::array<std::atomic<float>, diagnosticChannels> rawChannelDb {};
    std::array<std::atomic<float>, 2> mappedSidechainDb {};
    std::array<std::atomic<float>, 2> wrapperInputBusDb {};
    std::array<std::atomic<int>, 2> wrapperInputBusChannels {};
    std::array<std::atomic<bool>, 2> wrapperInputBusHasBuffers {};
    std::array<std::atomic<bool>, 2> wrapperInputBusSilentAll {};
    std::atomic<int> processBufferChannels { 0 };
    std::atomic<int> processBlockSamples { 0 };
    std::atomic<int> totalInputChannels { 0 };
    std::atomic<int> totalOutputChannels { 0 };
    std::atomic<int> inputBusCount { 0 };
    std::atomic<int> outputBusCount { 0 };
    std::atomic<int> wrapperNumInputs { 0 };
    std::atomic<int> wrapperNumOutputs { 0 };
    std::atomic<int> wrapperNumSamples { 0 };
    std::atomic<int> mainInputChannels { 0 };
    std::atomic<int> mainOutputChannels { 0 };
    std::atomic<int> sidechainChannels { 0 };
    std::atomic<int> sidechainMap0 { -1 };
    std::atomic<int> sidechainMap1 { -1 };
    std::atomic<bool> sidechainBusPresent { false };
    std::atomic<bool> sidechainBusEnabled { false };
};
} // namespace liveflow
