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

#ifndef BACONPAUL_SIDEQUEST_UI_ASCIISCOPE_VISUAL_COMPONENT_H
#define BACONPAUL_SIDEQUEST_UI_ASCIISCOPE_VISUAL_COMPONENT_H

#include <array>
#include <cstddef>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "engine/asciiscope-audio-snapshot.h"

namespace baconpaul::sidequest_ns::ui
{
struct AsciiscopeGlyphCell
{
    char glyph{' '};
    float intensity{0.0f};
    int palette{0};
};

struct AsciiscopeTraceGlyph
{
    char glyph{'.'};
    float x{0.5f};
    float y{0.5f};
    float intensity{0.0f};
    int palette{0};
};

struct AsciiscopeVisualFrame
{
    void reset(int newCols, int newRows)
    {
        cols = newCols;
        rows = newRows;
        cells.assign(static_cast<std::size_t>(cols * rows), {});
        traceGlyphs.clear();
        title = {};
        readout = {};
        feed = {};
        feedIsStale = false;
        circleDiagnostic = false;
    }

    AsciiscopeGlyphCell &cell(int x, int y)
    {
        return cells[static_cast<std::size_t>(y * cols + x)];
    }

    int cols{0};
    int rows{0};
    std::vector<AsciiscopeGlyphCell> cells;
    std::vector<AsciiscopeTraceGlyph> traceGlyphs;
    juce::String title;
    juce::String readout;
    juce::String feed;
    bool feedIsStale{false};
    bool circleDiagnostic{false};
};

struct AsciiscopeVisualComponent : juce::Component
{
    AsciiscopeVisualComponent();

    void paint(juce::Graphics &g) override;
    void resized() override;

    void tick();
    void setLevels(float left, float right);
    void setSnapshot(const AsciiscopeAudioSnapshot &snapshot);
    void setVisualOptions(int mode, int palette, float gain, float circleFrequency);
    void setCircleDiagnostic(bool active);

    static constexpr uint32_t historySize{512};

    AsciiscopeVisualFrame buildVisualFrame(int cols, int rows, float visualAspect) const;
    void applyPhosphorMemory(AsciiscopeVisualFrame &frameData);
    void drawVisualFrame(juce::Graphics &g, juce::Rectangle<float> scope,
                         const AsciiscopeVisualFrame &frameData) const;
    void drawReadouts(juce::Graphics &g, juce::Rectangle<float> scope,
                      const AsciiscopeVisualFrame &frameData) const;

    AsciiscopeAudioSnapshot snapshot;
    std::array<float, historySize> monoHistory{};
    std::array<float, historySize> leftHistory{};
    std::array<float, historySize> rightHistory{};
    AsciiscopeVisualFrame phosphorFrame;
    std::vector<AsciiscopeTraceGlyph> phosphorTraceGlyphs;
    int traceMemoryMode{-1};
    uint32_t historyWrite{0};
    uint32_t historyCount{0};
    int latestSnapshotFrame{0};
    int scopeMode{0};
    int palette{0};
    float traceGain{1.0f};
    float circleFrequencyHz{1.0f};
    bool circleDiagnostic{false};
    bool hasSnapshot{false};
    float leftLevel{0.0f};
    float rightLevel{0.0f};
    float displayLeftLevel{0.0f};
    float displayRightLevel{0.0f};
    float displayRms{0.0f};
    float displayCorrelation{0.0f};
    float displayTransient{0.0f};
    int frame{0};
};
} // namespace baconpaul::sidequest_ns::ui

#endif // BACONPAUL_SIDEQUEST_UI_ASCIISCOPE_VISUAL_COMPONENT_H
