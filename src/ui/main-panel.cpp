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

#include "main-panel.h"
#include "plugin-editor.h"
#include "patch-data-bindings.h"

namespace baconpaul::sidequest_ns::ui
{

MainPanel::MainPanel(PluginEditor &e)
    : sst::jucegui::components::NamedPanel("Main Panel"), editor(e)
{
    visual = std::make_unique<AsciiscopeVisualComponent>();
    addAndMakeVisible(*visual);

    circleButton.setClickingTogglesState(true);
    circleButton.onClick = [this]()
    {
        circleDiagnostic = circleButton.getToggleState();
        if (visual)
            visual->setCircleDiagnostic(circleDiagnostic);
    };
    addAndMakeVisible(circleButton);

    knobs.resize(e.patchCopy.params.size());
    knobAs.resize(e.patchCopy.params.size());
    for (int i = 0; i < e.patchCopy.params.size(); i++)
    {
        createComponent(editor, *this, *editor.patchCopy.params[i], knobs[i], knobAs[i]);
        addAndMakeVisible(*knobs[i]);
    }
}

void MainPanel::resized()
{
    auto b = getContentArea();
    auto w = b.getWidth();
    auto x = b.getX();
    auto visualHeight = std::max(110, b.getHeight() * 2 / 3);
    visual->setBounds(b.withHeight(visualHeight).reduced(2));

    auto y = b.getY() + visualHeight + 8;
    auto spw = 50;
    auto sph = 70;
    circleButton.setBounds(x, y, 80, 24);
    x += 88;

    for (int i = 0; i < editor.patchCopy.params.size(); i++)
    {
        knobs[i]->setBounds(x, y, spw - 5, sph - 5);
        x += spw;
        if (x + spw > w)
        {
            x = b.getX();
            y += sph;
        }
    }
}

void MainPanel::tickVisual()
{
    if (visual)
        visual->tick();
}

void MainPanel::setVisualLevels(float left, float right)
{
    if (visual)
        visual->setLevels(left, right);
}

void MainPanel::setVisualSnapshot(const AsciiscopeAudioSnapshot &snapshot)
{
    if (visual)
        visual->setSnapshot(snapshot);
}

void MainPanel::setVisualOptions(int mode, int palette, float gain, float circleFrequency)
{
    if (visual)
        visual->setVisualOptions(mode, palette, gain, circleFrequency);
}

} // namespace baconpaul::sidequest_ns::ui
