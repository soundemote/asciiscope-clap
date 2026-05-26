/*
 * SideQuest Starting Point
 *
 * Basically lets paul bootstrap his projects.
 *
 * Copyright 2024-2025, Paul Walker and Various authors, as described in the github
 * transaction log.
 *
 * This source repo is released under the MIT license, but has
 * GPL3 dependencies, as such the combined work will be
 * released under GPL3.
 *
 * The source code and license are at https://github.com/baconpaul/sidequest-startingpoint
 */

#include "asciiscope-visual-component.h"

#include <algorithm>
#include <cmath>

namespace baconpaul::sidequest_ns::ui
{
namespace
{
constexpr auto glyphRamp = " .:-=+*#%@";

const char *modeName(int mode)
{
    switch (mode)
    {
    case 1:
        return "mirror";
    case 2:
        return "spectral";
    default:
        return "wave";
    }
}

const char *paletteName(int palette)
{
    switch (palette)
    {
    case 1:
        return "ember";
    case 2:
        return "ice";
    default:
        return "neon";
    }
}

const char *traceInterpolationName(int mode)
{
    switch (mode)
    {
    case 1:
        return "linear";
    case 2:
        return "catmull";
    default:
        return "off";
    }
}

juce::Colour phosphorFor(float energy, int palette)
{
    const auto e = std::clamp(energy, 0.0f, 1.0f);
    switch (palette % 3)
    {
    case 1:
        if (e > 0.82f)
            return juce::Colour(0xfffff3d8);
        if (e > 0.58f)
            return juce::Colour(0xffffb000);
        if (e > 0.32f)
            return juce::Colour(0xffff4a3d);
        return juce::Colour(0xff7a1e15).withAlpha(0.82f);
    case 2:
        if (e > 0.82f)
            return juce::Colour(0xffffffff);
        if (e > 0.58f)
            return juce::Colour(0xffa6f6ff);
        if (e > 0.32f)
            return juce::Colour(0xff67a6ff);
        return juce::Colour(0xff233d88).withAlpha(0.82f);
    default:
        if (e > 0.82f)
            return juce::Colour(0xfff7fbff);
        if (e > 0.58f)
            return juce::Colour(0xff5efcff);
        if (e > 0.32f)
            return juce::Colour(0xffb35cff);
        return juce::Colour(0xff276dff).withAlpha(0.82f);
    }
}

float readHistorySample(const std::array<float, AsciiscopeVisualComponent::historySize> &history,
                        uint32_t write, uint32_t count, int sampleOffset)
{
    if (count == 0)
        return 0.0f;

    const auto clampedOffset =
        static_cast<uint32_t>(std::clamp(sampleOffset, 0, static_cast<int>(count) - 1));
    const auto oldest = (write + AsciiscopeVisualComponent::historySize - count) %
                        AsciiscopeVisualComponent::historySize;
    const auto index = (oldest + clampedOffset) % AsciiscopeVisualComponent::historySize;
    return history[index];
}

float readHistory(const std::array<float, AsciiscopeVisualComponent::historySize> &history,
                  uint32_t write, uint32_t count, float position, int interpolationMode)
{
    if (count == 0)
        return 0.0f;

    if (count == 1)
        return readHistorySample(history, write, count, 0);

    const auto clampedPosition = std::clamp(position, 0.0f, 1.0f);
    const auto samplePosition = clampedPosition * static_cast<float>(count - 1);
    if (interpolationMode <= 0)
        return readHistorySample(history, write, count,
                                 static_cast<int>(std::round(samplePosition)));

    const auto i1 = static_cast<int>(std::floor(samplePosition));
    const auto amount = samplePosition - static_cast<float>(i1);
    const auto y1 = readHistorySample(history, write, count, i1);
    const auto y2 = readHistorySample(history, write, count, i1 + 1);

    if (interpolationMode == 1)
        return y1 + (y2 - y1) * amount;

    const auto y0 = readHistorySample(history, write, count, i1 - 1);
    const auto y3 = readHistorySample(history, write, count, i1 + 2);
    const auto t2 = amount * amount;
    const auto t3 = t2 * amount;
    return 0.5f * ((2.0f * y1) + (-y0 + y2) * amount +
                   (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3) * t2 +
                   (-y0 + 3.0f * y1 - 3.0f * y2 + y3) * t3);
}

void drawMeter(juce::Graphics &g, juce::Rectangle<float> bounds, float level, float peakHold,
               juce::Colour colour, const char *label)
{
    const auto value = std::clamp(level, 0.0f, 1.0f);
    const auto held = std::clamp(peakHold, 0.0f, 1.0f);
    g.setColour(juce::Colour(0xff07101c).withAlpha(0.82f));
    g.fillRect(bounds);
    g.setColour(juce::Colour(0xff18304d).withAlpha(0.78f));
    g.drawRect(bounds, 1.0f);

    auto fill = bounds.reduced(1.0f);
    fill.setWidth(fill.getWidth() * value);
    g.setColour(colour.withAlpha(0.82f));
    g.fillRect(fill);

    const auto tickX = juce::jmap(held, 0.0f, 1.0f, bounds.getX() + 1.0f, bounds.getRight() - 2.0f);
    g.setColour(juce::Colour(0xfff7fbff).withAlpha(0.82f));
    g.fillRect(juce::Rectangle<float>(tickX, bounds.getY() + 1.0f, 2.0f, bounds.getHeight() - 2.0f));

    g.setColour(juce::Colour(0xffd7faff).withAlpha(0.74f));
    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));
    g.drawText(label, bounds.reduced(4.0f, 0.0f), juce::Justification::centredLeft, false);
}

