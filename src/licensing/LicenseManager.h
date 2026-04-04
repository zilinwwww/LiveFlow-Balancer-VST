#pragma once

#include <juce_core/juce_core.h>
#include "MachineFingerprint.h"
#include "LicenseEncryption.h"

namespace liveflow
{
/**
 * LicenseManager — handles the full activation lifecycle.
 *
 * States:
 *   [Unlicensed] → activate(key) → [Licensed]
 *   [Licensed]   → validate() fails → [Unlicensed]
 *   [Licensed]   → offline > 3 days → [Unlicensed]
 */
class LicenseManager
{
public:
    static constexpr const char* kActivationUrl = "https://liveflow.micro-grav.com/api/plugin/activate";
    static constexpr const char* kValidateUrl   = "https://liveflow.micro-grav.com/api/plugin/validate";

    // Fallback URLs for domestic users (e.g. AliCloud, Tencent Cloud, or Vercel CNAME)
    static constexpr const char* kActivationUrlFallback = "https://liveflow-fallback.micro-grav.com/api/plugin/activate";
    static constexpr const char* kValidateUrlFallback   = "https://liveflow-fallback.micro-grav.com/api/plugin/validate";
    static constexpr int kOfflineGraceDays = 3;

    LicenseManager()
        : machineId (MachineFingerprint::generate()),
          licenseFile (getLicenseFilePath())
    {
        loadLicense();
    }

    bool isLicensed() const noexcept { return licensed; }
    juce::String getLicenseKey() const noexcept { return licenseKey; }
    juce::String getMachineId() const noexcept { return machineId; }
    juce::String getStatusMessage() const noexcept { return statusMessage; }

    static juce::String getHardwareViaPowershell()
    {
        juce::ChildProcess process;
        juce::StringArray args;
        args.add("powershell");
        args.add("-NoProfile");
        args.add("-Command");
        args.add("$cpu=(Get-CimInstance Win32_Processor | Select-Object -ExpandProperty Name) -join ', ';"
                 "$gpu=(Get-CimInstance Win32_VideoController | Select-Object -ExpandProperty Name) -join ', ';"
                 "$snd=(Get-CimInstance Win32_SoundDevice | Select-Object -ExpandProperty Name) -join ', ';"
                 "$kbd=(Get-CimInstance Win32_Keyboard | Select-Object -ExpandProperty Description) -join ', ';"
                 "$mse=(Get-CimInstance Win32_PointingDevice | Select-Object -ExpandProperty Description) -join ', ';"
                 "$mobo=(Get-CimInstance Win32_BaseBoard | Select-Object -ExpandProperty Product) -join ', ';"
                 "$mon=(Get-CimInstance Win32_DesktopMonitor | Select-Object -ExpandProperty Name) -join ', ';"
                 "@{CPU=$cpu;GPU=$gpu;Sound=$snd;Keyboard=$kbd;Mouse=$mse;Motherboard=$mobo;Monitor=$mon} | ConvertTo-Json -Compress");
        
        if (process.start(args, juce::ChildProcess::wantStdOut | juce::ChildProcess::wantStdErr))
        {
            juce::String output = process.readAllProcessOutput();
            return output.trim();
        }
        return "{}";
    }

    /**
     * Attempt to activate with a license key.
     * Makes a synchronous HTTP call to the server.
     * Returns true if activation succeeded.
     */
    bool activate (const juce::String& key)
    {
        juce::DynamicObject::Ptr info = new juce::DynamicObject();
        info->setProperty("os", juce::SystemStats::getOperatingSystemName());
        // Will rely on WMI CPU instead for more detailed model name
        info->setProperty("ram", juce::String(juce::SystemStats::getMemorySizeInMegabytes()) + "MB");
        info->setProperty("hostname", juce::SystemStats::getComputerName());
        
        juce::var hwVar = juce::JSON::parse(getHardwareViaPowershell());
        if (auto* hwObj = hwVar.getDynamicObject()) {
            // Append CPU Core info alongside WMI name
            juce::String cpuName = hwObj->getProperty("CPU").toString();
            info->setProperty("cpu", cpuName.isNotEmpty() ? cpuName + " (" + juce::String(juce::SystemStats::getNumCpus()) + " cores)" : "Unknown CPU");
            info->setProperty("gpu", hwObj->getProperty("GPU"));
            info->setProperty("sound", hwObj->getProperty("Sound"));
            info->setProperty("keyboard", hwObj->getProperty("Keyboard"));
            info->setProperty("mouse", hwObj->getProperty("Mouse"));
            info->setProperty("motherboard", hwObj->getProperty("Motherboard"));
            info->setProperty("monitor", hwObj->getProperty("Monitor"));
        }

        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        root->setProperty("key", key);
        root->setProperty("machineId", machineId);
        root->setProperty("machineName", juce::SystemStats::getComputerName());
        root->setProperty("machineInfo", juce::JSON::toString(juce::var(info.get())));

        juce::String jsonBody = juce::JSON::toString(juce::var(root.get()));

        juce::URL url (kActivationUrl);
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withExtraHeaders ("Content-Type: application/json\r\nUser-Agent: LiveFlow-Plugin/1.0")
                           .withConnectionTimeoutMs (15000)
                           .withNumRedirectsToFollow (3);

        auto stream = url.withPOSTData (jsonBody).createInputStream (options);

        if (stream == nullptr)
        {
            // Try fallback URL if primary fails
            juce::URL fallbackUrl (kActivationUrlFallback);
            stream = fallbackUrl.withPOSTData (jsonBody).createInputStream (options);
        }

        if (stream == nullptr)
        {
            statusMessage = "Network error: could not reach server.";
            return false;
        }

        auto response = stream->readEntireStreamAsString();
        auto parsed = juce::JSON::parse (response);

        if (parsed.getProperty ("status", "") == juce::var ("ok"))
        {
            licenseKey = key;
            licensed = true;
            offlineSince = juce::Time();
            statusMessage = "Activated successfully!";
            saveLicense();
            return true;
        }

        auto code = parsed.getProperty ("code", "").toString();
        if (code == "INVALID_KEY") statusMessage = "Invalid license key.";
        else if (code == "ALREADY_BOUND") statusMessage = "Key is already bound to another machine.";
        else if (code == "REVOKED") statusMessage = "License has been revoked.";
        else statusMessage = "Activation failed: " + code;
        return false;
    }

