// SPDX-License-Identifier: GPL-3.0-or-later

#include "groove/groove_project.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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

[[nodiscard]] double extractDouble(std::string_view json, std::string_view key, double fallback)
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
    while (last < json.size()) {
        const char ch = json[last];
        const bool numeric = std::isdigit(static_cast<unsigned char>(ch)) != 0 ||
            ch == '-' || ch == '+' || ch == '.' || ch == 'e' || ch == 'E';
        if (!numeric) {
            break;
        }
        ++last;
    }
    if (last == first) {
        return fallback;
    }
    try {
        return std::stod(std::string{json.substr(first, last - first)});
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

[[nodiscard]] std::size_t findMatchingClose(std::string_view json, std::size_t open_pos, char open_char, char close_char)
{
    bool in_string = false;
    bool escaped = false;
    int depth = 0;
    for (std::size_t index = open_pos; index < json.size(); ++index) {
        const char ch = json[index];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                in_string = false;
            }
            continue;
        }
        if (ch == '"') {
            in_string = true;
            continue;
        }
        if (ch == open_char) {
            ++depth;
        } else if (ch == close_char) {
            --depth;
            if (depth == 0) {
                return index;
            }
        }
    }
    return std::string_view::npos;
}

[[nodiscard]] std::string_view extractBlock(std::string_view json, std::string_view key, char open_char, char close_char)
{
    const std::string needle = "\"" + std::string(key) + "\"";
    const std::size_t key_pos = json.find(needle);
    if (key_pos == std::string_view::npos) {
        return {};
    }
    const std::size_t colon = json.find(':', key_pos + needle.size());
    if (colon == std::string_view::npos) {
        return {};
    }
    const std::size_t open_pos = json.find(open_char, colon + 1);
    if (open_pos == std::string_view::npos) {
        return {};
    }
    const std::size_t close_pos = findMatchingClose(json, open_pos, open_char, close_char);
    if (close_pos == std::string_view::npos || close_pos < open_pos) {
        return {};
    }
    return json.substr(open_pos, close_pos - open_pos + 1U);
}

[[nodiscard]] std::vector<std::string_view> splitTopLevelObjects(std::string_view array_json)
{
    std::vector<std::string_view> objects{};
    bool in_string = false;
    bool escaped = false;
    int depth = 0;
    std::size_t object_start = std::string_view::npos;
    for (std::size_t index = 0; index < array_json.size(); ++index) {
        const char ch = array_json[index];
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (ch == '\\') {
                escaped = true;
            } else if (ch == '"') {
                in_string = false;
            }
            continue;
        }
        if (ch == '"') {
            in_string = true;
            continue;
        }
        if (ch == '{') {
            if (depth == 0) {
                object_start = index;
            }
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0 && object_start != std::string_view::npos) {
                objects.push_back(array_json.substr(object_start, index - object_start + 1U));
                object_start = std::string_view::npos;
            }
        }
    }
    return objects;
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

void parseSounds(std::string_view json, GrooveProject& project)
{
    const auto sounds_json = extractBlock(json, "sounds", '[', ']');
    if (sounds_json.empty()) {
        return;
    }
    for (const auto object : splitTopLevelObjects(sounds_json)) {
        const auto slot = static_cast<std::size_t>(std::clamp(extractInt(object, "slot", -1), 0, 15));
        auto& sound = project.sounds[slot];
        sound.type = grooveSoundTypeFromString(extractString(object, "type", toString(sound.type)));
        sound.id = extractString(object, "id", sound.id);
        sound.name = extractString(object, "name", sound.name);
        sound.sourceUri = extractString(object, "source_uri", sound.sourceUri);
        sound.mode = groovePlaybackModeFromString(extractString(object, "mode", toString(sound.mode)));
        sound.gain = static_cast<float>(extractDouble(object, "gain", sound.gain));
        sound.pitchSemitone = static_cast<float>(extractDouble(object, "pitch", sound.pitchSemitone));
        sound.pan = static_cast<float>(extractDouble(object, "pan", sound.pan));
        sound.startSeconds = extractDouble(object, "start", sound.startSeconds);
        sound.endSeconds = extractDouble(object, "end", sound.endSeconds);
        sound.fadeInMs = extractDouble(object, "fade_in_ms", sound.fadeInMs);
        sound.fadeOutMs = extractDouble(object, "fade_out_ms", sound.fadeOutMs);
        sound.normalized = extractBool(object, "normalized", sound.normalized);
        sound.chokeGroup = static_cast<std::uint8_t>(std::clamp(extractInt(object, "choke_group", sound.chokeGroup), 0, 16));

        const auto slices_json = extractBlock(object, "slices", '[', ']');
        sound.slices.clear();
        for (const auto slice_json : splitTopLevelObjects(slices_json)) {
            SampleSlice slice{};
            slice.id = extractString(slice_json, "id", {});
            slice.name = extractString(slice_json, "name", {});
            slice.startSeconds = extractDouble(slice_json, "start", 0.0);
            slice.endSeconds = extractDouble(slice_json, "end", 0.0);
            slice.pitchSemitone = static_cast<std::int8_t>(std::clamp(extractInt(slice_json, "pitch", 0), -24, 24));
            slice.gain = static_cast<float>(extractDouble(slice_json, "gain", 1.0));
            sound.slices.push_back(std::move(slice));
        }
    }
}