void drawCorrelationMeter(juce::Graphics &g, juce::Rectangle<float> bounds, float correlation,
                          int palette)
{
    const auto corr = std::clamp(correlation, -1.0f, 1.0f);
    const auto width = std::clamp(1.0f - std::abs(corr), 0.0f, 1.0f);
    g.setColour(juce::Colour(0xff07101c).withAlpha(0.82f));
    g.fillRect(bounds);
    g.setColour(juce::Colour(0xff18304d).withAlpha(0.78f));
    g.drawRect(bounds, 1.0f);

    const auto centreX = bounds.getCentreX();
    const auto markerX = juce::jmap(corr, -1.0f, 1.0f, bounds.getX() + 2.0f, bounds.getRight() - 2.0f);
    g.setColour(phosphorFor(0.24f + width * 0.44f, palette).withAlpha(0.46f));
    g.fillRect(bounds.reduced(1.0f).withWidth(bounds.getWidth() * width).withCentre({centreX, bounds.getCentreY()}));
    g.setColour(juce::Colour(0xffd7faff).withAlpha(0.30f));
    g.drawVerticalLine(static_cast<int>(centreX), bounds.getY() + 1.0f, bounds.getBottom() - 1.0f);
    g.setColour(corr < -0.10f ? juce::Colour(0xffff4a3d).withAlpha(0.85f)
                              : phosphorFor(0.70f, palette).withAlpha(0.90f));
    g.fillRect(juce::Rectangle<float>(markerX - 1.5f, bounds.getY() + 1.0f, 3.0f,
                                      bounds.getHeight() - 2.0f));

    g.setColour(juce::Colour(0xffd7faff).withAlpha(0.68f));
    g.setFont(juce::FontOptions(8.0f, juce::Font::plain));
    g.drawText("C", bounds.reduced(4.0f, 0.0f), juce::Justification::centredLeft, false);
}

void drawTerminalTexture(juce::Graphics &g, juce::Rectangle<float> scope, int frame)
{
    const auto shimmer = std::sin(static_cast<float>(frame) * 0.11f) * 0.5f + 0.5f;
    g.setColour(juce::Colour(0xff000000).withAlpha(0.10f + shimmer * 0.025f));
    for (auto y = scope.getY() + 3.0f; y < scope.getBottom(); y += 4.0f)
        g.fillRect(juce::Rectangle<float>(scope.getX() + 1.0f, y, scope.getWidth() - 2.0f, 1.0f));

    g.setColour(juce::Colour(0xff5efcff).withAlpha(0.025f));
    for (auto x = scope.getX() + 5.0f; x < scope.getRight(); x += 16.0f)
        g.fillRect(juce::Rectangle<float>(x, scope.getY() + 1.0f, 1.0f, scope.getHeight() - 2.0f));

    g.setColour(juce::Colour(0xff000000).withAlpha(0.20f));
    g.drawRoundedRectangle(scope.reduced(3.0f), 4.0f, 6.0f);
}

char glyphFor(float intensity)
{
    const auto glyphIndex = std::clamp(static_cast<int>(intensity * 9.0f), 0, 9);
    return glyphRamp[glyphIndex];
}

void writeCell(AsciiscopeVisualFrame &frameData, int x, int y, float intensity, int palette)
{
    if (intensity <= 0.05f)
        return;

    auto &cell = frameData.cell(x, y);
    if (intensity <= cell.intensity)
        return;

    cell.glyph = glyphFor(intensity);
    cell.intensity = intensity;
    cell.palette = palette;
}

void addTraceGlyph(AsciiscopeVisualFrame &frameData, float x, float y, char glyph,
                   float intensity, int palette)
{
    frameData.traceGlyphs.push_back({
        glyph,
        std::clamp(x, 0.0f, 1.0f),
        std::clamp(y, 0.0f, 1.0f),
        std::clamp(intensity, 0.0f, 1.0f),
        palette,
    });
}

