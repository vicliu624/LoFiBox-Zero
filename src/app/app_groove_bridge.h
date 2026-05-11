// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "application/groove_command_service.h"
#include "app/input_event.h"
#include "groove/groove_controller.h"
#include "ui/pages/groove/capture_overlay.h"
#include "ui/pages/groove/chain_overlay.h"
#include "ui/pages/groove/export_overlay.h"
#include "ui/pages/groove/fx_overlay.h"
#include "ui/pages/groove/midi_overlay.h"
#include "ui/pages/groove/pocket_groove_main_view.h"
#include "ui/pages/groove/project_overlay.h"
#include "ui/pages/groove/sample_edit_overlay.h"
#include "ui/pages/groove/slice_overlay.h"

namespace lofibox::app {

class GroovePlaybackControl {
public:
    virtual ~GroovePlaybackControl() = default;
    virtual void pauseCurrentPlaybackForGroove() = 0;
    virtual bool playGrooveRenderFile(const std::filesystem::path& path) { (void)path; return false; }
    virtual void stopGroovePlayback() {}
};

using GrooveCurrentPlaybackSource = ::lofibox::application::GrooveCaptureSource;

class AppGrooveBridge {
public:
    AppGrooveBridge();
    explicit AppGrooveBridge(lofibox::groove::GrooveProject project);

    [[nodiscard]] bool active() const noexcept;
    void enter(GroovePlaybackControl& playback);
    void exit();
    void openCaptureOverlay(GrooveCurrentPlaybackSource source);

    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> dispatch(const lofibox::groove::PocketGrooveCommand& command);
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> handleInput(const InputEvent& event, bool fn_held = false);

    [[nodiscard]] const lofibox::groove::GrooveController& controller() const noexcept;
    [[nodiscard]] lofibox::ui::pages::groove::PocketGrooveMainView mainView() const;
    [[nodiscard]] lofibox::groove::GrooveOverlay activeOverlay() const noexcept;
    [[nodiscard]] lofibox::ui::pages::groove::CaptureOverlayView captureOverlayView() const;
    [[nodiscard]] lofibox::ui::pages::groove::SampleEditOverlayView sampleEditOverlayView() const;
    [[nodiscard]] lofibox::ui::pages::groove::SliceOverlayView sliceOverlayView() const;
    [[nodiscard]] lofibox::ui::pages::groove::ChainOverlayView chainOverlayView() const;
    [[nodiscard]] lofibox::ui::pages::groove::FxOverlayView fxOverlayView() const;
    [[nodiscard]] lofibox::ui::pages::groove::MidiOverlayView midiOverlayView() const;
    [[nodiscard]] lofibox::ui::pages::groove::ExportOverlayView exportOverlayView() const;
    [[nodiscard]] lofibox::ui::pages::groove::ProjectOverlayView projectOverlayView() const;
    [[nodiscard]] bool dirty() const noexcept;
    [[nodiscard]] const std::string& lastStatus() const noexcept;

private:
    using clock = std::chrono::steady_clock;

    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> handleMainInput(const InputEvent& event, bool fn_held);
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> handleCaptureInput(const InputEvent& event);
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> handleSampleEditInput(const InputEvent& event);
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> handleChainInput(const InputEvent& event);
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> handleMidiInput(const InputEvent& event);
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> handleExportInput(const InputEvent& event);
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> handleProjectInput(const InputEvent& event);
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> executeCapture();
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> executeExport();
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> executeProjectAction();
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> rewriteSelectedSample(bool reverse);
    [[nodiscard]] std::vector<lofibox::groove::GrooveEvent> autoSliceSelectedSample();
    void applyOperationResult(const ::lofibox::application::GrooveOperationResult& result);
    void markDirty();
    void autoSave();
    void playRenderedProject();
    [[nodiscard]] std::uint8_t currentPlayheadStep() const noexcept;

    lofibox::groove::GrooveController controller_{};
    ::lofibox::application::GrooveCommandService operations_{};
    GroovePlaybackControl* playback_{nullptr};
    std::optional<GrooveCurrentPlaybackSource> captureSource_{};
    std::uint8_t captureLengthIndex_{1};
    std::uint8_t captureTargetSlot_{3};
    int sampleEditRow_{0};
    int midiRow_{0};
    int exportRow_{0};
    int exportProgress_{0};
    int projectAction_{0};
    std::string lastStatus_{"READY"};
    std::string lastExportFile_{};
    bool dirty_{false};
    clock::time_point playStarted_{};
    bool active_{false};
};

} // namespace lofibox::app
