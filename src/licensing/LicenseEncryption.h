#pragma once

#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>

namespace liveflow
{
class LicenseEncryption
{
public:
    static juce::MemoryBlock encrypt (const juce::String& plaintext, const juce::String& machineId)
    {
        auto keyBlock = deriveKey (machineId);
        auto data = plaintext.toUTF8();
        auto dataLen = static_cast<size_t> (strlen (data));

        juce::MemoryBlock result (dataLen);
        auto* out = static_cast<uint8_t*> (result.getData());
        auto* keyBytes = static_cast<const uint8_t*> (keyBlock.getData());
        auto keyLen = keyBlock.getSize();

        for (size_t i = 0; i < dataLen; ++i)
            out[i] = static_cast<uint8_t> (data[i]) ^ keyBytes[i % keyLen];

        return result;
    }

    static juce::String decrypt (const juce::MemoryBlock& ciphertext, const juce::String& machineId)
    {
        auto keyBlock = deriveKey (machineId);
        auto len = ciphertext.getSize();

        juce::MemoryBlock result (len + 1);
        auto* out = static_cast<uint8_t*> (result.getData());
        auto* in = static_cast<const uint8_t*> (ciphertext.getData());
        auto* keyBytes = static_cast<const uint8_t*> (keyBlock.getData());
        auto keyLen = keyBlock.getSize();

        for (size_t i = 0; i < len; ++i)
            out[i] = in[i] ^ keyBytes[i % keyLen];
        out[len] = 0;

        return juce::String::fromUTF8 (reinterpret_cast<const char*> (out), static_cast<int> (len));
    }

private:
    static juce::MemoryBlock deriveKey (const juce::String& machineId)
    {
        auto saltedInput = machineId + "liveflow-salt-v1";
        auto hash = juce::SHA256 (saltedInput.toUTF8());
        return hash.getRawData(); // 32 bytes
    }
};
} // namespace liveflow