struct TracePoint
{
    float x{0.5f};
    float y{0.5f};
};

TracePoint circlePoint(float phase, float radiusX, float radiusY)
{
    return {
        0.5f + std::cos(phase) * radiusX,
        0.5f + std::sin(phase) * radiusY,
    };
}

TracePoint lerpPoint(TracePoint a, TracePoint b, float amount)
{
    return {
        a.x + (b.x - a.x) * amount,
        a.y + (b.y - a.y) * amount,
    };
}

TracePoint catmullRomPoint(TracePoint p0, TracePoint p1, TracePoint p2, TracePoint p3,
                           float amount)
{
    const auto t2 = amount * amount;
    const auto t3 = t2 * amount;
    return {
        0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * amount +
                (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
        0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * amount +
                (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3),
    };
}

void addInterpolatedTrace(AsciiscopeVisualFrame &frameData, const std::vector<TracePoint> &points,
                          char glyph, float intensity, int palette, int cols, int rows,
                          int interpolationMode)
{
    if (points.empty())
        return;

    if (points.size() == 1 || interpolationMode <= 0)
    {
        for (const auto &point : points)
            addTraceGlyph(frameData, point.x, point.y, glyph, intensity, palette);
        return;
    }

    for (std::size_t i = 1; i < points.size(); ++i)
    {
        const auto p0 = points[i > 1 ? i - 2 : i - 1];
        const auto p1 = points[i - 1];
        const auto p2 = points[i];
        const auto p3 = points[std::min(i + 1, points.size() - 1)];
        const auto cellDistance =
            std::max(std::abs(p2.x - p1.x) * static_cast<float>(cols),
                     std::abs(p2.y - p1.y) * static_cast<float>(rows));
        const auto steps = std::max(1, static_cast<int>(std::ceil(cellDistance * 1.20f)));

        for (int step = i == 1 ? 0 : 1; step <= steps; ++step)
        {
            const auto amount = static_cast<float>(step) / static_cast<float>(steps);
            const auto point = interpolationMode == 1
                                   ? lerpPoint(p1, p2, amount)
                                   : catmullRomPoint(p0, p1, p2, p3, amount);
            addTraceGlyph(frameData, point.x, point.y, glyph, intensity, palette);
        }
    }
}

void addCircleTrace(AsciiscopeVisualFrame &frameData, int frame, float frequencyHz,
                    int palette, float visualAspect, int cols, int rows,
                    int interpolationMode)
{
    frameData.circleDiagnostic = true;

    const auto aspect = std::max(0.05f, visualAspect);
    auto radiusX = 0.42f;
    auto radiusY = 0.42f;
    if (aspect < 1.0f)
        radiusX *= aspect;
    else
        radiusY /= aspect;

    const auto t = static_cast<float>(frame) * frequencyHz * 0.05235988f;
    const auto phaseStep = frequencyHz * 0.05235988f;
    const auto p0 = circlePoint(t - phaseStep * 2.0f, radiusX, radiusY);
    const auto p1 = circlePoint(t - phaseStep, radiusX, radiusY);
    const auto p2 = circlePoint(t, radiusX, radiusY);
    const auto p3 = circlePoint(t + phaseStep, radiusX, radiusY);
    auto steps = 1;
    if (interpolationMode > 0)
    {
        const auto cellDistance =
            std::max(std::abs(p2.x - p1.x) * static_cast<float>(cols),
                     std::abs(p2.y - p1.y) * static_cast<float>(rows));
        steps = std::max(1, static_cast<int>(std::ceil(cellDistance * 1.35f)));
    }

    frameData.traceGlyphs.reserve(static_cast<std::size_t>(steps + 3));

    for (int i = 0; i <= steps; ++i)
    {
        const auto amount = static_cast<float>(i) / static_cast<float>(steps);
        auto point = p2;
        if (interpolationMode == 1)
            point = lerpPoint(p1, p2, amount);
        else if (interpolationMode >= 2)
            point = catmullRomPoint(p0, p1, p2, p3, amount);

        const auto intensity = 0.58f + amount * 0.34f;
        addTraceGlyph(frameData, point.x, point.y, i == steps ? '@' : '+', intensity, palette);
    }

    for (int i = 0; i < 3; ++i)
    {
        const auto phase = t - static_cast<float>(i) * 0.055f;
        const auto intensity = i == 0 ? 0.98f : 0.74f - static_cast<float>(i) * 0.16f;
        addTraceGlyph(frameData, 0.5f + std::cos(phase) * radiusX,
                      0.5f + std::sin(phase) * radiusY, i == 0 ? '@' : '+',
                      intensity, palette);
    }
}

void addStereoPhaseSparks(AsciiscopeVisualFrame &frameData, const AsciiscopeAudioSnapshot &snapshot,
                          int palette, float traceGain, float correlation, float transient)
{
    if (snapshot.sampleCount == 0)
        return;

    const auto snapshotEnergy = (snapshot.leftRms + snapshot.rightRms) * 0.5f;
    if (snapshotEnergy < 0.0025f && transient < 0.01f)
        return;

    const auto stride = std::max(1U, snapshot.sampleCount / 28U);
    const auto width = std::clamp(1.0f - std::abs(correlation), 0.0f, 1.0f);
    for (uint32_t i = 0; i < snapshot.sampleCount; i += stride)
    {
        const auto left = std::clamp(snapshot.left[i] * traceGain, -1.0f, 1.0f);
        const auto right = std::clamp(snapshot.right[i] * traceGain, -1.0f, 1.0f);
        const auto amp = std::clamp((std::abs(left) + std::abs(right)) * 0.55f +
                                        transient * 0.20f,
                                    0.0f, 1.0f);
        if (amp < 0.018f)
            continue;

        const auto glyph = amp > 0.72f ? '@' : (amp > 0.40f ? '*' : '+');
        const auto intensity = std::clamp(0.12f + amp * 0.68f + width * 0.14f, 0.0f, 1.0f);
        addTraceGlyph(frameData, 0.5f + left * (0.20f + width * 0.17f),
                      0.5f - right * (0.20f + width * 0.17f), glyph, intensity, palette);
    }
}
} // namespace

