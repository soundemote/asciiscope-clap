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

float readHistory(const std::array<float, AsciiscopeVisualComponent::historySize> &history,
                  uint32_t write, uint32_t count, float position)
{
    if (count == 0)
        return 0.0f;

    const auto clampedPosition = std::clamp(position, 0.0f, 1.0f);
    const auto newestDistance = static_cast<uint32_t>((1.0f - clampedPosition) *
                                                      static_cast<float>(count - 1));
    const auto index = (write + AsciiscopeVisualComponent::historySize - 1U - newestDistance) %
                       AsciiscopeVisualComponent::historySize;
    return history[index];
}

void drawMeter(juce::Graphics &g, juce::Rectangle<float> bounds, float level,
               juce::Colour colour, const char *label)
{
    const auto value = std::clamp(level, 0.0f, 1.0f);
    g.setColour(juce::Colour(0xff07101c).withAlpha(0.82f));
    g.fillRect(bounds);
    g.setColour(juce::Colour(0xff18304d).withAlpha(0.78f));
    g.drawRect(bounds, 1.0f);

    auto fill = bounds.reduced(1.0f);
    fill.setWidth(fill.getWidth() * value);
    g.setColour(colour.withAlpha(0.82f));
    g.fillRect(fill);

    g.setColour(juce::Colour(0xffd7faff).withAlpha(0.74f));
    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));
    g.drawText(label, bounds.reduced(4.0f, 0.0f), juce::Justification::centredLeft, false);
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

void addCircleTrace(AsciiscopeVisualFrame &frameData, int frame, float frequencyHz,
                    int palette, float visualAspect)
{
    frameData.circleDiagnostic = true;
    frameData.traceGlyphs.reserve(3);

    const auto aspect = std::max(0.05f, visualAspect);
    auto radiusX = 0.42f;
    auto radiusY = 0.42f;
    if (aspect < 1.0f)
        radiusX *= aspect;
    else
        radiusY /= aspect;

    const auto t = static_cast<float>(frame) * frequencyHz * 0.05235988f;
    for (int i = 0; i < 3; ++i)
    {
        const auto phase = t - static_cast<float>(i) * 0.055f;
        const auto intensity = i == 0 ? 0.98f : 0.74f - static_cast<float>(i) * 0.16f;
        addTraceGlyph(frameData, 0.5f + std::cos(phase) * radiusX,
                      0.5f + std::sin(phase) * radiusY, i == 0 ? '@' : '+',
                      intensity, palette);
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
    const auto leftAlpha = leftLevel > displayLeftLevel ? 0.42f : 0.10f;
    const auto rightAlpha = rightLevel > displayRightLevel ? 0.42f : 0.10f;
    const auto rmsTarget = hasSnapshot ? (snapshot.leftRms + snapshot.rightRms) * 0.5f : 0.0f;
    const auto transientTarget = hasSnapshot ? snapshot.transientAmount : 0.0f;
    const auto correlationTarget = hasSnapshot ? snapshot.stereoCorrelation : 0.0f;
    const auto transientAlpha = transientTarget > displayTransient ? 0.36f : 0.08f;
    displayLeftLevel += (leftLevel - displayLeftLevel) * leftAlpha;
    displayRightLevel += (rightLevel - displayRightLevel) * rightAlpha;
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
                                                 float circleFrequency)
{
    scopeMode = std::clamp(mode, 0, 2);
    palette = std::clamp(selectedPalette, 0, 2);
    traceGain = std::clamp(0.25f + gain * 2.75f, 0.25f, 3.0f);
    circleFrequencyHz = std::clamp(0.05f + circleFrequency * 3.95f, 0.05f, 4.0f);
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
                        paletteName(palette) + " // gain " + juce::String(traceGain, 2) +
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
        frameData.feed = juce::String("block ") + juce::String(snapshot.sampleCount) +
                         " // frame " + juce::String(static_cast<double>(snapshot.frameIndex), 0) +
                         " // " + feedState + " age " + juce::String(age) +
                         " // rms " + juce::String(displayRms, 3) +
                         " // corr " + juce::String(displayCorrelation, 2) +
                         " // crest " + juce::String(displayTransient, 2);
    }

    if (circleDiagnostic)
    {
        addCircleTrace(frameData, frame, circleFrequencyHz, palette, visualAspect);
        return frameData;
    }

    const auto energy =
        std::clamp(0.18f + displayLeftLevel * 0.62f + displayRightLevel * 0.42f, 0.0f, 1.0f);
    const auto width = hasSnapshot ? std::clamp(1.0f - std::abs(displayCorrelation), 0.0f, 1.0f)
                                   : 0.0f;
    const auto transient = hasSnapshot ? displayTransient : 0.0f;
    frameData.traceGlyphs.reserve(static_cast<std::size_t>(cols * (scopeMode == 1 ? 2 : 1)));

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
            const auto left = readHistory(leftHistory, historyWrite, historyCount, historyPosition);
            const auto right = readHistory(rightHistory, historyWrite, historyCount, historyPosition);
            mono = readHistory(monoHistory, historyWrite, historyCount, historyPosition);
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
            addTraceGlyph(frameData, xPos, 1.0f - bin, bin > 0.72f ? '@' : '*',
                          0.70f + bin * 0.26f, palette);

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
            addTraceGlyph(frameData, xPos, mirrorA, '+', 0.78f + energy * 0.18f, palette);
            addTraceGlyph(frameData, xPos, mirrorB, '+', 0.78f + energy * 0.18f, palette);
        }
        else
        {
            addTraceGlyph(frameData, xPos, centre, '@', 0.82f + energy * 0.14f, palette);
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

    return frameData;
}

