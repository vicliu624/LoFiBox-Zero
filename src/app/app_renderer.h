// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app/app_page.h"
#include "app/app_page_model.h"
#include "app/app_state.h"
#include "app/library_model.h"
#include "app/navigation_state.h"
#include "groove/groove_controller.h"
#include "playback/playback_state.h"
#include "core/canvas.h"
#include "runtime/runtime_snapshot.h"
#include "ui/pages/groove/capture_overlay.h"
#include "ui/pages/groove/chain_overlay.h"
#include "ui/pages/groove/export_overlay.h"
#include "ui/pages/groove/fx_overlay.h"
#include "ui/pages/groove/midi_overlay.h"
#include "ui/pages/groove/pocket_groove_main_view.h"
#include "ui/pages/groove/project_overlay.h"
#include "ui/pages/groove/sample_edit_overlay.h"
#include "ui/pages/groove/slice_overlay.h"
#include "ui/ui_models.h"
#include "ui/ui_theme.h"

namespace lofibox::app {

class AppRenderTarget {
public:
    virtual ~AppRenderTarget() = default;

    [[nodiscard]] virtual AppPage currentPage() const noexcept = 0;
    [[nodiscard]] virtual const ui::UiAssets& assets() const noexcept = 0;
    [[nodiscard]] virtual std::chrono::steady_clock::time_point bootStarted() const noexcept = 0;
    [[nodiscard]] virtual LibraryIndexState libraryState() const noexcept = 0;
    [[nodiscard]] virtual LibraryScanProgress libraryScanProgress() const { return {}; }
    [[nodiscard]] virtual StorageInfo storage() const = 0;
    [[nodiscard]] virtual bool networkConnected() const noexcept = 0;
    [[nodiscard]] virtual int mainMenuIndex() const noexcept = 0;
    [[nodiscard]] virtual const PlaybackSession& playbackSession() const noexcept = 0;
    [[nodiscard]] virtual const TrackRecord* findTrack(int id) const noexcept = 0;
    [[nodiscard]] virtual const EqState& eqState() const noexcept = 0;
    [[nodiscard]] virtual runtime::EqRuntimeSnapshot eqRuntimeSnapshot() const noexcept = 0;
    [[nodiscard]] virtual ListSelection listSelection() const noexcept = 0;
    [[nodiscard]] virtual AppPageModel pageModel() const = 0;
    [[nodiscard]] virtual bool helpOpen() const noexcept = 0;
    [[nodiscard]] virtual AppPage helpPage() const noexcept = 0;
    [[nodiscard]] virtual const ui::UiTheme& theme() const noexcept = 0;
    [[nodiscard]] virtual ui::pages::groove::PocketGrooveMainView pocketGrooveMainView() const { return {}; }
    [[nodiscard]] virtual groove::GrooveOverlay pocketGrooveOverlay() const noexcept { return groove::GrooveOverlay::None; }
    [[nodiscard]] virtual ui::pages::groove::CaptureOverlayView pocketGrooveCaptureOverlayView() const { return {}; }
    [[nodiscard]] virtual ui::pages::groove::SampleEditOverlayView pocketGrooveSampleEditOverlayView() const { return {}; }
    [[nodiscard]] virtual ui::pages::groove::SliceOverlayView pocketGrooveSliceOverlayView() const { return {}; }
    [[nodiscard]] virtual ui::pages::groove::ChainOverlayView pocketGrooveChainOverlayView() const { return {}; }
    [[nodiscard]] virtual ui::pages::groove::FxOverlayView pocketGrooveFxOverlayView() const { return {}; }
    [[nodiscard]] virtual ui::pages::groove::MidiOverlayView pocketGrooveMidiOverlayView() const { return {}; }
    [[nodiscard]] virtual ui::pages::groove::ExportOverlayView pocketGrooveExportOverlayView() const { return {}; }
    [[nodiscard]] virtual ui::pages::groove::ProjectOverlayView pocketGrooveProjectOverlayView() const { return {}; }
};

void renderApp(core::Canvas& canvas, const AppRenderTarget& target);

} // namespace lofibox::app