AsciiscopeVisualComponent::AsciiscopeVisualComponent()
{
    setBufferedToImage(true);
    setInterceptsMouseClicks(false, false);
}

void AsciiscopeVisualComponent::resized() {}

void AsciiscopeVisualComponent::tick()
{
    ++frame;
    const auto snapshotAge = hasSnapshot ? std::max(0, frame - latestSnapshotFrame) : 0;
    const auto isStale = hasSnapshot && snapshotAge > 8;
    const auto leftTarget = isStale ? 0.0f : leftLevel;
    const auto rightTarget = isStale ? 0.0f : rightLevel;
    const auto rmsTarget = hasSnapshot && !isStale ? (snapshot.leftRms + snapshot.rightRms) * 0.5f : 0.0f;
    const auto transientTarget = hasSnapshot && !isStale ? snapshot.transientAmount : 0.0f;
    const auto correlationTarget = hasSnapshot && !isStale ? snapshot.stereoCorrelation : 0.0f;
    const auto leftAlpha = leftTarget > displayLeftLevel ? 0.42f : 0.10f;
    const auto rightAlpha = rightTarget > displayRightLevel ? 0.42f : 0.10f;
    const auto transientAlpha = transientTarget > displayTransient ? 0.36f : 0.08f;
    displayLeftLevel += (leftTarget - displayLeftLevel) * leftAlpha;
    displayRightLevel += (rightTarget - displayRightLevel) * rightAlpha;
    displayLeftPeakHold = std::max(displayLeftLevel, displayLeftPeakHold - 0.010f);
    displayRightPeakHold = std::max(displayRightLevel, displayRightPeakHold - 0.010f);
    displayRms += (rmsTarget - displayRms) * 0.18f;
    displayCorrelation += (correlationTarget - displayCorrelation) * 0.16f;
    displayTransient += (transientTarget - displayTransient) * transientAlpha;
    repaint();
}

void AsciiscopeVisualComponent::setLevels(float left, float right)
{
    leftLevel = std::clamp(left, 0.0f, 1.0f);
    rightLevel = std::clamp(right, 0.0f, 1.0f);
}

void AsciiscopeVisualComponent::setSnapshot(const AsciiscopeAudioSnapshot &s)
{
    snapshot = s;
    hasSnapshot = snapshot.sampleCount > 0;
    leftLevel = std::clamp(snapshot.leftPeak, 0.0f, 1.0f);
    rightLevel = std::clamp(snapshot.rightPeak, 0.0f, 1.0f);
    latestSnapshotFrame = frame;

    for (uint32_t i = 0; i < snapshot.sampleCount; ++i)
    {
        const auto left = std::clamp(snapshot.left[i], -1.0f, 1.0f);
        const auto right = std::clamp(snapshot.right[i], -1.0f, 1.0f);
        leftHistory[historyWrite] = left;
        rightHistory[historyWrite] = right;
        monoHistory[historyWrite] = std::clamp((left + right) * 0.5f, -1.0f, 1.0f);
        historyWrite = (historyWrite + 1U) % historySize;
        historyCount = std::min(historyCount + 1U, historySize);
    }
}