    /**
     * Background validation — called after plugin starts with existing license.
     * Should be called from a background thread.
     */
    void validateInBackground()
    {
        if (! licensed || licenseKey.isEmpty())
            return;

        auto jsonBody = juce::String ("{\"key\":\"") + licenseKey + "\",\"machineId\":\"" + machineId + "\"}";

        juce::URL url (kValidateUrl);
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                           .withExtraHeaders ("Content-Type: application/json\r\nUser-Agent: LiveFlow-Plugin/1.0")
                           .withConnectionTimeoutMs (15000)
                           .withNumRedirectsToFollow (3);

        auto stream = url.withPOSTData (jsonBody).createInputStream (options);

        if (stream == nullptr)
        {
            // Try fallback URL if primary fails
            juce::URL fallbackUrl (kValidateUrlFallback);
            stream = fallbackUrl.withPOSTData (jsonBody).createInputStream (options);
        }

        if (stream == nullptr)
        {
            // Network failure — apply offline grace period
            handleOffline();
            return;
        }

        auto response = stream->readEntireStreamAsString();
        auto parsed = juce::JSON::parse (response);

        if (parsed.getProperty ("status", "") != juce::var ("ok"))
        {
            handleOffline();
            return;
        }

        bool valid = parsed.getProperty ("valid", false);

        if (valid)
        {
            // All good — clear any offline timestamp
            if (offlineSince != juce::Time())
            {
                offlineSince = juce::Time();
                saveLicense();
            }
        }
        else
        {
            // License invalidated remotely (unbound, rebound, or revoked)
            invalidate();
        }
    }

    /** Force deactivation — deletes local license. */
    void invalidate()
    {
        licensed = false;
        licenseKey = {};
        offlineSince = juce::Time();
        statusMessage = "License is no longer valid. Please re-activate.";

        if (licenseFile.existsAsFile())
            licenseFile.deleteFile();
    }

private:
    juce::String machineId;
    juce::File licenseFile;
    bool licensed = false;
    juce::String licenseKey;
    juce::Time offlineSince;
    juce::String statusMessage;

    static juce::File getLicenseFilePath()
    {
        auto appDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                          .getChildFile (".liveflow");
        appDir.createDirectory();
        return appDir.getChildFile ("license.dat");
    }

    void loadLicense()
    {
        if (! licenseFile.existsAsFile())
        {
            licensed = false;
            return;
        }

        juce::MemoryBlock data;
        if (! licenseFile.loadFileAsData (data) || data.getSize() == 0)
        {
            licensed = false;
            return;
        }

        auto json = LicenseEncryption::decrypt (data, machineId);
        auto parsed = juce::JSON::parse (json);

        if (parsed.isVoid())
        {
            licenseFile.deleteFile();
            licensed = false;
            return;
        }

        auto storedMachineId = parsed.getProperty ("machineId", "").toString();
        if (storedMachineId != machineId)
        {
            // License file from a different machine — invalid
            licenseFile.deleteFile();
            licensed = false;
            return;
        }

        licenseKey = parsed.getProperty ("key", "").toString();
        auto offlineStr = parsed.getProperty ("offlineSince", "").toString();
        if (offlineStr.isNotEmpty())
            offlineSince = juce::Time::fromISO8601 (offlineStr);
        else
            offlineSince = juce::Time();

        licensed = licenseKey.isNotEmpty();
    }

    void saveLicense()
    {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty ("key", licenseKey);
        obj->setProperty ("machineId", machineId);
        obj->setProperty ("activatedAt", juce::Time::getCurrentTime().toISO8601 (true));
        if (offlineSince != juce::Time())
            obj->setProperty ("offlineSince", offlineSince.toISO8601 (true));
        else
            obj->setProperty ("offlineSince", "");

        auto json = juce::JSON::toString (juce::var (obj.get()));
        auto encrypted = LicenseEncryption::encrypt (json, machineId);
        licenseFile.replaceWithData (encrypted.getData(), encrypted.getSize());
    }

    void handleOffline()
    {
        if (offlineSince == juce::Time())
        {
            // First time offline — record timestamp
            offlineSince = juce::Time::getCurrentTime();
            saveLicense();
        }
        else
        {
            // Check grace period
            auto elapsed = juce::Time::getCurrentTime() - offlineSince;
            if (elapsed.inDays() >= kOfflineGraceDays)
            {
                invalidate();
            }
        }
    }
};
} // namespace liveflow
