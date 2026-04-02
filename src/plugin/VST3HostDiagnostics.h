#pragma once

#include <array>
#include <atomic>

namespace liveflow::vst3debug
{
struct Snapshot
{
    int numInputs = 0;
    int numOutputs = 0;
    int numSamples = 0;
    std::array<int, 2> inputBusChannels {};
    std::array<float, 2> inputBusDb { -100.0f, -100.0f };
    std::array<bool, 2> inputBusHasBuffers { false, false };
    std::array<bool, 2> inputBusSilentAll { false, false };
};

struct SharedState
{
    std::atomic<int> numInputs { 0 };
    std::atomic<int> numOutputs { 0 };
    std::atomic<int> numSamples { 0 };
    std::array<std::atomic<int>, 2> inputBusChannels { 0, 0 };
    std::array<std::atomic<float>, 2> inputBusDb { -100.0f, -100.0f };
    std::array<std::atomic<bool>, 2> inputBusHasBuffers { false, false };
    std::array<std::atomic<bool>, 2> inputBusSilentAll { false, false };
};

inline SharedState& state() noexcept
{
    static SharedState diagnostics;
    return diagnostics;
}

inline Snapshot capture() noexcept
{
    Snapshot snapshot;
    auto& diagnostics = state();
    snapshot.numInputs = diagnostics.numInputs.load (std::memory_order_relaxed);
    snapshot.numOutputs = diagnostics.numOutputs.load (std::memory_order_relaxed);
    snapshot.numSamples = diagnostics.numSamples.load (std::memory_order_relaxed);

    for (size_t i = 0; i < snapshot.inputBusChannels.size(); ++i)
        snapshot.inputBusChannels[i] = diagnostics.inputBusChannels[i].load (std::memory_order_relaxed);

    for (size_t i = 0; i < snapshot.inputBusDb.size(); ++i)
        snapshot.inputBusDb[i] = diagnostics.inputBusDb[i].load (std::memory_order_relaxed);

    for (size_t i = 0; i < snapshot.inputBusHasBuffers.size(); ++i)
        snapshot.inputBusHasBuffers[i] = diagnostics.inputBusHasBuffers[i].load (std::memory_order_relaxed);

    for (size_t i = 0; i < snapshot.inputBusSilentAll.size(); ++i)
        snapshot.inputBusSilentAll[i] = diagnostics.inputBusSilentAll[i].load (std::memory_order_relaxed);

    return snapshot;
}
} // namespace liveflow::vst3debug