void AsciiscopeVisualComponent::setVisualOptions(int mode, int selectedPalette, float gain,
                                                 float circleFrequency, int interpolationMode)
{
    scopeMode = std::clamp(mode, 0, 2);
    palette = std::clamp(selectedPalette, 0, 2);
    traceGain = std::clamp(0.25f + gain * 2.75f, 0.25f, 3.0f);
    circleFrequencyHz = std::clamp(0.05f + circleFrequency * 3.95f, 0.05f, 4.0f);
    traceInterpolationMode = std::clamp(interpolationMode, 0, 2);
}

void AsciiscopeVisualComponent::setCircleDiagnostic(bool active)
{
    circleDiagnostic = active;
}

AsciiscopeVisualFrame AsciiscopeVisualComponent::buildVisualFrame(int cols, int rows,
                                                                  float visualAspect) const
{
    AsciiscopeVisualFrame frameData;
    frameData.reset(cols, rows);

    frameData.title = juce::String("ASCIISCOPE CLAP // ") +
                      (circleDiagnostic ? "circle diagnostic" : modeName(scopeMode)) +
                      (hasSnapshot ? " snapshot feed" : " demo feed");

    frameData.readout = juce::String("mode ") + juce::String(scopeMode) + " " +
                        modeName(scopeMode) + " // palette " + juce::String(palette) + " " +
                        paletteName(palette) + " // interp " +
                        traceInterpolationName(traceInterpolationMode) + " // gain " +
                        juce::String(traceGain, 2) +
                        (circleDiagnostic
                             ? juce::String(" // circle ") + juce::String(circleFrequencyHz, 2) +
                                   "hz"
                             : juce::String()) +
                        " // L " + juce::String(displayLeftLevel, 2) + " R " +
                        juce::String(displayRightLevel, 2) + " // juce glyph";

    if (hasSnapshot)
    {
        const auto age = std::max(0, frame - latestSnapshotFrame);
        frameData.feedIsStale = age > 8;
        const auto feedState = frameData.feedIsStale ? "stale" : "live";
        const auto inputState = displayRms < 0.003f && displayTransient < 0.01f ? "quiet" : "signal";
        frameData.feed = juce::String("block ") + juce::String(snapshot.sampleCount) +
                         " // frame " + juce::String(static_cast<double>(snapshot.frameIndex), 0) +
                         " // sample " +
                         juce::String(static_cast<double>(snapshot.blockStartSample), 0) +
                         " @ " + juce::String(snapshot.sampleRate * 0.001f, 1) + "k" +
                         " // " + feedState + " age " + juce::String(age) +
                         " // " + inputState +
                         " // rms " + juce::String(displayRms, 3) +
                         " // corr " + juce::String(displayCorrelation, 2) +
                         " // crest " + juce::String(displayTransient, 2);
    }

    if (circleDiagnostic)
    {
        addCircleTrace(frameData, frame, circleFrequencyHz, palette, visualAspect, cols, rows,
                       traceInterpolationMode);
        return frameData;
    }

    const auto energy =
        std::clamp(0.18f + displayLeftLevel * 0.62f + displayRightLevel * 0.42f, 0.0f, 1.0f);
    const auto width = hasSnapshot ? std::clamp(1.0f - std::abs(displayCorrelation), 0.0f, 1.0f)
                                   : 0.0f;
    const auto transient = hasSnapshot ? displayTransient : 0.0f;
    frameData.traceGlyphs.reserve(static_cast<std::size_t>(cols * (scopeMode == 1 ? 2 : 1) + 36));

    if (hasSnapshot)
        addStereoPhaseSparks(frameData, snapshot, palette, traceGain, displayCorrelation, transient);

    std::vector<TracePoint> wavePoints;
    std::vector<TracePoint> mirrorTopPoints;
    std::vector<TracePoint> mirrorBottomPoints;
    std::vector<TracePoint> spectralPoints;
    wavePoints.reserve(static_cast<std::size_t>(cols));
    mirrorTopPoints.reserve(static_cast<std::size_t>(cols));
    mirrorBottomPoints.reserve(static_cast<std::size_t>(cols));
    spectralPoints.reserve(static_cast<std::size_t>(cols));

    for (int x = 0; x < cols; ++x)
    {
        const auto phase =
            (static_cast<float>(x) / static_cast<float>(cols)) * juce::MathConstants<float>::twoPi;
        const auto t = static_cast<float>(frame) * 0.065f;
        auto sample = std::sin(phase * 2.0f + t) * (0.23f + displayLeftLevel * 0.25f);
        auto stereoSpread = std::sin(phase * 3.0f - t * 0.7f) * displayRightLevel * 0.12f;
        auto mono = sample;
        if (hasSnapshot)
        {
            const auto historyPosition =
                static_cast<float>(x) / static_cast<float>(std::max(1, cols - 1));
            const auto left = readHistory(leftHistory, historyWrite, historyCount,
                                          historyPosition, traceInterpolationMode);
            const auto right = readHistory(rightHistory, historyWrite, historyCount,
                                           historyPosition, traceInterpolationMode);
            mono = readHistory(monoHistory, historyWrite, historyCount, historyPosition,
                               traceInterpolationMode);
            sample = std::clamp((left + right) * 0.27f * traceGain, -0.48f, 0.48f);
            stereoSpread = std::clamp((left - right) * (0.14f + width * 0.12f) * traceGain,
                                      -0.28f, 0.28f);
        }
        else
        {
            sample = std::clamp(sample * traceGain, -0.48f, 0.48f);
        }

        if (scopeMode == 2)
        {
            const auto folded = std::abs(mono * traceGain);
            const auto modulated =
                std::abs(std::sin(phase * 4.0f + t * 0.9f + folded * 7.0f)) * 0.22f;
            const auto bin = std::clamp(folded * 0.95f + displayRms * 1.45f + modulated +
                                            transient * 0.22f,
                                        0.0f, 1.0f);
            const auto xPos = static_cast<float>(x) / static_cast<float>(std::max(1, cols - 1));
            spectralPoints.push_back({xPos, 1.0f - bin});

            for (int y = 0; y < rows; ++y)
            {
                const auto row = static_cast<float>(y) / static_cast<float>(std::max(1, rows - 1));
                const auto height = 1.0f - row;
                const auto shimmer =
                    std::sin(phase * 17.0f - t * 5.0f + static_cast<float>(y) * 0.61f) * 0.5f +
                    0.5f;
                const auto intensity =
                    std::clamp((bin - height) * 2.9f + shimmer * (0.12f + width * 0.14f),
                               0.0f, 1.0f);
                writeCell(frameData, x, y, intensity, palette);
            }

            continue;
        }

        const auto waveA = sample + stereoSpread;
        const auto waveB =
            std::sin(phase * (scopeMode == 2 ? 9.0f : 5.0f) - t * 1.7f) *
            (0.10f + displayRightLevel * 0.16f);
        const auto centre = 0.5f + waveA + waveB;
        const auto mirrorA = 0.5f + std::abs(waveA) * 0.78f + waveB * 0.22f;
        const auto mirrorB = 0.5f - std::abs(waveA) * 0.78f - waveB * 0.22f;
        const auto glow = std::clamp(0.10f + energy * 0.55f + transient * 0.18f, 0.0f, 0.82f);
        const auto xPos = static_cast<float>(x) / static_cast<float>(std::max(1, cols - 1));
        if (scopeMode == 1)
        {
            mirrorTopPoints.push_back({xPos, mirrorA});
            mirrorBottomPoints.push_back({xPos, mirrorB});
        }
        else
        {
            wavePoints.push_back({xPos, centre});
        }

        for (int y = 0; y < rows; ++y)
        {
            const auto row = static_cast<float>(y) / static_cast<float>(std::max(1, rows - 1));
            const auto distance = scopeMode == 1
                                      ? std::min(std::abs(row - mirrorA), std::abs(row - mirrorB))
                                      : std::abs(row - centre);
            const auto pulse =
                std::sin(phase * 9.0f + t * 3.0f + static_cast<float>(y) * 0.37f) * 0.5f + 0.5f;
            const auto widthSparkle =
                std::sin(phase * 13.0f - t * 4.0f + static_cast<float>(y) * 0.51f) * 0.5f +
                0.5f;
            const auto intensity =
                std::clamp((glow - distance) * 2.6f + pulse * 0.16f +
                               widthSparkle * width * 0.18f + pulse * transient * 0.14f,
                           0.0f, 1.0f);
            writeCell(frameData, x, y, intensity, palette);
        }
    }

    if (scopeMode == 2)
    {
        addInterpolatedTrace(frameData, spectralPoints, '*', 0.82f + energy * 0.14f, palette,
                             cols, rows, traceInterpolationMode);
    }
    else if (scopeMode == 1)
    {
        addInterpolatedTrace(frameData, mirrorTopPoints, '+', 0.78f + energy * 0.18f, palette,
                             cols, rows, traceInterpolationMode);
        addInterpolatedTrace(frameData, mirrorBottomPoints, '+', 0.78f + energy * 0.18f, palette,
                             cols, rows, traceInterpolationMode);
    }
    else
    {
        addInterpolatedTrace(frameData, wavePoints, '@', 0.82f + energy * 0.14f, palette,
                             cols, rows, traceInterpolationMode);
    }

    return frameData;
}