void parsePatterns(std::string_view json, GrooveProject& project)
{
    const auto patterns_json = extractBlock(json, "patterns", '[', ']');
    if (patterns_json.empty()) {
        return;
    }
    const auto pattern_objects = splitTopLevelObjects(patterns_json);
    for (std::size_t pattern_index = 0; pattern_index < pattern_objects.size() && pattern_index < project.patterns.size(); ++pattern_index) {
        const auto pattern_json = pattern_objects[pattern_index];
        auto& pattern = project.patterns[pattern_index];
        pattern.name = extractString(pattern_json, "name", pattern.name);
        pattern.length = static_cast<std::uint8_t>(std::clamp(extractInt(pattern_json, "length", pattern.length), 1, 16));

        const auto tracks_json = extractBlock(pattern_json, "tracks", '[', ']');
        const auto track_objects = splitTopLevelObjects(tracks_json);
        for (std::size_t track_index = 0; track_index < track_objects.size() && track_index < pattern.tracks.size(); ++track_index) {
            const auto track_json = track_objects[track_index];
            auto& track = pattern.tracks[track_index];
            track.soundSlot = static_cast<std::uint8_t>(std::clamp(extractInt(track_json, "sound_slot", track.soundSlot), 0, 15));
            track.gain = static_cast<float>(extractDouble(track_json, "gain", track.gain));
            track.pan = static_cast<float>(extractDouble(track_json, "pan", track.pan));
            track.mute = extractBool(track_json, "mute", track.mute);
            track.solo = extractBool(track_json, "solo", track.solo);

            const auto steps_json = extractBlock(track_json, "steps", '[', ']');
            const auto step_objects = splitTopLevelObjects(steps_json);
            for (std::size_t step_index = 0; step_index < step_objects.size() && step_index < track.steps.size(); ++step_index) {
                const auto step_json = step_objects[step_index];
                auto& step = track.steps[step_index];
                step.trigger = extractBool(step_json, "trigger", step.trigger);
                step.velocity = static_cast<std::uint8_t>(std::clamp(extractInt(step_json, "velocity", step.velocity), 0, 127));
                step.pitchSemitone = static_cast<std::int8_t>(std::clamp(extractInt(step_json, "pitch", step.pitchSemitone), -24, 24));
                step.microTiming = static_cast<std::int8_t>(std::clamp(extractInt(step_json, "micro_timing", step.microTiming), -96, 96));
                step.sliceIndex = static_cast<std::uint8_t>(std::clamp(extractInt(step_json, "slice", step.sliceIndex), 0, 15));
                if (step_json.find("\"gain\"") != std::string_view::npos) {
                    step.hasGainLock = true;
                    step.gain = static_cast<float>(extractDouble(step_json, "gain", step.gain));
                }
                if (step_json.find("\"pan\"") != std::string_view::npos) {
                    step.hasPanLock = true;
                    step.pan = static_cast<float>(extractDouble(step_json, "pan", step.pan));
                }
                if (step_json.find("\"filter_cutoff\"") != std::string_view::npos) {
                    step.hasFilterLock = true;
                    step.filterCutoff = static_cast<float>(extractDouble(step_json, "filter_cutoff", step.filterCutoff));
                }
                if (step_json.find("\"fx_type\"") != std::string_view::npos) {
                    step.hasFxLock = true;
                    step.fxType = static_cast<std::uint8_t>(std::clamp(extractInt(step_json, "fx_type", step.fxType), 0, 8));
                    step.fxAmount = static_cast<float>(extractDouble(step_json, "fx_amount", step.fxAmount));
                }
            }
        }
    }
}

void parseSongChain(std::string_view json, GrooveProject& project)
{
    const auto chain_json = extractBlock(json, "song_chain", '{', '}');
    if (chain_json.empty()) {
        return;
    }
    project.songChain.enabled = extractBool(chain_json, "enabled", project.songChain.enabled);
    project.songChain.items.clear();
    const auto items_json = extractBlock(chain_json, "items", '[', ']');
    for (const auto item_json : splitTopLevelObjects(items_json)) {
        GrooveSongChainItem item{};
        item.patternIndex = static_cast<std::uint8_t>(std::clamp(extractInt(item_json, "pattern", 0), 0, 15));
        item.repeats = static_cast<std::uint8_t>(std::clamp(extractInt(item_json, "repeats", 1), 1, 99));
        item.label = extractString(item_json, "label", {});
        project.songChain.items.push_back(std::move(item));
    }
    if (project.songChain.items.empty()) {
        project.songChain.enabled = false;
    }
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
    project.exportSettings.tailSeconds = extractDouble(json, "tail_seconds", project.exportSettings.tailSeconds);
    parseSounds(json, project);
    parsePatterns(json, project);
    parseSongChain(json, project);
    return project;
}

} // namespace lofibox::groove
