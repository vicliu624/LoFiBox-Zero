// SPDX-License-Identifier: GPL-3.0-or-later

#include "groove/groove_project.h"
#include "groove/groove_project_repository.h"

#include <cassert>
#include <filesystem>
#include <string>

int main()
{
    auto project = lofibox::groove::makeDefaultGrooveProject("Late Beat");
    project.id = "groove-20260511-001";
    project.bpm = 92;
    project.swing = 12;
    project.sounds[3].type = lofibox::groove::GrooveSoundType::CapturedFromTrack;
    project.sounds[3].name = "CHOP_04";
    project.sounds[3].sourceUri = "lofibox-sample://groove-20260511-001/chop-04.wav";
    project.patterns[0].tracks[0].steps[0].trigger = true;
    project.patterns[0].tracks[0].steps[0].velocity = 110;
    project.patterns[0].tracks[0].steps[0].hasGainLock = true;
    project.patterns[0].tracks[0].steps[0].gain = 0.75f;
    project.songChain.enabled = true;
    project.songChain.items.push_back({0, 4, false, 0, "INTRO"});
    project.songChain.items.push_back({1, 8, false, 0, "BEAT"});

    const auto json = lofibox::groove::grooveProjectToJson(project);
    assert(json.find("\"schema_version\": 1") != std::string::npos);
    assert(json.find("\"name\": \"Late Beat\"") != std::string::npos);
    assert(json.find("\"sounds\"") != std::string::npos);
    assert(json.find("\"patterns\"") != std::string::npos);
    assert(json.find("\"song_chain\"") != std::string::npos);
    assert(json.find("webui") == std::string::npos);

    const auto parsed = lofibox::groove::grooveProjectFromJson(json);
    assert(parsed.id == project.id);
    assert(parsed.name == project.name);
    assert(parsed.bpm == 92);
    assert(parsed.swing == 12);
    assert(parsed.sounds[3].type == lofibox::groove::GrooveSoundType::CapturedFromTrack);
    assert(parsed.sounds[3].name == "CHOP_04");
    assert(parsed.sounds[3].sourceUri == "lofibox-sample://groove-20260511-001/chop-04.wav");
    assert(parsed.patterns[0].tracks[0].steps[0].trigger);
    assert(parsed.patterns[0].tracks[0].steps[0].velocity == 110);
    assert(parsed.patterns[0].tracks[0].steps[0].hasGainLock);
    assert(parsed.patterns[0].tracks[0].steps[0].gain == 0.75f);
    assert(parsed.songChain.enabled);
    assert(parsed.songChain.items.size() == 2);
    assert(parsed.songChain.items[0].label == "INTRO");
    assert(parsed.songChain.items[1].repeats == 8);
    assert(parsed.midi.inputChannel == 10);
    assert(parsed.exportSettings.sampleRate == 48000);

    const auto root = std::filesystem::temp_directory_path() / "lofibox-groove-project-smoke";
    lofibox::groove::GrooveStoragePaths paths{};
    paths.projectsDir = root / "projects";
    paths.samplesDir = root / "samples";
    paths.soundPacksDir = root / "soundpacks";
    paths.templatesDir = root / "templates";
    paths.cacheDir = root / "cache";
    paths.configFile = root / "config" / "groove.json";
    paths.exportsDir = root / "exports";
    paths.fallbackExportsDir = root / "fallback-exports";
    lofibox::groove::GrooveProjectRepository repo{paths};
    std::string error;
    assert(repo.save(project, &error));
    const auto loaded = repo.load(project.id, &error);
    assert(loaded.id == project.id);
    assert(loaded.sounds[3].sourceUri == project.sounds[3].sourceUri);
    assert(loaded.patterns[0].tracks[0].steps[0].trigger);
    assert(loaded.songChain.items.size() == 2);
    assert(!repo.listProjectFiles().empty());

    return 0;
}