void AsciiscopeVisualComponent::applyPhosphorMemory(AsciiscopeVisualFrame &frameData)
{
    if (frameData.circleDiagnostic)
    {
        phosphorFrame.reset(0, 0);
        for (auto &traceGlyph : phosphorTraceGlyphs)
        {
            traceGlyph.intensity *= 0.91f;
            traceGlyph.glyph = glyphFor(traceGlyph.intensity);
        }

        phosphorTraceGlyphs.erase(std::remove_if(phosphorTraceGlyphs.begin(),
                                                 phosphorTraceGlyphs.end(),
                                                 [](const auto &traceGlyph)
                                                 { return traceGlyph.intensity <= 0.04f; }),
                                  phosphorTraceGlyphs.end());

        phosphorTraceGlyphs.insert(phosphorTraceGlyphs.end(),
                                   frameData.traceGlyphs.begin(), frameData.traceGlyphs.end());

        constexpr auto maxCircleTrailGlyphs = 220U;
        if (phosphorTraceGlyphs.size() > maxCircleTrailGlyphs)
        {
            phosphorTraceGlyphs.erase(phosphorTraceGlyphs.begin(),
                                      phosphorTraceGlyphs.end() - maxCircleTrailGlyphs);
        }

        frameData.traceGlyphs = phosphorTraceGlyphs;
        return;
    }

    phosphorTraceGlyphs.clear();

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
    g.setColour(juce::Colour(0xff5efcff).withAlpha(0.85f));
    g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    g.drawText(frameData.title, scope.reduced(7.0f), juce::Justification::topLeft, false);

    g.setColour(phosphorFor(0.74f, palette).withAlpha(0.92f));
    g.drawText(frameData.readout, scope.reduced(7.0f), juce::Justification::bottomRight, false);

    auto meterArea = scope.reduced(7.0f).withHeight(19.0f).withWidth(118.0f);
    drawMeter(g, meterArea.removeFromTop(8.0f), displayLeftLevel, phosphorFor(0.62f, palette), "L");
    meterArea.removeFromTop(3.0f);
    drawMeter(g, meterArea.removeFromTop(8.0f), displayRightLevel, phosphorFor(0.48f, palette), "R");

    if (frameData.feed.isNotEmpty())
    {
        g.setColour((frameData.feedIsStale ? juce::Colour(0xffff4a3d) : phosphorFor(0.50f, palette))
                        .withAlpha(0.72f));
        g.drawText(frameData.feed, scope.reduced(7.0f, 22.0f), juce::Justification::topRight, false);
    }
}

void AsciiscopeVisualComponent::paint(juce::Graphics &g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll(juce::Colour(0xff03040a));

    auto scope = bounds.reduced(8.0f);
    g.setColour(juce::Colour(0xff13233a));
    g.drawRoundedRectangle(scope, 6.0f, 1.0f);

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
    drawReadouts(g, scope, visualFrame);
}
} // namespace baconpaul::sidequest_ns::ui