void AsciiscopeVisualComponent::applyPhosphorMemory(AsciiscopeVisualFrame &frameData)
{
    const auto currentTraceMode = frameData.circleDiagnostic ? 3 : scopeMode;
    if (currentTraceMode != traceMemoryMode)
    {
        phosphorTraceGlyphs.clear();
        traceMemoryMode = currentTraceMode;
    }

    const auto traceDecay = frameData.circleDiagnostic ? 0.91f : 0.72f;
    for (auto &traceGlyph : phosphorTraceGlyphs)
    {
        traceGlyph.intensity *= traceDecay;
        traceGlyph.glyph = glyphFor(traceGlyph.intensity);
    }

    phosphorTraceGlyphs.erase(std::remove_if(phosphorTraceGlyphs.begin(),
                                             phosphorTraceGlyphs.end(),
                                             [](const auto &traceGlyph)
                                             { return traceGlyph.intensity <= 0.04f; }),
                              phosphorTraceGlyphs.end());

    phosphorTraceGlyphs.insert(phosphorTraceGlyphs.end(),
                               frameData.traceGlyphs.begin(), frameData.traceGlyphs.end());

    const auto maxTraceGlyphs = frameData.circleDiagnostic ? 220U : 900U;
    if (phosphorTraceGlyphs.size() > maxTraceGlyphs)
    {
        phosphorTraceGlyphs.erase(phosphorTraceGlyphs.begin(),
                                  phosphorTraceGlyphs.end() - maxTraceGlyphs);
    }
    frameData.traceGlyphs = phosphorTraceGlyphs;

    if (frameData.circleDiagnostic)
    {
        phosphorFrame.reset(0, 0);
        return;
    }

    if (phosphorFrame.cols != frameData.cols || phosphorFrame.rows != frameData.rows)
        phosphorFrame.reset(frameData.cols, frameData.rows);

    constexpr auto decay = 0.84f;
    for (int y = 0; y < frameData.rows; ++y)
    {
        for (int x = 0; x < frameData.cols; ++x)
        {
            const auto index = static_cast<std::size_t>(y * frameData.cols + x);
            auto &held = phosphorFrame.cells[index];
            const auto &incoming = frameData.cells[index];

            held.intensity *= decay;
            if (held.intensity <= 0.035f)
            {
                held = {};
            }

            if (incoming.intensity > held.intensity)
            {
                held = incoming;
            }
            else if (held.intensity > 0.05f)
            {
                held.glyph = glyphFor(held.intensity);
            }
        }
    }

    frameData.cells = phosphorFrame.cells;
}

