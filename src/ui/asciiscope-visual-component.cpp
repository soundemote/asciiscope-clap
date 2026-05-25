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

void AsciiscopeVisualComponent::setVisualOptions(int mode, int selectedPalette, float gain)
{
    scopeMode = std::clamp(mode, 0, 2);
    palette = std::clamp(selectedPalette, 0, 2);
    traceGain = std::clamp(0.25f + gain * 2.75f, 0.25f, 3.0f);
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

    const auto energy = std::clamp(0.18f + leftLevel * 0.62f + rightLevel * 0.42f, 0.0f, 1.0f);
    const auto width = hasSnapshot ? std::clamp(1.0f - std::abs(snapshot.stereoCorrelation), 0.0f, 1.0f) : 0.0f;
    const auto transient = hasSnapshot ? snapshot.transientAmount : 0.0f;
    const auto rows = std::max(8, static_cast<int>(scope.getHeight() / 11.0f));
    const auto cols = std::max(24, static_cast<int>(scope.getWidth() / 8.0f));
    const auto cellW = scope.getWidth() / static_cast<float>(cols);
    const auto cellH = scope.getHeight() / static_cast<float>(rows);

    g.setFont(juce::FontOptions(std::max(8.0f, cellH + 1.5f), juce::Font::plain));
    for (int x = 0; x < cols; ++x)
    {
        const auto phase = (static_cast<float>(x) / static_cast<float>(cols)) * juce::MathConstants<float>::twoPi;
        const auto t = static_cast<float>(frame) * 0.065f;
        auto sample = std::sin(phase * 2.0f + t) * (0.23f + leftLevel * 0.25f);
        auto stereoSpread = std::sin(phase * 3.0f - t * 0.7f) * rightLevel * 0.12f;
        if (hasSnapshot)
        {
            const auto historyPosition = static_cast<float>(x) / static_cast<float>(std::max(1, cols - 1));
            const auto left = readHistory(leftHistory, historyWrite, historyCount, historyPosition);
            const auto right = readHistory(rightHistory, historyWrite, historyCount, historyPosition);
            sample = std::clamp((left + right) * 0.27f * traceGain, -0.48f, 0.48f);
            stereoSpread = std::clamp((left - right) * (0.14f + width * 0.12f) * traceGain, -0.28f, 0.28f);
        }
        else
        {
            sample = std::clamp(sample * traceGain, -0.48f, 0.48f);
        }

        if (scopeMode == 1)
            sample = std::abs(sample + stereoSpread) * 0.88f - 0.22f;
        else if (scopeMode == 2)
            sample = std::sin((sample + stereoSpread) * 5.0f + phase * 1.7f + t * 0.8f) *
                     (0.18f + energy * 0.32f);

        const auto waveA = sample + stereoSpread;
        const auto waveB = std::sin(phase * (scopeMode == 2 ? 9.0f : 5.0f) - t * 1.7f) *
                           (0.10f + rightLevel * 0.16f);
        const auto centre = 0.5f + waveA + waveB;
        const auto glow = std::clamp(0.10f + energy * 0.55f + transient * 0.18f, 0.0f, 0.82f);

        for (int y = 0; y < rows; ++y)
        {
            const auto row = static_cast<float>(y) / static_cast<float>(std::max(1, rows - 1));
            const auto distance = std::abs(row - centre);
            const auto pulse = std::sin(phase * 9.0f + t * 3.0f + static_cast<float>(y) * 0.37f) * 0.5f + 0.5f;
            const auto widthSparkle = std::sin(phase * 13.0f - t * 4.0f + static_cast<float>(y) * 0.51f) *
                                          0.5f +
                                      0.5f;
            const auto intensity = std::clamp((glow - distance) * 2.6f + pulse * 0.16f +
                                                  widthSparkle * width * 0.18f + pulse * transient * 0.14f,
                                              0.0f, 1.0f);
            if (intensity <= 0.05f)
                continue;

            const auto glyphIndex = std::clamp(static_cast<int>(intensity * 9.0f), 0, 9);
            const char text[] = {glyphRamp[glyphIndex], 0};
            g.setColour(phosphorFor(intensity, palette));
            g.drawText(text,
                       juce::Rectangle<float>(scope.getX() + static_cast<float>(x) * cellW,
                                              scope.getY() + static_cast<float>(y) * cellH,
                                              cellW + 1.0f, cellH + 1.0f),
                       juce::Justification::centred, false);
        }
    }

    g.setColour(juce::Colour(0xff5efcff).withAlpha(0.85f));
    g.setFont(juce::FontOptions(11.0f, juce::Font::plain));
    g.drawText(juce::String("ASCIISCOPE CLAP // ") + modeName(scopeMode) +
                   (hasSnapshot ? " snapshot feed" : " demo feed"),
               scope.reduced(7.0f), juce::Justification::topLeft, false);

    const auto readout = juce::String("mode ") + juce::String(scopeMode) + " " + modeName(scopeMode) +
                         " // palette " + juce::String(palette) + " " + paletteName(palette) +
                         " // gain " + juce::String(traceGain, 2) +
                         " // L " + juce::String(leftLevel, 2) + " R " + juce::String(rightLevel, 2);
    g.setColour(phosphorFor(0.74f, palette).withAlpha(0.92f));
    g.drawText(readout, scope.reduced(7.0f), juce::Justification::bottomRight, false);

    if (hasSnapshot)
    {
        const auto age = std::max(0, frame - latestSnapshotFrame);
        const auto feed = juce::String("block ") + juce::String(snapshot.sampleCount) +
                          " // frame " + juce::String(static_cast<double>(snapshot.frameIndex), 0) +
                          " // age " + juce::String(age) +
                          " // rms " + juce::String((snapshot.leftRms + snapshot.rightRms) * 0.5f, 3) +
                          " // corr " + juce::String(snapshot.stereoCorrelation, 2) +
                          " // crest " + juce::String(snapshot.transientAmount, 2);
        g.setColour(phosphorFor(0.50f, palette).withAlpha(0.72f));
        g.drawText(feed, scope.reduced(7.0f, 22.0f), juce::Justification::topRight, false);
    }
}
} // namespace baconpaul::sidequest_ns::ui
