#include "VisualizationState.h"

namespace liveflow
{
void VisualizationState::reset() noexcept
{
    currentVocalLufs.store (-70.0f, std::memory_order_relaxed);
    currentBackingLufs.store (-70.0f, std::memory_order_relaxed);
    currentGainDb.store (0.0f, std::memory_order_relaxed);
    currentVoicePresence.store (0.0f, std::memory_order_relaxed);
    currentHeldVocalLevel.store (-60.0f, std::memory_order_relaxed);
    currentAnchorPoint.store (-24.0f, std::memory_order_relaxed);
    currentSidechainConnected.store (false, std::memory_order_relaxed);

    for (auto& slot : gainHistory)
        slot.store (0.0f, std::memory_order_relaxed);
    gainHeadIndex.store (0, std::memory_order_relaxed);

    for (auto& slot : trailVocalLufs)
        slot.store (-70.0f, std::memory_order_relaxed);
    for (auto& slot : trailBackingLufs)
        slot.store (-70.0f, std::memory_order_relaxed);
    trailHeadIndex.store (0, std::memory_order_relaxed);

    for (auto& value : rawChannelDb)
        value.store (-100.0f, std::memory_order_relaxed);

    for (auto& value : mappedSidechainDb)
        value.store (-100.0f, std::memory_order_relaxed);

    for (auto& value : wrapperInputBusDb)
        value.store (-100.0f, std::memory_order_relaxed);

    for (auto& value : wrapperInputBusChannels)
        value.store (0, std::memory_order_relaxed);

    for (auto& value : wrapperInputBusHasBuffers)
        value.store (false, std::memory_order_relaxed);

    for (auto& value : wrapperInputBusSilentAll)
        value.store (false, std::memory_order_relaxed);

    processBufferChannels.store (0, std::memory_order_relaxed);
    processBlockSamples.store (0, std::memory_order_relaxed);
    totalInputChannels.store (0, std::memory_order_relaxed);
    totalOutputChannels.store (0, std::memory_order_relaxed);
    inputBusCount.store (0, std::memory_order_relaxed);
    outputBusCount.store (0, std::memory_order_relaxed);
    wrapperNumInputs.store (0, std::memory_order_relaxed);
    wrapperNumOutputs.store (0, std::memory_order_relaxed);
    wrapperNumSamples.store (0, std::memory_order_relaxed);
    mainInputChannels.store (0, std::memory_order_relaxed);
    mainOutputChannels.store (0, std::memory_order_relaxed);
    sidechainChannels.store (0, std::memory_order_relaxed);
    sidechainMap0.store (-1, std::memory_order_relaxed);
    sidechainMap1.store (-1, std::memory_order_relaxed);
    sidechainBusPresent.store (false, std::memory_order_relaxed);
    sidechainBusEnabled.store (false, std::memory_order_relaxed);
}

void VisualizationState::pushFrame (const float vocalLufs,
                                    const float backingLufs,
                                    const float gainDb,
                                    const float voicePresence,
                                    const float heldVocalLevel,
                                    const float anchorPoint,
                                    const bool sidechainConnected) noexcept
{
    currentVocalLufs.store (vocalLufs, std::memory_order_relaxed);
    currentBackingLufs.store (backingLufs, std::memory_order_relaxed);
    currentGainDb.store (gainDb, std::memory_order_relaxed);
    currentVoicePresence.store (voicePresence, std::memory_order_relaxed);
    currentHeldVocalLevel.store (heldVocalLevel, std::memory_order_relaxed);
    currentAnchorPoint.store (anchorPoint, std::memory_order_relaxed);
    currentSidechainConnected.store (sidechainConnected, std::memory_order_relaxed);

    // Push to gain timeline ring buffer
    const auto gIndex = gainHeadIndex.load (std::memory_order_relaxed);
    gainHistory[static_cast<size_t> (gIndex)].store (gainDb, std::memory_order_relaxed);
    gainHeadIndex.store ((gIndex + 1) % gainHistorySize, std::memory_order_release);

    // Push to state trail ring buffer
    const auto tIndex = trailHeadIndex.load (std::memory_order_relaxed);
    trailVocalLufs[static_cast<size_t> (tIndex)].store (vocalLufs, std::memory_order_relaxed);
    trailBackingLufs[static_cast<size_t> (tIndex)].store (backingLufs, std::memory_order_relaxed);
    trailHeadIndex.store ((tIndex + 1) % trailSize, std::memory_order_release);
}

void VisualizationState::updateDiagnostics (const Snapshot::Diagnostics& diagnostics) noexcept
{
    for (size_t index = 0; index < rawChannelDb.size(); ++index)
        rawChannelDb[index].store (diagnostics.rawChannelDb[index], std::memory_order_relaxed);

    for (size_t index = 0; index < mappedSidechainDb.size(); ++index)
        mappedSidechainDb[index].store (diagnostics.mappedSidechainDb[index], std::memory_order_relaxed);

    for (size_t index = 0; index < wrapperInputBusDb.size(); ++index)
        wrapperInputBusDb[index].store (diagnostics.wrapperInputBusDb[index], std::memory_order_relaxed);

    for (size_t index = 0; index < wrapperInputBusChannels.size(); ++index)
        wrapperInputBusChannels[index].store (diagnostics.wrapperInputBusChannels[index], std::memory_order_relaxed);

    for (size_t index = 0; index < wrapperInputBusHasBuffers.size(); ++index)
        wrapperInputBusHasBuffers[index].store (diagnostics.wrapperInputBusHasBuffers[index], std::memory_order_relaxed);

    for (size_t index = 0; index < wrapperInputBusSilentAll.size(); ++index)
        wrapperInputBusSilentAll[index].store (diagnostics.wrapperInputBusSilentAll[index], std::memory_order_relaxed);

    processBufferChannels.store (diagnostics.processBufferChannels, std::memory_order_relaxed);
    processBlockSamples.store (diagnostics.processBlockSamples, std::memory_order_relaxed);
    totalInputChannels.store (diagnostics.totalInputChannels, std::memory_order_relaxed);
    totalOutputChannels.store (diagnostics.totalOutputChannels, std::memory_order_relaxed);
    inputBusCount.store (diagnostics.inputBusCount, std::memory_order_relaxed);
    outputBusCount.store (diagnostics.outputBusCount, std::memory_order_relaxed);
    wrapperNumInputs.store (diagnostics.wrapperNumInputs, std::memory_order_relaxed);
    wrapperNumOutputs.store (diagnostics.wrapperNumOutputs, std::memory_order_relaxed);
    wrapperNumSamples.store (diagnostics.wrapperNumSamples, std::memory_order_relaxed);
    mainInputChannels.store (diagnostics.mainInputChannels, std::memory_order_relaxed);
    mainOutputChannels.store (diagnostics.mainOutputChannels, std::memory_order_relaxed);
    sidechainChannels.store (diagnostics.sidechainChannels, std::memory_order_relaxed);
    sidechainMap0.store (diagnostics.sidechainMap0, std::memory_order_relaxed);
    sidechainMap1.store (diagnostics.sidechainMap1, std::memory_order_relaxed);
    sidechainBusPresent.store (diagnostics.sidechainBusPresent, std::memory_order_relaxed);
    sidechainBusEnabled.store (diagnostics.sidechainBusEnabled, std::memory_order_relaxed);
}

VisualizationState::Snapshot VisualizationState::capture() const noexcept
{
    Snapshot snapshot;

    // Current state
    snapshot.vocalLufs = currentVocalLufs.load (std::memory_order_relaxed);
    snapshot.backingLufs = currentBackingLufs.load (std::memory_order_relaxed);
    snapshot.gainDb = currentGainDb.load (std::memory_order_relaxed);
    snapshot.voicePresence = currentVoicePresence.load (std::memory_order_relaxed);
    snapshot.heldVocalLevel = currentHeldVocalLevel.load (std::memory_order_relaxed);
    snapshot.anchorPoint = currentAnchorPoint.load (std::memory_order_relaxed);
    snapshot.sidechainConnected = currentSidechainConnected.load (std::memory_order_relaxed);

    // Gain timeline
    snapshot.gainHeadIndex = gainHeadIndex.load (std::memory_order_acquire);
    for (size_t index = 0; index < snapshot.gainHistory.size(); ++index)
        snapshot.gainHistory[index] = gainHistory[index].load (std::memory_order_relaxed);

    // State trail
    snapshot.trailHeadIndex = trailHeadIndex.load (std::memory_order_acquire);
    for (size_t index = 0; index < snapshot.trail.size(); ++index)
    {
        snapshot.trail[index].vocalLufs = trailVocalLufs[index].load (std::memory_order_relaxed);
        snapshot.trail[index].backingLufs = trailBackingLufs[index].load (std::memory_order_relaxed);
    }

    // Diagnostics
    snapshot.diagnostics.processBufferChannels = processBufferChannels.load (std::memory_order_relaxed);
    snapshot.diagnostics.processBlockSamples = processBlockSamples.load (std::memory_order_relaxed);
    snapshot.diagnostics.totalInputChannels = totalInputChannels.load (std::memory_order_relaxed);
    snapshot.diagnostics.totalOutputChannels = totalOutputChannels.load (std::memory_order_relaxed);
    snapshot.diagnostics.inputBusCount = inputBusCount.load (std::memory_order_relaxed);
    snapshot.diagnostics.outputBusCount = outputBusCount.load (std::memory_order_relaxed);
    snapshot.diagnostics.wrapperNumInputs = wrapperNumInputs.load (std::memory_order_relaxed);
    snapshot.diagnostics.wrapperNumOutputs = wrapperNumOutputs.load (std::memory_order_relaxed);
    snapshot.diagnostics.wrapperNumSamples = wrapperNumSamples.load (std::memory_order_relaxed);
    snapshot.diagnostics.mainInputChannels = mainInputChannels.load (std::memory_order_relaxed);
    snapshot.diagnostics.mainOutputChannels = mainOutputChannels.load (std::memory_order_relaxed);
    snapshot.diagnostics.sidechainChannels = sidechainChannels.load (std::memory_order_relaxed);
    snapshot.diagnostics.sidechainMap0 = sidechainMap0.load (std::memory_order_relaxed);
    snapshot.diagnostics.sidechainMap1 = sidechainMap1.load (std::memory_order_relaxed);
    snapshot.diagnostics.sidechainBusPresent = sidechainBusPresent.load (std::memory_order_relaxed);
    snapshot.diagnostics.sidechainBusEnabled = sidechainBusEnabled.load (std::memory_order_relaxed);

    for (size_t index = 0; index < snapshot.diagnostics.rawChannelDb.size(); ++index)
        snapshot.diagnostics.rawChannelDb[index] = rawChannelDb[index].load (std::memory_order_relaxed);

    for (size_t index = 0; index < snapshot.diagnostics.mappedSidechainDb.size(); ++index)
        snapshot.diagnostics.mappedSidechainDb[index] = mappedSidechainDb[index].load (std::memory_order_relaxed);

    for (size_t index = 0; index < snapshot.diagnostics.wrapperInputBusDb.size(); ++index)
        snapshot.diagnostics.wrapperInputBusDb[index] = wrapperInputBusDb[index].load (std::memory_order_relaxed);

    for (size_t index = 0; index < snapshot.diagnostics.wrapperInputBusChannels.size(); ++index)
        snapshot.diagnostics.wrapperInputBusChannels[index] = wrapperInputBusChannels[index].load (std::memory_order_relaxed);

    for (size_t index = 0; index < snapshot.diagnostics.wrapperInputBusHasBuffers.size(); ++index)
        snapshot.diagnostics.wrapperInputBusHasBuffers[index] = wrapperInputBusHasBuffers[index].load (std::memory_order_relaxed);

    for (size_t index = 0; index < snapshot.diagnostics.wrapperInputBusSilentAll.size(); ++index)
        snapshot.diagnostics.wrapperInputBusSilentAll[index] = wrapperInputBusSilentAll[index].load (std::memory_order_relaxed);

    return snapshot;
}
} // namespace liveflow