void AsciiscopeVisualComponent::drawVisualFrame(juce::Graphics &g, juce::Rectangle<float> scope,
                                                const AsciiscopeVisualFrame &frameData) const
{
    if (frameData.circleDiagnostic)
    {
        const auto square = scope.withSizeKeepingCentre(std::min(scope.getWidth(), scope.getHeight()),
                                                        std::min(scope.getWidth(), scope.getHeight()))
                                .reduced(12.0f);
        const auto centre = square.getCentre();
        g.setColour(juce::Colour(0xff18304d).withAlpha(0.55f));
        g.drawRect(square, 1.0f);
        g.drawHorizontalLine(static_cast<int>(centre.y), square.getX(), square.getRight());
        g.drawVerticalLine(static_cast<int>(centre.x), square.getY(), square.getBottom());
    }

    const auto cols = std::max(1, frameData.cols);
    const auto rows = std::max(1, frameData.rows);
    const auto cellW = scope.getWidth() / static_cast<float>(cols);
    const auto cellH = scope.getHeight() / static_cast<float>(rows);

    g.setFont(juce::FontOptions(std::max(8.0f, cellH + 1.5f), juce::Font::plain));
    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < cols; ++x)
        {
            const auto &cell = frameData.cells[static_cast<std::size_t>(y * cols + x)];
            if (cell.intensity <= 0.05f)
                continue;

            const char text[] = {cell.glyph, 0};
            g.setColour(phosphorFor(cell.intensity, cell.palette));
            g.drawText(text,
                       juce::Rectangle<float>(scope.getX() + static_cast<float>(x) * cellW,
                                              scope.getY() + static_cast<float>(y) * cellH,
                                              cellW + 1.0f, cellH + 1.0f),
                       juce::Justification::centred, false);
        }
    }

    g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
    for (const auto &traceGlyph : frameData.traceGlyphs)
    {
        const char text[] = {traceGlyph.glyph, 0};
        const auto x = scope.getX() + std::clamp(traceGlyph.x, 0.0f, 1.0f) * scope.getWidth();
        const auto y = scope.getY() + std::clamp(traceGlyph.y, 0.0f, 1.0f) * scope.getHeight();
        g.setColour(phosphorFor(traceGlyph.intensity, traceGlyph.palette)
                        .withAlpha(traceGlyph.intensity > 0.8f ? 0.98f : 0.78f));
        g.drawText(text, juce::Rectangle<float>(x - 6.0f, y - 6.0f, 12.0f, 12.0f),
                   juce::Justification::centred, false);
    }
}

