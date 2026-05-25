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

#include <juce_gui_basics/juce_gui_basics.h>

#include "engine/asciiscope-audio-snapshot.h"

namespace baconpaul::sidequest_ns::ui
{
struct AsciiscopeVisualComponent : juce::Component
{
    AsciiscopeVisualComponent();

    void paint(juce::Graphics &g) override;
    void resized() override;

    void tick();
    void setLevels(float left, float right);
    void setSnapshot(const AsciiscopeAudioSnapshot &snapshot);
    void setVisualOptions(int mode, int palette, float gain);

    static constexpr uint32_t historySize{512};

    AsciiscopeAudioSnapshot snapshot;
    std::array<float, historySize> monoHistory{};
    std::array<float, historySize> leftHistory{};
    std::array<float, historySize> rightHistory{};
    uint32_t historyWrite{0};
    uint32_t historyCount{0};
    int scopeMode{0};
    int palette{0};
    float traceGain{1.0f};
    bool hasSnapshot{false};
    float leftLevel{0.0f};
    float rightLevel{0.0f};
    int frame{0};
};
} // namespace baconpaul::sidequest_ns::ui

#endif // BACONPAUL_SIDEQUEST_UI_ASCIISCOPE_VISUAL_COMPONENT_H
