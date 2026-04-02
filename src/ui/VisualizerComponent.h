#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "core/BalancerMath.h"
#include "core/PluginParameters.h"
#include "core/VisualizationState.h"

namespace liveflow
{
/**
 * VisualizerComponent — Interactive Target Curve Canvas
 *
 * Core of the FabFilter-style UI. Shows:
 *   - Target curve (white/cyan line) defined by Balance + Dynamics + AnchorPoint
 *   - Real-time state point (glowing dot at current vocal/backing LUFS)
 *   - State point trail (fading positions over last ~2-3 seconds)
 *   - Gain vector (vertical line from state point to curve)
 *   - 45° reference line (vocal = backing)
 *   - Noise floor region (dark area on the left)
 *   - Grid + scale labels
 *
 * X axis: Vocal LUFS (-60 to -5)
 * Y axis: Backing Target LUFS (-60 to -5)
 */
class VisualizerComponent final : public juce::Component,
                                  private juce::Timer
{
public:
    VisualizerComponent (const VisualizationState& stateToWatch,
                         juce::AudioProcessorValueTreeState& parametersToEdit);
    ~VisualizerComponent() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Mouse interactions
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

private:
    // Canvas coordinate system
    static constexpr float kMinLufs = -60.0f;
    static constexpr float kMaxLufs = -5.0f;

    void timerCallback() override;

    enum class DragMode
    {
        none,
        balance,
        dynamics,
        anchor,
        noiseGate
    };

    DragMode detectDragMode (const juce::Point<float>& position) const;
    void updateCursor (DragMode mode);
    juce::RangedAudioParameter* getParameterForMode (DragMode mode) const;
    void commitParameterChange (juce::RangedAudioParameter* param, float unnormalizedValue) const;

    DragMode currentDragMode = DragMode::none;
    DragMode hoverMode = DragMode::none;
    float dragStartParameterValue = 0.0f;
    float dragStartX = 0.0f;
    float dragStartY = 0.0f;

    // Drawing sub-functions
    void drawGrid (juce::Graphics&, juce::Rectangle<float> canvas) const;
    void drawReferenceLine (juce::Graphics&, juce::Rectangle<float> canvas) const;
    void drawNoiseFloorRegion (juce::Graphics&, juce::Rectangle<float> canvas) const;
    void drawTargetCurve (juce::Graphics&, juce::Rectangle<float> canvas) const;
    void drawRangeRegion (juce::Graphics&, juce::Rectangle<float> canvas) const;
    void drawTrail (juce::Graphics&, juce::Rectangle<float> canvas) const;
    void drawStatePoint (juce::Graphics&, juce::Rectangle<float> canvas) const;
    void drawGainVector (juce::Graphics&, juce::Rectangle<float> canvas) const;
    void drawInfoOverlay (juce::Graphics&, juce::Rectangle<float> canvas) const;

    // Coordinate conversion
    [[nodiscard]] float lufsToX (juce::Rectangle<float> canvas, float lufs) const noexcept;
    [[nodiscard]] float lufsToY (juce::Rectangle<float> canvas, float lufs) const noexcept;
    [[nodiscard]] juce::Point<float> lufsToPoint (juce::Rectangle<float> canvas, float vocalLufs, float backingLufs) const noexcept;

    // Target curve computation (mirrors BalancerMath but for display)
    [[nodiscard]] float computeTargetBackingLufs (float vocalLufs) const noexcept;

    // Get the inner plot area (excludes scale labels)
    [[nodiscard]] juce::Rectangle<float> getCanvasBounds() const noexcept;

    // Read current parameter values
    [[nodiscard]] float getBalanceDb() const noexcept;
    [[nodiscard]] float getDynamicsPercent() const noexcept;
    [[nodiscard]] bool getExpansionMode() const noexcept;
    [[nodiscard]] float getNoiseGateDb() const noexcept;
    [[nodiscard]] float getBoostCeilingDb() const noexcept;
    [[nodiscard]] float getDuckFloorDb() const noexcept;
    [[nodiscard]] float getRangeDb() const noexcept;
    [[nodiscard]] float getAnchorManualDb() const noexcept;
    [[nodiscard]] bool getAnchorAuto() const noexcept;

    const VisualizationState& state;
    juce::AudioProcessorValueTreeState& parameters;
    VisualizationState::Snapshot snapshot;
};
} // namespace liveflow