void AsciiscopeVisualComponent::drawReadouts(juce::Graphics &g, juce::Rectangle<float> scope,
                                             const AsciiscopeVisualFrame &frameData) const
{
    const auto inner = scope.reduced(7.0f);
    const auto topShelf = inner.withHeight(17.0f);
    const auto bottomShelf = inner.withTop(inner.getBottom() - 17.0f);
    const auto feedShelf = inner.withTrimmedLeft(std::max(120.0f, inner.getWidth() * 0.28f))
                           .withHeight(17.0f)
                           .translated(0.0f, 22.0f);

    g.setColour(juce::Colour(0xff03040a).withAlpha(0.55f));
    g.fillRoundedRectangle(topShelf, 3.0f);
    g.fillRoundedRectangle(bottomShelf, 3.0f);
    if (frameData.feed.isNotEmpty())
        g.fillRoundedRectangle(feedShelf, 3.0f);

    g.setColour(juce::Colour(0xff5efcff).withAlpha(0.85f));
    g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    g.drawText(frameData.title, topShelf.reduced(4.0f, 0.0f), juce::Justification::centredLeft, false);

    g.setColour(phosphorFor(0.74f, palette).withAlpha(0.92f));
    g.drawText(frameData.readout, bottomShelf.reduced(4.0f, 0.0f), juce::Justification::centredRight,
               false);

    auto meterArea = scope.reduced(7.0f).withHeight(19.0f).withWidth(118.0f);
    drawMeter(g, meterArea.removeFromTop(8.0f), displayLeftLevel, displayLeftPeakHold,
              phosphorFor(0.62f, palette), "L");
    meterArea.removeFromTop(3.0f);
    drawMeter(g, meterArea.removeFromTop(8.0f), displayRightLevel, displayRightPeakHold,
              phosphorFor(0.48f, palette), "R");
    drawCorrelationMeter(g, scope.reduced(7.0f).translated(0.0f, 23.0f).withHeight(9.0f).withWidth(118.0f),
                         displayCorrelation, palette);

    if (frameData.feed.isNotEmpty())
    {
        g.setColour((frameData.feedIsStale ? juce::Colour(0xffff4a3d) : phosphorFor(0.50f, palette))
                        .withAlpha(0.72f));
        g.drawText(frameData.feed, feedShelf.reduced(4.0f, 0.0f), juce::Justification::centredRight,
                   false);
    }
}

void AsciiscopeVisualComponent::paint(juce::Graphics &g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll(juce::Colour(0xff03040a));

    auto scope = bounds.reduced(8.0f);
    const auto snapshotAge = hasSnapshot ? std::max(0, frame - latestSnapshotFrame) : 0;
    const auto isStale = hasSnapshot && snapshotAge > 8;
    const auto borderEnergy = std::clamp(displayRms * 1.4f + displayTransient * 0.55f, 0.0f, 1.0f);
    g.setColour(juce::Colour(0xff13233a));
    g.drawRoundedRectangle(scope, 6.0f, 1.0f);
    g.setColour((isStale ? juce::Colour(0xffff4a3d) : phosphorFor(0.48f + borderEnergy * 0.46f, palette))
                    .withAlpha(0.18f + borderEnergy * 0.60f));
    g.drawRoundedRectangle(scope.reduced(1.0f), 5.0f, 1.0f + borderEnergy * 2.0f);

    g.setColour(juce::Colour(0xff18304d).withAlpha(0.55f));
    for (int i = 1; i < 4; ++i)
    {
        const auto x = scope.getX() + scope.getWidth() * static_cast<float>(i) / 4.0f;
        g.drawVerticalLine(static_cast<int>(x), scope.getY(), scope.getBottom());
    }
    for (int i = 1; i < 3; ++i)
    {
        const auto y = scope.getY() + scope.getHeight() * static_cast<float>(i) / 3.0f;
        g.drawHorizontalLine(static_cast<int>(y), scope.getX(), scope.getRight());
    }

    const auto rows = std::max(8, static_cast<int>(scope.getHeight() / 11.0f));
    const auto cols = std::max(24, static_cast<int>(scope.getWidth() / 8.0f));
    const auto visualAspect = scope.getHeight() / std::max(1.0f, scope.getWidth());
    auto visualFrame = buildVisualFrame(cols, rows, visualAspect);
    applyPhosphorMemory(visualFrame);
    drawVisualFrame(g, scope, visualFrame);
    drawTerminalTexture(g, scope, frame);
    drawReadouts(g, scope, visualFrame);
}
} // namespace baconpaul::sidequest_ns::ui
