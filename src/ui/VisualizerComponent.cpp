#include "VisualizerComponent.h"

namespace
{
juce::String formatLufs (const float value)
{
    return juce::String (juce::roundToInt (value));
}
} // namespace

namespace liveflow
{
VisualizerComponent::VisualizerComponent (const VisualizationState& stateToWatch,
                                          juce::AudioProcessorValueTreeState& parametersToEdit)
    : state (stateToWatch),
      parameters (parametersToEdit)
{
    startTimerHz (30);
}

void VisualizerComponent::paint (juce::Graphics& graphics)
{
    const auto bounds = getLocalBounds().toFloat();

    // Background gradient
    juce::ColourGradient background (juce::Colour (0xf00b1420), bounds.getTopLeft(),
                                     juce::Colour (0xf0131f32), bounds.getBottomRight(), false);
    graphics.setGradientFill (background);
    graphics.fillRoundedRectangle (bounds, 14.0f);

    // Border
    graphics.setColour (juce::Colour (0x18ffffff));
    graphics.drawRoundedRectangle (bounds.reduced (0.75f), 14.0f, 1.0f);

    const auto canvas = getCanvasBounds();

    // Clip to canvas for all inner drawing
    graphics.saveState();
    graphics.reduceClipRegion (canvas.toNearestIntEdges());

    drawNoiseFloorRegion (graphics, canvas);
    drawGrid (graphics, canvas);
    drawReferenceLine (graphics, canvas);
    drawRangeRegion (graphics, canvas);
    drawTargetCurve (graphics, canvas);
    drawTrail (graphics, canvas);
    drawGainVector (graphics, canvas);
    drawStatePoint (graphics, canvas);

    graphics.restoreState();

    // Draw scale labels outside the clip region
    drawInfoOverlay (graphics, canvas);
}

void VisualizerComponent::resized()
{
}

void VisualizerComponent::timerCallback()
{
    snapshot = state.capture();
    repaint();
}

// ═══════════════════════════════════════════
// Drawing sub-functions
// ═══════════════════════════════════════════

void VisualizerComponent::drawGrid (juce::Graphics& graphics, const juce::Rectangle<float> canvas) const
{
    graphics.setColour (juce::Colour (0x0affffff));

    // Horizontal grid lines (backing LUFS)
    for (float lufs = -55.0f; lufs <= -10.0f; lufs += 5.0f)
    {
        const auto y = lufsToY (canvas, lufs);
        graphics.drawHorizontalLine (juce::roundToInt (y), canvas.getX(), canvas.getRight());
    }

    // Vertical grid lines (vocal LUFS)
    for (float lufs = -55.0f; lufs <= -10.0f; lufs += 5.0f)
    {
        const auto x = lufsToX (canvas, lufs);
        graphics.drawVerticalLine (juce::roundToInt (x), canvas.getY(), canvas.getBottom());
    }

    // Major grid lines every 10 dB
    graphics.setColour (juce::Colour (0x14ffffff));
    for (float lufs = -50.0f; lufs <= -10.0f; lufs += 10.0f)
    {
        const auto y = lufsToY (canvas, lufs);
        const auto x = lufsToX (canvas, lufs);
        graphics.drawHorizontalLine (juce::roundToInt (y), canvas.getX(), canvas.getRight());
        graphics.drawVerticalLine (juce::roundToInt (x), canvas.getY(), canvas.getBottom());
    }
}

void VisualizerComponent::drawReferenceLine (juce::Graphics& graphics, const juce::Rectangle<float> canvas) const
{
    // 45° line: vocal LUFS == backing LUFS
    const auto p1 = lufsToPoint (canvas, kMinLufs, kMinLufs);
    const auto p2 = lufsToPoint (canvas, kMaxLufs, kMaxLufs);

    graphics.setColour (juce::Colour (0x18ffffff));
    const float dashLengths[] = { 6.0f, 4.0f };
    juce::Path dashPath;
    dashPath.startNewSubPath (p1);
    dashPath.lineTo (p2);
    juce::PathStrokeType stroke (1.0f);
    stroke.createDashedStroke (dashPath, dashPath, dashLengths, 2);
    graphics.strokePath (dashPath, juce::PathStrokeType (1.0f));
}

void VisualizerComponent::drawNoiseFloorRegion (juce::Graphics& graphics, const juce::Rectangle<float> canvas) const
{
    const auto noiseFloor = getNoiseGateDb();
    const auto noiseX = lufsToX (canvas, noiseFloor);

    // Dark overlay for noise floor region
    auto noiseRegion = canvas.withRight (noiseX);
    juce::ColourGradient noiseFill (juce::Colour (0x30000000), noiseRegion.getRight(), noiseRegion.getCentreY(),
                                    juce::Colour (0x18000000), noiseRegion.getX(), noiseRegion.getCentreY(), false);
    graphics.setGradientFill (noiseFill);
    graphics.fillRect (noiseRegion);

    // Noise floor boundary line
    const bool isGateActive = (hoverMode == DragMode::noiseGate || currentDragMode == DragMode::noiseGate);
    graphics.setColour (isGateActive ? juce::Colour (0x80ffffff) : juce::Colour (0x40ffffff));
    const float dashLengths[] = { 4.0f, 3.0f };
    juce::Path boundaryPath;
    boundaryPath.startNewSubPath (noiseX, canvas.getY());
    boundaryPath.lineTo (noiseX, canvas.getBottom());
    juce::PathStrokeType stroke (isGateActive ? 2.0f : 1.0f);
    juce::Path dashedBoundary;
    stroke.createDashedStroke (dashedBoundary, boundaryPath, dashLengths, 2);
    graphics.strokePath (dashedBoundary, stroke);

    // Label
    graphics.setColour (juce::Colour (0x60ffffff));
    graphics.setFont (juce::FontOptions (8.5f));
    graphics.drawText ("NOISE", juce::Rectangle<float> (noiseX - 32.0f, canvas.getY() + 4.0f, 28.0f, 12.0f),
                       juce::Justification::centredRight);
}

void VisualizerComponent::drawTargetCurve (juce::Graphics& graphics, const juce::Rectangle<float> canvas) const
{
    const auto noiseFloor = getNoiseGateDb();

    juce::Path curvePath;
    bool started = false;

    // Sample the curve across the vocal LUFS range
    for (float vLufs = kMinLufs; vLufs <= kMaxLufs; vLufs += 0.5f)
    {
        float targetBacking;
        if (vLufs < noiseFloor)
        {
            // Below noise floor: don't draw the active curve
            continue;
        }
        else
        {
            targetBacking = computeTargetBackingLufs (vLufs);
        }

        const auto point = lufsToPoint (canvas, vLufs, targetBacking);

        if (! started)
        {
            curvePath.startNewSubPath (point);
            started = true;
        }
        else
        {
            curvePath.lineTo (point);
        }
    }

    if (started)
    {
        const bool isCurveActive = (hoverMode == DragMode::balance) || (currentDragMode == DragMode::balance) || 
                                   (hoverMode == DragMode::dynamics) || (currentDragMode == DragMode::dynamics);

        // Glow effect
        graphics.setColour (juce::Colour (isCurveActive ? 0x243ecfd5 : 0x183ecfd5));
        graphics.strokePath (curvePath, juce::PathStrokeType (isCurveActive ? 12.0f : 8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Main curve line
        juce::ColourGradient curveGradient (juce::Colour (0xff3ecfd5), canvas.getX(), canvas.getCentreY(),
                                             juce::Colour (0xff6eedf3), canvas.getRight(), canvas.getCentreY(), false);
        graphics.setGradientFill (curveGradient);
        graphics.strokePath (curvePath, juce::PathStrokeType (isCurveActive ? 3.5f : 2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Draw anchor point marker
    const auto anchorLufs = snapshot.anchorPoint;
    const auto anchorTargetBacking = computeTargetBackingLufs (anchorLufs);
    const auto anchorPos = lufsToPoint (canvas, anchorLufs, anchorTargetBacking);

    if (canvas.contains (anchorPos))
    {
        const bool isAnchorActive = (hoverMode == DragMode::anchor) || (currentDragMode == DragMode::anchor);
        const float radiusMultiplier = isAnchorActive ? 1.4f : 1.0f;

        // Outer glow
        graphics.setColour (juce::Colour (isAnchorActive ? 0x303ecfd5 : 0x203ecfd5));
        graphics.fillEllipse (anchorPos.x - 8.0f * radiusMultiplier, anchorPos.y - 8.0f * radiusMultiplier, 16.0f * radiusMultiplier, 16.0f * radiusMultiplier);
        // Inner dot
        graphics.setColour (juce::Colour (0xff3ecfd5));
        graphics.fillEllipse (anchorPos.x - 4.0f * radiusMultiplier, anchorPos.y - 4.0f * radiusMultiplier, 8.0f * radiusMultiplier, 8.0f * radiusMultiplier);
        // Center white
        graphics.setColour (juce::Colour (0xddffffff));
        graphics.fillEllipse (anchorPos.x - 2.0f * radiusMultiplier, anchorPos.y - 2.0f * radiusMultiplier, 4.0f * radiusMultiplier, 4.0f * radiusMultiplier);
    }
}

void VisualizerComponent::drawRangeRegion (juce::Graphics& graphics, const juce::Rectangle<float> canvas) const
{
    const auto boostCeiling = getBoostCeilingDb();
    const auto duckFloor = getDuckFloorDb();
    const auto noiseFloor = getNoiseGateDb();
    const auto rangeLimit = getRangeDb();
    const bool isExpander = getExpansionMode();

    // Mathematically swap roles based on Expand vs Compress
    // Compress: Upper represents ducking (capped by duckFloor), Lower represents boosting (capped by boostCeiling)
    // Expand:   Upper represents boosting (capped by boostCeiling), Lower represents ducking (capped by duckFloor)
    const auto upperSize = isExpander ? std::min (boostCeiling, rangeLimit) : std::min (duckFloor, rangeLimit);
    const auto lowerSize = isExpander ? std::min (duckFloor, rangeLimit) : std::min (boostCeiling, rangeLimit);

    // 1. Build a deterministic array of LUFS sample points to prevent interpolation gaps at float boundaries
    std::vector<float> lufsPoints;
    lufsPoints.push_back (noiseFloor);

    // Add all integer points strictly inside the bounds
    const int startInt = static_cast<int> (std::ceil (noiseFloor));
    const int endInt = static_cast<int> (std::floor (kMaxLufs));
    for (int i = startInt; i <= endInt; ++i)
    {
        const float val = static_cast<float> (i);
        if (val > noiseFloor && val < kMaxLufs)
            lufsPoints.push_back (val);
    }
    
    // Ensure we absolutely hit the right edge
    if (lufsPoints.empty() || std::abs (lufsPoints.back() - kMaxLufs) > 0.001f)
        lufsPoints.push_back (kMaxLufs);

    juce::Path curvePath;
    bool started = false;

    for (const float vLufs : lufsPoints)
    {
        const auto targetBacking = computeTargetBackingLufs (vLufs);
        const auto curvePoint = lufsToPoint (canvas, vLufs, targetBacking);

        if (! started)
        {
            curvePath.startNewSubPath (curvePoint);
            started = true;
        }
        else
        {
            curvePath.lineTo (curvePoint);
        }
    }

    if (! started)
        return;

    // 2. Build the fill region between curve and upper limit (Iterating Backwards exactly on the same points)
    juce::Path upperRegion;
    upperRegion.addPath (curvePath);
    for (auto it = lufsPoints.rbegin(); it != lufsPoints.rend(); ++it)
    {
        const float vLufs = *it;
        const auto targetBacking = computeTargetBackingLufs (vLufs);
        const auto upperPoint = lufsToPoint (canvas, vLufs, targetBacking + upperSize);
        upperRegion.lineTo (upperPoint);
    }
    upperRegion.closeSubPath();

    // 3. Build the fill region between curve and lower limit
    juce::Path lowerRegion;
    lowerRegion.addPath (curvePath);
    for (auto it = lufsPoints.rbegin(); it != lufsPoints.rend(); ++it)
    {
        const float vLufs = *it;
        const auto targetBacking = computeTargetBackingLufs (vLufs);
        const auto lowerPoint = lufsToPoint (canvas, vLufs, targetBacking - lowerSize);
        lowerRegion.lineTo (lowerPoint);
    }
    lowerRegion.closeSubPath();

    if (isExpander)
    {
        // Expand Mode
        graphics.setColour (juce::Colour (0x1a81e69d)); // Upper is Boost (Green)
        graphics.fillPath (upperRegion);
        graphics.setColour (juce::Colour (0x1affaf6e)); // Lower is Duck (Yellow)
        graphics.fillPath (lowerRegion);
    }
    else
    {
        // Compress Mode
        graphics.setColour (juce::Colour (0x1affaf6e)); // Upper is Duck (Yellow)
        graphics.fillPath (upperRegion);
        graphics.setColour (juce::Colour (0x1a81e69d)); // Lower is Boost (Green)
        graphics.fillPath (lowerRegion);
    }
}

void VisualizerComponent::drawTrail (juce::Graphics& graphics, const juce::Rectangle<float> canvas) const
{
    const auto trailCount = static_cast<int> (snapshot.trail.size());
    if (trailCount == 0) return;

    for (int i = 0; i < trailCount; ++i)
    {
        const auto orderedIndex = (snapshot.trailHeadIndex + i) % trailCount;
        const auto& point = snapshot.trail[static_cast<size_t> (orderedIndex)];

        if (point.vocalLufs < kMinLufs + 1.0f && point.backingLufs < kMinLufs + 1.0f)
            continue;

        const auto age = static_cast<float> (trailCount - 1 - i) / static_cast<float> (trailCount - 1);
        const auto alpha = juce::jlimit (0.0f, 0.4f, (1.0f - age) * 0.4f);
        const auto pos = lufsToPoint (canvas, point.vocalLufs, point.backingLufs);

        if (! canvas.contains (pos))
            continue;

        const auto size = 2.0f + (1.0f - age) * 2.0f;
        graphics.setColour (juce::Colour (0xffffffff).withAlpha (alpha));
        graphics.fillEllipse (pos.x - size * 0.5f, pos.y - size * 0.5f, size, size);
    }
}

void VisualizerComponent::drawStatePoint (juce::Graphics& graphics, const juce::Rectangle<float> canvas) const
{
    if (! snapshot.sidechainConnected)
        return;

    const auto pos = lufsToPoint (canvas, snapshot.vocalLufs, snapshot.backingLufs);

    if (! canvas.expanded (20.0f).contains (pos))
        return;

    // Determine color based on gain
    const auto gainDb = snapshot.gainDb;
    juce::Colour pointColour;
    if (gainDb < -0.5f)
        pointColour = juce::Colour (0xffffaf6e).interpolatedWith (juce::Colour (0xffff5e50), juce::jlimit (0.0f, 1.0f, std::abs (gainDb) / 12.0f));
    else if (gainDb > 0.5f)
        pointColour = juce::Colour (0xff81e69d).interpolatedWith (juce::Colour (0xff38bda1), juce::jlimit (0.0f, 1.0f, gainDb / 12.0f));
    else
        pointColour = juce::Colour (0xff3ecfd5);

    // Outer glow
    graphics.setColour (pointColour.withAlpha (0.15f));
    graphics.fillEllipse (pos.x - 14.0f, pos.y - 14.0f, 28.0f, 28.0f);
    graphics.setColour (pointColour.withAlpha (0.3f));
    graphics.fillEllipse (pos.x - 8.0f, pos.y - 8.0f, 16.0f, 16.0f);

    // Main dot
    graphics.setColour (pointColour);
    graphics.fillEllipse (pos.x - 5.0f, pos.y - 5.0f, 10.0f, 10.0f);

    // Bright center
    graphics.setColour (juce::Colour (0xccffffff));
    graphics.fillEllipse (pos.x - 2.5f, pos.y - 2.5f, 5.0f, 5.0f);
}

void VisualizerComponent::drawGainVector (juce::Graphics& graphics, const juce::Rectangle<float> canvas) const
{
    if (! snapshot.sidechainConnected || snapshot.voicePresence < 0.05f)
        return;

    const auto statePos = lufsToPoint (canvas, snapshot.vocalLufs, snapshot.backingLufs);
    const auto targetBacking = computeTargetBackingLufs (snapshot.vocalLufs);
    const auto targetPos = lufsToPoint (canvas, snapshot.vocalLufs, targetBacking);

    if (! canvas.expanded (20.0f).contains (statePos))
        return;

    const auto gainDb = snapshot.gainDb;
    juce::Colour vectorColour;
    if (gainDb < -0.3f)
        vectorColour = juce::Colour (0xffffaf6e);
    else if (gainDb > 0.3f)
        vectorColour = juce::Colour (0xff81e69d);
    else
        return; // No significant gain, no vector to draw

    graphics.setColour (vectorColour.withAlpha (0.6f * snapshot.voicePresence));
    graphics.drawLine (statePos.x, statePos.y, targetPos.x, targetPos.y, 2.0f);

    // Small arrowhead/diamond at the target end
    graphics.setColour (vectorColour.withAlpha (0.4f * snapshot.voicePresence));
    graphics.fillEllipse (targetPos.x - 3.0f, targetPos.y - 3.0f, 6.0f, 6.0f);
}

void VisualizerComponent::drawInfoOverlay (juce::Graphics& graphics, const juce::Rectangle<float> canvas) const
{
    // Y-axis labels (backing LUFS) — on the left
    graphics.setColour (juce::Colour (0xff5a7088));
    graphics.setFont (juce::FontOptions (9.0f));

    for (float lufs = -50.0f; lufs <= -10.0f; lufs += 10.0f)
    {
        const auto y = lufsToY (canvas, lufs);
        graphics.drawText (formatLufs (lufs),
                           juce::Rectangle<float> (canvas.getX() - 32.0f, y - 6.0f, 28.0f, 12.0f),
                           juce::Justification::centredRight);
    }

    // X-axis labels (vocal LUFS) — on the bottom
    for (float lufs = -50.0f; lufs <= -10.0f; lufs += 10.0f)
    {
        const auto x = lufsToX (canvas, lufs);
        graphics.drawText (formatLufs (lufs),
                           juce::Rectangle<float> (x - 14.0f, canvas.getBottom() + 2.0f, 28.0f, 12.0f),
                           juce::Justification::centred);
    }

    // Axis titles
    graphics.setColour (juce::Colour (0xff89a0ba));
    graphics.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    graphics.drawText ("VOCAL  LUFS  ->",
                       juce::Rectangle<float> (canvas.getCentreX() - 50.0f, canvas.getBottom() + 12.0f, 100.0f, 14.0f),
                       juce::Justification::centred);

    // Rotated Y-axis title
    graphics.saveState();
    graphics.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi,
                                                             canvas.getX() - 36.0f, canvas.getCentreY()));
    graphics.drawText ("BACKING  LUFS  ->",
                       juce::Rectangle<float> (canvas.getX() - 86.0f, canvas.getCentreY() - 7.0f, 100.0f, 14.0f),
                       juce::Justification::centred);
    graphics.restoreState();

    // Top-right badges
    auto topRight = getLocalBounds().toFloat().reduced (12.0f).removeFromTop (20.0f);

    // Sidechain status badge
    const auto scText = snapshot.sidechainConnected ? "SC ACTIVE" : "SC OFFLINE";
    const auto scColour = snapshot.sidechainConnected ? juce::Colour (0xff35d1c7) : juce::Colour (0xff7f95ab);
    auto scBadge = topRight.removeFromRight (80.0f);

    graphics.setColour (scColour.withAlpha (0.12f));
    graphics.fillRoundedRectangle (scBadge, 10.0f);
    graphics.setColour (scColour.withAlpha (0.52f));
    graphics.drawRoundedRectangle (scBadge.reduced (0.5f), 10.0f, 1.0f);
    graphics.setColour (scColour.brighter (0.18f));
    graphics.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    graphics.drawText (scText, scBadge, juce::Justification::centred);

    topRight.removeFromRight (6.0f);

    // Gain badge
    auto gainBadge = topRight.removeFromRight (70.0f);
    const auto gainColour = snapshot.gainDb < -0.3f ? juce::Colour (0xffffaa73)
                          : snapshot.gainDb > 0.3f  ? juce::Colour (0xff7ee6aa)
                                                    : juce::Colour (0xff89a0ba);
    const auto gainPrefix = snapshot.gainDb > 0.05f ? "+" : "";
    const auto gainText = gainPrefix + juce::String (snapshot.gainDb, 1) + " dB";

    graphics.setColour (juce::Colour (0x12000000));
    graphics.fillRoundedRectangle (gainBadge, 10.0f);
    graphics.setColour (gainColour.withAlpha (0.5f));
    graphics.drawRoundedRectangle (gainBadge.reduced (0.5f), 10.0f, 1.0f);
    graphics.setColour (gainColour);
    graphics.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    graphics.drawText (gainText, gainBadge, juce::Justification::centred);
}

// ═══════════════════════════════════════════
// Coordinate conversion
// ═══════════════════════════════════════════

float VisualizerComponent::lufsToX (const juce::Rectangle<float> canvas, const float lufs) const noexcept
{
    return juce::jmap (juce::jlimit (kMinLufs, kMaxLufs, lufs), kMinLufs, kMaxLufs, canvas.getX(), canvas.getRight());
}

float VisualizerComponent::lufsToY (const juce::Rectangle<float> canvas, const float lufs) const noexcept
{
    // Y axis is inverted: higher LUFS at top
    return juce::jmap (juce::jlimit (kMinLufs, kMaxLufs, lufs), kMinLufs, kMaxLufs, canvas.getBottom(), canvas.getY());
}

juce::Point<float> VisualizerComponent::lufsToPoint (const juce::Rectangle<float> canvas,
                                                      const float vocalLufs,
                                                      const float backingLufs) const noexcept
{
    return { lufsToX (canvas, vocalLufs), lufsToY (canvas, backingLufs) };
}

// ═══════════════════════════════════════════
// Target curve
// ═══════════════════════════════════════════

float VisualizerComponent::computeTargetBackingLufs (const float vocalLufs) const noexcept
{
    const auto balance = getBalanceDb();
    const auto dynamics = getDynamicsPercent();
    const auto anchor = snapshot.anchorPoint;

    const auto slope = dynamics * 0.030f;
    const auto dynamicOffset = balance + slope * (vocalLufs - anchor);
    return vocalLufs - dynamicOffset;
}

juce::Rectangle<float> VisualizerComponent::getCanvasBounds() const noexcept
{
    auto bounds = getLocalBounds().toFloat().reduced (12.0f);
    bounds.removeFromTop (26.0f);    // Space for badges
    bounds.removeFromBottom (28.0f); // Space for X-axis labels
    bounds.removeFromLeft (38.0f);   // Space for Y-axis labels
    bounds.removeFromRight (8.0f);   // Right padding
    return bounds;
}

// ═══════════════════════════════════════════
// Parameter reading
// ═══════════════════════════════════════════

float VisualizerComponent::getBalanceDb() const noexcept
{
    if (auto* p = parameters.getRawParameterValue (param::balance))
        return p->load();
    return 3.0f;
}

float VisualizerComponent::getDynamicsPercent() const noexcept
{
    if (auto* p = parameters.getRawParameterValue (param::dynamics))
        return p->load();
    return 0.0f;
}

bool VisualizerComponent::getExpansionMode() const noexcept
{
    if (auto* p = parameters.getRawParameterValue (param::expansionMode))
        return p->load() > 0.5f;
    return false;
}

float VisualizerComponent::getNoiseGateDb() const noexcept
{
    if (auto* p = parameters.getRawParameterValue (param::noiseGate))
        return p->load();
    return -45.0f;
}

float VisualizerComponent::getBoostCeilingDb() const noexcept
{
    if (auto* p = parameters.getRawParameterValue (param::boostCeiling))
        return p->load();
    return 6.0f;
}

float VisualizerComponent::getDuckFloorDb() const noexcept
{
    if (auto* p = parameters.getRawParameterValue (param::duckFloor))
        return p->load();
    return 10.0f;
}

float VisualizerComponent::getRangeDb() const noexcept
{
    if (auto* p = parameters.getRawParameterValue (param::range))
        return p->load();
    return 8.0f;
}

bool VisualizerComponent::getAnchorAuto() const noexcept
{
    if (auto* p = parameters.getRawParameterValue (param::anchorAuto))
        return p->load() > 0.5f;
    return true;
}

float VisualizerComponent::getAnchorManualDb() const noexcept
{
    if (auto* p = parameters.getRawParameterValue (param::anchorManual))
        return p->load();
    return -20.0f;
}

// ═══════════════════════════════════════════
// Mouse Interactions
// ═══════════════════════════════════════════

VisualizerComponent::DragMode VisualizerComponent::detectDragMode (const juce::Point<float>& position) const
{
    const auto canvas = getCanvasBounds();
    if (! canvas.contains (position))
        return DragMode::none;

    const auto mouseLufsX = juce::jmap (position.x, canvas.getX(), canvas.getRight(), kMinLufs, kMaxLufs);

    // 1. Noise gate
    const auto noiseGate = getNoiseGateDb();
    const auto noiseGateX = lufsToX (canvas, noiseGate);
    if (std::abs (position.x - noiseGateX) <= 8.0f)
        return DragMode::noiseGate;

    // 2. Anchor point
    const auto anchorX = lufsToX (canvas, snapshot.anchorPoint);
    const auto anchorY = lufsToY (canvas, computeTargetBackingLufs (snapshot.anchorPoint));
    if (position.getDistanceFrom (juce::Point<float> (anchorX, anchorY)) <= 12.0f)
        return DragMode::anchor;

    // 3. Balance (Curve hover)
    if (mouseLufsX > noiseGate)
    {
        const auto expectedY = lufsToY (canvas, computeTargetBackingLufs (mouseLufsX));
        if (std::abs (position.y - expectedY) <= 8.0f)
            return DragMode::balance;
    }

    return DragMode::none;
}

void VisualizerComponent::updateCursor (DragMode mode)
{
    switch (mode)
    {
        case DragMode::balance:
            setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
            break;
        case DragMode::anchor:
            setMouseCursor (getAnchorAuto() ? juce::MouseCursor::NormalCursor : juce::MouseCursor::LeftRightResizeCursor);
            break;
        case DragMode::noiseGate:
            setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
            break;
        default:
            setMouseCursor (juce::MouseCursor::NormalCursor);
            break;
    }
}

juce::RangedAudioParameter* VisualizerComponent::getParameterForMode (DragMode mode) const
{
    switch (mode)
    {
        case DragMode::balance:     return parameters.getParameter (param::balance);
        case DragMode::dynamics:    return parameters.getParameter (param::dynamics);
        case DragMode::anchor:      return parameters.getParameter (param::anchorManual);
        case DragMode::noiseGate:   return parameters.getParameter (param::noiseGate);
        default:                    return nullptr;
    }
}

void VisualizerComponent::commitParameterChange (juce::RangedAudioParameter* param, float unnormalizedValue) const
{
    if (param != nullptr)
        param->setValueNotifyingHost (param->convertTo0to1 (unnormalizedValue));
}

void VisualizerComponent::mouseMove (const juce::MouseEvent& e)
{
    const auto newHoverMode = detectDragMode (e.position);
    if (hoverMode != newHoverMode)
    {
        hoverMode = newHoverMode;
        updateCursor (hoverMode);
        repaint();
    }
}

void VisualizerComponent::mouseDown (const juce::MouseEvent& e)
{
    currentDragMode = detectDragMode (e.position);

    if (currentDragMode == DragMode::anchor && getAnchorAuto())
        currentDragMode = DragMode::none; // Auto anchor is non-draggable

    if (currentDragMode == DragMode::none)
        return;

    if (auto* param = getParameterForMode (currentDragMode))
    {
        param->beginChangeGesture();
        dragStartParameterValue = param->convertFrom0to1 (param->getValue());
        dragStartX = e.position.x;
        dragStartY = e.position.y;
    }
}

void VisualizerComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (currentDragMode == DragMode::none)
        return;

    auto* param = getParameterForMode (currentDragMode);
    if (! param) return;

    const auto canvas = getCanvasBounds();

    if (currentDragMode == DragMode::balance)
    {
        const auto lufsRange = kMaxLufs - kMinLufs;
        const auto dbPerPixel = lufsRange / canvas.getHeight();
        const auto deltaDb = (e.position.y - dragStartY) * dbPerPixel;
        commitParameterChange (param, dragStartParameterValue + deltaDb);
    }
    else if (currentDragMode == DragMode::noiseGate || currentDragMode == DragMode::anchor)
    {
        const auto lufsRange = kMaxLufs - kMinLufs;
        const auto dbPerPixel = lufsRange / canvas.getWidth();
        const auto deltaDb = (e.position.x - dragStartX) * dbPerPixel;
        commitParameterChange (param, dragStartParameterValue + deltaDb);
    }
}

void VisualizerComponent::mouseUp (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    if (currentDragMode != DragMode::none)
    {
        if (auto* param = getParameterForMode (currentDragMode))
            param->endChangeGesture();
        
        currentDragMode = DragMode::none;
    }
}

void VisualizerComponent::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    const auto mode = detectDragMode (e.position);
    if (mode == DragMode::balance || mode == DragMode::anchor)
    {
        if (auto* param = getParameterForMode (DragMode::dynamics))
        {
            param->beginChangeGesture();
            const auto currentValue = param->convertFrom0to1 (param->getValue());
            const auto delta = wheel.deltaY * 50.0f;
            commitParameterChange (param, currentValue + delta);
            param->endChangeGesture();
        }
    }
}

void VisualizerComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto mode = detectDragMode (e.position);
    if (mode != DragMode::none)
    {
        // For anchor, if user double clicks, we could toggle Auto.
        if (mode == DragMode::anchor)
        {
            if (auto* param = parameters.getParameter (param::anchorAuto))
            {
                param->beginChangeGesture();
                param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f);
                param->endChangeGesture();
                return;
            }
        }
        
        if (auto* param = getParameterForMode (mode))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->getDefaultValue());
            param->endChangeGesture();
        }
    }
}

} // namespace liveflow
