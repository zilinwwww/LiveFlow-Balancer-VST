#include <cmath>
#include <iostream>
#include <vector>

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

#include "core/BalancerEngine.h"
#include "core/BalancerMath.h"
#include "core/LookAheadDelay.h"
#include "core/LoudnessEstimator.h"
#include "core/PluginParameters.h"
#include "core/VisualizationState.h"

namespace
{

class BalancerMathTest final : public juce::UnitTest
{
public:
    BalancerMathTest() : juce::UnitTest ("Balancer math", "LiveFlow") {}

    void runTest() override
    {
        beginTest ("dbToNormalised strictly bounds to range");
        expectWithinAbsoluteError (liveflow::math::dbToNormalised (liveflow::math::kMinMeterDb - 10.0f), 0.0f, 1.0e-5f);
        expectWithinAbsoluteError (liveflow::math::dbToNormalised (liveflow::math::kMaxMeterDb + 10.0f), 1.0f, 1.0e-5f);
        expectWithinAbsoluteError (liveflow::math::dbToNormalised ((liveflow::math::kMinMeterDb + liveflow::math::kMaxMeterDb) * 0.5f), 0.5f, 1.0e-5f);

        beginTest ("timeMsToCoefficient correctly scales half-life");
        const auto slowCoeff = liveflow::math::timeMsToCoefficient (48000.0, 10000.0f);
        expectGreaterThan (slowCoeff, 0.999f);
        expectLessThan (slowCoeff, 1.0f);

        const auto fastCoeff = liveflow::math::timeMsToCoefficient (48000.0, 1.0f);
        expectWithinAbsoluteError (fastCoeff, 0.9793f, 1.0e-3f);

        beginTest ("Sigmoid and Presence bounds");
        expectWithinAbsoluteError (liveflow::math::sigmoid (0.0f), 0.5f, 1.0e-5f);
        expectWithinAbsoluteError (liveflow::math::sigmoid (100.0f), 1.0f, 1.0e-5f);
        expectWithinAbsoluteError (liveflow::math::sigmoid (-100.0f), 0.0f, 1.0e-5f);

        expectGreaterThan (liveflow::math::computeVoicePresence (-20.0f, -30.0f, 8.0f), 0.7f);
        expectLessThan (liveflow::math::computeVoicePresence (-40.0f, -30.0f, 8.0f), 0.3f);

        beginTest ("Target curve computes correct dynamic ducking gain (0.030 slope)");
        expectWithinAbsoluteError (liveflow::math::computeTargetCurveGainDb (-15.0f, -12.0f, 3.0f, 50.0f, -20.0f), -13.5f, 1.0e-3f);
        expectWithinAbsoluteError (liveflow::math::computeTargetCurveGainDb (-15.0f, -12.0f, 3.0f, 0.0f, -20.0f), -6.0f, 1.0e-3f);

        beginTest ("Gain Clamping");
        expectWithinAbsoluteError (liveflow::math::clampGainDb (-100.0f, 30.0f, 12.0f), -30.0f, 1.0e-5f);
        expectWithinAbsoluteError (liveflow::math::clampGainDb (100.0f, 30.0f, 12.0f), 12.0f, 1.0e-5f);
        expectWithinAbsoluteError (liveflow::math::clampGainDb (0.0f, 30.0f, 12.0f), 0.0f, 1.0e-5f);
    }
};

class LookAheadDelayTest final : public juce::UnitTest
{
public:
    LookAheadDelayTest() : juce::UnitTest ("LookAhead Delay", "LiveFlow") {}

    void runTest() override
    {
        beginTest ("Exact Delay Latency Preservation & Isolation");
        liveflow::LookAheadDelay<float> delay;
        delay.prepare (2, 1000);
        delay.setDelaySamples (50);

        std::vector<float> inputSignal (100000);
        for (size_t i = 0; i < inputSignal.size(); ++i)
            inputSignal[i] = (i % 100 == 0) ? 1.0f : 0.0f;

        for (size_t i = 0; i < inputSignal.size(); ++i)
        {
            const auto delayedLeft = delay.processSample (0, inputSignal[i]);
            const auto delayedRight = delay.processSample (1, 0.0f);
            delay.advance();

            expectWithinAbsoluteError (delayedRight, 0.0f, 1.0e-7f);

            if (i >= 50)
                expectWithinAbsoluteError (delayedLeft, inputSignal[i - 50], 1.0e-7f);
        }
    }
};

class LoudnessEstimatorTest final : public juce::UnitTest
{
public:
    LoudnessEstimatorTest() : juce::UnitTest ("Loudness Estimator", "LiveFlow") {}

