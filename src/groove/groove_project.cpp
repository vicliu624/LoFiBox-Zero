// SPDX-License-Identifier: GPL-3.0-or-later

#include "groove/groove_project.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <utility>

namespace lofibox::groove {
namespace {

[[nodiscard]] std::string escapeJson(std::string_view value)
{
    std::ostringstream out;
    for (char ch : value) {
        switch (ch) {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default: out << ch; break;
        }
    }
    return out.str();
}

[[nodiscard]] std::string extractString(std::string_view json, std::string_view key, std::string fallback)
{
    const std::string needle = "\"" + std::string(key) + "\"";
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string_view::npos) {
        return fallback;
    }
    const std::size_t colon = json.find(':', key_pos + needle.size());
    if (colon == std::string_view::npos) {
        return fallback;
    }
    std::size_t quote = json.find('"', colon + 1);
    if (quote == std::string_view::npos) {
        return fallback;
    }
    std::ostringstream out;
    for (std::size_t index = quote + 1; index < json.size(); ++index) {
        const char ch = json[index];
        if (ch == '"') {
            return out.str();
        }
        if (ch == '\\' && index + 1 < json.size()) {
            ++index;
            const char escaped = json[index];
            if (escaped == 'n') out << '\n';
            else if (escaped == 'r') out << '\r';
            else if (escaped == 't') out << '\t';
            else out << escaped;
        } else {
            out << ch;
        }
    }
    return fallback;
}

[[nodiscard]] int extractInt(std::string_view json, std::string_view key, int fallback)
{
    const std::string needle = "\"" + std::string(key) + "\"";
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string_view::npos) {
        return fallback;
    }
    const std::size_t colon = json.find(':', key_pos + needle.size());
    if (colon == std::string_view::npos) {
        return fallback;
    }
    std::size_t first = colon + 1;
    while (first < json.size() && std::isspace(static_cast<unsigned char>(json[first])) != 0) {
        ++first;
    }
    std::size_t last = first;
    while (last < json.size() && (std::isdigit(static_cast<unsigned char>(json[last])) != 0 || json[last] == '-')) {
        ++last;
    }
    if (last == first) {
        return fallback;
    }
    try {
        return std::stoi(std::string{json.substr(first, last - first)});
    } catch (...) {
        return fallback;
    }
}

[[nodiscard]] bool extractBool(std::string_view json, std::string_view key, bool fallback)
{
    const std::string needle = "\"" + std::string(key) + "\"";
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string_view::npos) {
        return fallback;
    }
    const std::size_t colon = json.find(':', key_pos + needle.size());
    if (colon == std::string_view::npos) {
        return fallback;
    }
    std::size_t first = colon + 1;
    while (first < json.size() && std::isspace(static_cast<unsigned char>(json[first])) != 0) {
        ++first;
    }
    if (json.substr(first, 4) == "true") {
        return true;
    }
    if (json.substr(first, 5) == "false") {
        return false;
    }
    return fallback;
}

void appendStepJson(std::ostringstream& out, const GrooveStep& step)
{
    out << "{\"trigger\":" << (step.trigger ? "true" : "false")
        << ",\"velocity\":" << static_cast<int>(step.velocity)
        << ",\"pitch\":" << static_cast<int>(step.pitchSemitone)
        << ",\"micro_timing\":" << static_cast<int>(step.microTiming)
        << ",\"slice\":" << static_cast<int>(step.sliceIndex);
    if (step.hasGainLock) {
        out << ",\"gain\":" << step.gain;
    }
    if (step.hasPanLock) {
        out << ",\"pan\":" << step.pan;
    }
    if (step.hasFilterLock) {
        out << ",\"filter_cutoff\":" << step.filterCutoff;
    }
    if (step.hasFxLock) {
        out << ",\"fx_type\":" << static_cast<int>(step.fxType)
            << ",\"fx_amount\":" << step.fxAmount;
    }
    out << '}';
}

} // namespace

void initializeDefaultSlotNotes(GrooveMidiSettings& midi) noexcept
{
    for (std::size_t index = 0; index < midi.slotNoteMap.size(); ++index) {
        midi.slotNoteMap[index] = static_cast<std::uint8_t>(36U + index);
    }
}

std::string patternName(std::uint8_t pattern_index)
{
    return "A" + std::to_string(static_cast<int>(pattern_index) + 1);
}

