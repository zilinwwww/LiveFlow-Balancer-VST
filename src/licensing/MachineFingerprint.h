#pragma once

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>

namespace liveflow
{
class MachineFingerprint
{
public:
    static juce::String generate()
    {
        juce::StringArray components;
        auto deviceIds = juce::SystemStats::getDeviceIdentifiers();
        components.addArray (deviceIds);
        components.add (juce::SystemStats::getComputerName());
        auto combined = components.joinIntoString ("|");
        auto hash = juce::SHA256 (combined.toUTF8());
        return hash.toHexString();
    }
};
} // namespace liveflow
