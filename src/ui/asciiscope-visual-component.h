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

#include <juce_gui_basics/juce_gui_basics.h>

namespace baconpaul::sidequest_ns::ui
{
struct AsciiscopeVisualComponent : juce::Component
{
    AsciiscopeVisualComponent();

    void paint(juce::Graphics &g) override;
    void resized() override;

    void tick();
    void setLevels(float left, float right);

    float leftLevel{0.0f};
    float rightLevel{0.0f};
    int frame{0};
};
} // namespace baconpaul::sidequest_ns::ui

#endif // BACONPAUL_SIDEQUEST_UI_ASCIISCOPE_VISUAL_COMPONENT_H