    void runTest() override
    {
        beginTest ("Absolute Silence Bottom Limit");
        liveflow::LoudnessEstimator<float> estimator;
        estimator.prepare (48000.0, 2);
        
        std::array<float, 2> silentFrame { 0.0f, 0.0f };
        for (int i = 0; i < 48000 * 5; ++i)
            estimator.pushFrame (silentFrame);
        
        expectLessThan (estimator.getLufs(), -119.0f); // 1.0e-12 floor ensures -120 LUFS lower bound

        beginTest ("K-Weighting Low frequency attenuation (20Hz vs 5kHz)");
        auto getLufsForFreq = [&](float hz) {
            estimator.reset();
            const float increment = juce::MathConstants<float>::twoPi * hz / 48000.0f;
            float phase = 0.0f;
            for (int i = 0; i < 48000 * 2; ++i)
            {
                float sample = 0.5f * std::sin (phase);
                estimator.pushFrame ({ sample, sample });
                phase += increment;
            }
            return estimator.getLufs();
        };

        const auto lufs20Hz = getLufsForFreq (20.0f);
        const auto lufs5kHz = getLufsForFreq (5000.0f);
        
        expectLessThan (lufs20Hz, lufs5kHz - 10.0f); 
    }
};



class BalancerEngineTest final : public juce::UnitTest
{
public:
    BalancerEngineTest() : juce::UnitTest ("Balancer Engine Integration", "LiveFlow") {}

    void runTest() override
    {
        beginTest ("Absence of Sidechain behaves transparently");
        liveflow::BalancerEngine<float> engine;
        engine.prepare (48000.0, 512, 2, 2);
        
        liveflow::RuntimeSettings defaultSettings;
        engine.updateSettings (defaultSettings);
        
        // Populate main with a 100Hz sine wave (above the 38Hz cut-off)
        juce::AudioBuffer<float> mainBuffer (2, 512);
        const float mainIncrement = juce::MathConstants<float>::twoPi * 100.0f / 48000.0f;
        float mainPhase = 0.0f;
        for (int s = 0; s < 512; ++s)
        {
            float val = 0.5f * std::sin(mainPhase);
            mainBuffer.setSample (0, s, val);
            mainBuffer.setSample (1, s, val);
            mainPhase += mainIncrement;
        }
        
        liveflow::VisualizationState visualState;
        
        for (int i = 0; i < 100; ++i)
            engine.process (mainBuffer, nullptr, visualState);
        
        const auto lastSample = mainBuffer.getSample (0, 511);
        const auto expectedLastSample = 0.5f * std::sin (511 * mainIncrement);
        expectWithinAbsoluteError (lastSample, expectedLastSample, 1.0e-3f);

        beginTest ("Auto Anchor Converges Correctly");
        engine.reset();
        
        liveflow::RuntimeSettings anchorSettings;
        anchorSettings.anchorAutoMode = true;
        anchorSettings.noiseGateDb = -60.0f;
        engine.updateSettings (anchorSettings);
        
        juce::AudioBuffer<float> sidechainBuffer (2, 512);
        
        const float increment = juce::MathConstants<float>::twoPi * 1000.0f / 48000.0f;
        float phase = 0.0f;
        for (int s = 0; s < 512; ++s)
        {
            float val = 0.8f * std::sin(phase);
            sidechainBuffer.setSample (0, s, val);
            sidechainBuffer.setSample (1, s, val);
            phase += increment;
        }

        auto latestSidechainLufs = -60.0f;
        auto latestAnchor = -24.0f;

        // Process for over 50 seconds to let the huge 10-sec EMA completely settle
        for (int i = 0; i < 5000; ++i)
        {
            auto currentMain = mainBuffer;
            engine.process (currentMain, &sidechainBuffer, visualState);

            if (i % 50 == 0)
            {
                const auto snap = visualState.capture();
                latestSidechainLufs = snap.vocalLufs;
                latestAnchor = snap.anchorPoint;
            }
        }
        
        const auto finalSnap = visualState.capture();
        expectWithinAbsoluteError (finalSnap.anchorPoint, finalSnap.vocalLufs, 0.5f);
        
        beginTest ("Expansion mode inverts target gain");
        engine.reset();
        
        // RE-FILL mainBuffer with consistent signal since it was modified in place during the prior test stages
        mainPhase = 0.0f;
        for (int s = 0; s < 512; ++s)
        {
            float val = 0.5f * std::sin(mainPhase);
            mainBuffer.setSample (0, s, val);
            mainBuffer.setSample (1, s, val);
            mainPhase += mainIncrement;
        }
        
        liveflow::RuntimeSettings expansionSettings;
        expansionSettings.expansionMode = true;
        expansionSettings.speedMs = 15.0f; 
        expansionSettings.balanceDb = 20.0f; // Force a massive duck ordinarily, so expansion forces a massive boost
        engine.updateSettings (expansionSettings);
        
        for (int i = 0; i < 500; ++i)
        {
            auto currentMain = mainBuffer;
            engine.process (currentMain, &sidechainBuffer, visualState);
        }
        
        const auto expandedSnap = visualState.capture();
        expectGreaterThan (expandedSnap.gainDb, 0.1f);
    }
};

class ConsoleTestRunner final : public juce::UnitTestRunner
{
protected:
    void logMessage (const juce::String& message) override
    {
        std::cout << message << '\n';
    }
};

static BalancerMathTest balancerMathTest;
static LookAheadDelayTest lookAheadDelayTest;
static LoudnessEstimatorTest loudnessEstimatorTest;
static BalancerEngineTest balancerEngineTest;

} // namespace

int main()
{
    ConsoleTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.setPassesAreLogged (true);
    runner.runAllTests (0x4c697665466c6f77LL);

    auto totalFailures = 0;
    for (int index = 0; index < runner.getNumResults(); ++index)
        if (const auto* result = runner.getResult (index))
            totalFailures += result->failures;

    return totalFailures == 0 ? 0 : 1;
}