GrooveProject makeDefaultGrooveProject(std::string name)
{
    GrooveProject project{};
    project.id = "groove-new";
    project.name = std::move(name);
    initializeDefaultSlotNotes(project.midi);

    for (std::size_t pattern = 0; pattern < project.patterns.size(); ++pattern) {
        project.patterns[pattern].name = patternName(static_cast<std::uint8_t>(pattern));
        for (std::size_t track = 0; track < project.patterns[pattern].tracks.size(); ++track) {
            project.patterns[pattern].tracks[track].soundSlot = static_cast<std::uint8_t>(track);
        }
    }
    for (std::size_t slot = 0; slot < project.sounds.size(); ++slot) {
        project.sounds[slot].name = "S" + (slot + 1 < 10 ? std::string{"0"} : std::string{}) + std::to_string(slot + 1);
    }
    return project;
}

const char* toString(MidiClockMode mode) noexcept
{
    switch (mode) {
    case MidiClockMode::Internal: return "internal";
    case MidiClockMode::External: return "external";
    case MidiClockMode::Send: return "send";
    }
    return "internal";
}

const char* toString(GrooveExportTarget target) noexcept
{
    switch (target) {
    case GrooveExportTarget::CurrentPattern: return "current_pattern";
    case GrooveExportTarget::SongChain: return "song_chain";
    }
    return "song_chain";
}

MidiClockMode midiClockModeFromString(std::string_view value) noexcept
{
    if (value == "external") return MidiClockMode::External;
    if (value == "send") return MidiClockMode::Send;
    return MidiClockMode::Internal;
}

GrooveExportTarget grooveExportTargetFromString(std::string_view value) noexcept
{
    if (value == "current_pattern") return GrooveExportTarget::CurrentPattern;
    return GrooveExportTarget::SongChain;
}

std::string grooveProjectToJson(const GrooveProject& project)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(3);
    out << "{\n";
    out << "  \"schema_version\": 1,\n";
    out << "  \"id\": \"" << escapeJson(project.id) << "\",\n";
    out << "  \"name\": \"" << escapeJson(project.name) << "\",\n";
    out << "  \"bpm\": " << project.bpm << ",\n";
    out << "  \"swing\": " << static_cast<int>(project.swing) << ",\n";
    out << "  \"active_pattern\": " << static_cast<int>(project.activePattern) << ",\n";
    out << "  \"sounds\": [\n";
    for (std::size_t slot = 0; slot < project.sounds.size(); ++slot) {
        const auto& sound = project.sounds[slot];
        out << "    {\"slot\": " << slot
            << ", \"type\": \"" << toString(sound.type)
            << "\", \"name\": \"" << escapeJson(sound.name)
            << "\", \"source_uri\": \"" << escapeJson(sound.sourceUri)
            << "\", \"mode\": \"" << toString(sound.mode)
            << "\", \"gain\": " << sound.gain
            << ", \"pitch\": " << sound.pitchSemitone
            << ", \"pan\": " << sound.pan
            << ", \"start\": " << sound.startSeconds
            << ", \"end\": " << sound.endSeconds
            << ", \"fade_in_ms\": " << sound.fadeInMs
            << ", \"fade_out_ms\": " << sound.fadeOutMs
            << ", \"normalized\": " << (sound.normalized ? "true" : "false")
            << ", \"choke_group\": " << static_cast<int>(sound.chokeGroup)
            << ", \"slices\": [";
        for (std::size_t slice = 0; slice < sound.slices.size(); ++slice) {
            const auto& item = sound.slices[slice];
            if (slice != 0U) out << ", ";
            out << "{\"id\":\"" << escapeJson(item.id)
                << "\",\"name\":\"" << escapeJson(item.name)
                << "\",\"start\":" << item.startSeconds
                << ",\"end\":" << item.endSeconds
                << ",\"pitch\":" << static_cast<int>(item.pitchSemitone)
                << ",\"gain\":" << item.gain << "}";
        }
        out << "]}";
        out << (slot + 1U == project.sounds.size() ? "\n" : ",\n");
    }
    out << "  ],\n";
    out << "  \"patterns\": [\n";
    for (std::size_t pattern = 0; pattern < project.patterns.size(); ++pattern) {
        const auto& groove_pattern = project.patterns[pattern];
        out << "    {\"name\":\"" << escapeJson(groove_pattern.name)
            << "\",\"length\":" << static_cast<int>(groove_pattern.length)
            << ",\"tracks\":[";
        for (std::size_t track = 0; track < groove_pattern.tracks.size(); ++track) {
            const auto& groove_track = groove_pattern.tracks[track];
            if (track != 0U) out << ',';
            out << "{\"sound_slot\":" << static_cast<int>(groove_track.soundSlot)
                << ",\"gain\":" << groove_track.gain
                << ",\"pan\":" << groove_track.pan
                << ",\"mute\":" << (groove_track.mute ? "true" : "false")
                << ",\"solo\":" << (groove_track.solo ? "true" : "false")
                << ",\"steps\":[";
            for (std::size_t step = 0; step < groove_track.steps.size(); ++step) {
                if (step != 0U) out << ',';
                appendStepJson(out, groove_track.steps[step]);
            }
            out << "]}";
        }
        out << "]}";
        out << (pattern + 1U == project.patterns.size() ? "\n" : ",\n");
    }
    out << "  ],\n";
    out << "  \"song_chain\": {\"enabled\": " << (project.songChain.enabled ? "true" : "false") << ", \"items\": [";
    for (std::size_t index = 0; index < project.songChain.items.size(); ++index) {
        const auto& item = project.songChain.items[index];
        if (index != 0U) out << ", ";
        out << "{\"pattern\": " << static_cast<int>(item.patternIndex)
            << ", \"repeats\": " << static_cast<int>(item.repeats)
            << ", \"label\": \"" << escapeJson(item.label) << "\"}";
    }
    out << "]},\n";
    out << "  \"midi\": {\"clock_mode\": \"" << toString(project.midi.clockMode)
        << "\", \"input_channel\": " << static_cast<int>(project.midi.inputChannel)
        << ", \"output_channel\": " << static_cast<int>(project.midi.outputChannel)
        << ", \"note_input_enabled\": " << (project.midi.noteInputEnabled ? "true" : "false")
        << ", \"clock_input_enabled\": " << (project.midi.clockInputEnabled ? "true" : "false")
        << ", \"clock_output_enabled\": " << (project.midi.clockOutputEnabled ? "true" : "false")
        << "},\n";
    out << "  \"export\": {\"target\": \"" << toString(project.exportSettings.target)
        << "\", \"sample_rate\": " << project.exportSettings.sampleRate
        << ", \"bit_depth\": " << project.exportSettings.bitDepth
        << ", \"normalize\": " << (project.exportSettings.normalize ? "true" : "false")
        << ", \"include_master_fx\": " << (project.exportSettings.includeMasterFx ? "true" : "false")
        << ", \"include_tail\": " << (project.exportSettings.includeTail ? "true" : "false")
        << ", \"tail_seconds\": " << project.exportSettings.tailSeconds << "}\n";
    out << "}\n";
    return out.str();
}

