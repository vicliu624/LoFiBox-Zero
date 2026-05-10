// SPDX-License-Identifier: GPL-3.0-or-later

#include "groove/groove_controller.h"
#include "groove/groove_sequencer.h"
#include "groove/groove_transport.h"

#include <cassert>

int main()
{
    assert(lofibox::groove::secondsPerBeat(120) == 0.5);
    assert(lofibox::groove::secondsPerStep(120) == 0.125);
    assert(lofibox::groove::swingOffsetSeconds(120, 50, 1) > 0.0);
    assert(lofibox::groove::swingOffsetSeconds(120, 50, 2) == 0.0);

    auto project = lofibox::groove::makeDefaultGrooveProject("Timing");
    project.bpm = 120;
    project.swing = 25;
    project.patterns[0].tracks[0].steps[0].trigger = true;
    project.patterns[0].tracks[0].steps[4].trigger = true;
    project.patterns[0].tracks[1].soundSlot = 1;
    project.patterns[0].tracks[1].steps[4].trigger = true;
    project.patterns[1].tracks[0].steps[0].trigger = true;
    project.songChain.enabled = true;
    project.songChain.items.push_back({0, 2, false, 0, "A"});
    project.songChain.items.push_back({1, 1, false, 0, "B"});

    const auto events = lofibox::groove::collectPatternTriggers(project.patterns[0], 0, project.bpm, project.swing);
    assert(events.size() == 3);
    assert(events[0].stepIndex == 0);
    assert(events[1].startSeconds <= events[2].startSeconds);

    const auto chain_events = lofibox::groove::collectSongChainTriggers(project);
    assert(chain_events.size() == 7);
    assert(lofibox::groove::songChainDurationSeconds(project) == 6.0);

    lofibox::groove::GrooveController controller{project};
    lofibox::groove::PocketGrooveCommand select = lofibox::groove::makeGrooveCommand(lofibox::groove::PocketGrooveCommandType::SelectStep);
    select.stepIndex = 3;
    const auto select_events = controller.dispatch(select);
    (void)select_events;
    const auto toggle_events = controller.dispatch(lofibox::groove::makeGrooveCommand(lofibox::groove::PocketGrooveCommandType::ToggleStep));
    (void)toggle_events;
    auto lock = lofibox::groove::makeGrooveCommand(lofibox::groove::PocketGrooveCommandType::SetStepGain);
    lock.floatValue = 0.5f;
    const auto lock_events = controller.dispatch(lock);
    (void)lock_events;
    assert(controller.project().patterns[0].tracks[0].steps[3].trigger);
    assert(controller.project().patterns[0].tracks[0].steps[3].hasGainLock);

    return 0;
}