GrooveProject grooveProjectFromJson(std::string_view json)
{
    GrooveProject project = makeDefaultGrooveProject();
    project.id = extractString(json, "id", project.id);
    project.name = extractString(json, "name", project.name);
    project.bpm = static_cast<std::uint16_t>(std::clamp(extractInt(json, "bpm", project.bpm), 40, 300));
    project.swing = static_cast<std::uint8_t>(std::clamp(extractInt(json, "swing", project.swing), 0, 75));
    project.activePattern = static_cast<std::uint8_t>(std::clamp(extractInt(json, "active_pattern", project.activePattern), 0, 15));
    project.midi.clockMode = midiClockModeFromString(extractString(json, "clock_mode", toString(project.midi.clockMode)));
    project.midi.inputChannel = static_cast<std::uint8_t>(std::clamp(extractInt(json, "input_channel", project.midi.inputChannel), 1, 16));
    project.midi.outputChannel = static_cast<std::uint8_t>(std::clamp(extractInt(json, "output_channel", project.midi.outputChannel), 1, 16));
    project.midi.noteInputEnabled = extractBool(json, "note_input_enabled", project.midi.noteInputEnabled);
    project.midi.clockInputEnabled = extractBool(json, "clock_input_enabled", project.midi.clockInputEnabled);
    project.midi.clockOutputEnabled = extractBool(json, "clock_output_enabled", project.midi.clockOutputEnabled);
    project.exportSettings.target = grooveExportTargetFromString(extractString(json, "target", toString(project.exportSettings.target)));
    project.exportSettings.sampleRate = static_cast<std::uint32_t>(std::clamp(extractInt(json, "sample_rate", project.exportSettings.sampleRate), 8000, 192000));
    project.exportSettings.bitDepth = static_cast<std::uint16_t>(std::clamp(extractInt(json, "bit_depth", project.exportSettings.bitDepth), 16, 24));
    project.exportSettings.normalize = extractBool(json, "normalize", project.exportSettings.normalize);
    project.exportSettings.includeMasterFx = extractBool(json, "include_master_fx", project.exportSettings.includeMasterFx);
    project.exportSettings.includeTail = extractBool(json, "include_tail", project.exportSettings.includeTail);
    return project;
}

} // namespace lofibox::groove
