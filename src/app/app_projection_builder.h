// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/app_page.h"
#include "ui/pages/about_page.h"
#include "ui/pages/equalizer_page.h"
#include "ui/pages/list_page.h"
#include "ui/pages/lyrics_page.h"
#include "ui/pages/main_menu_page.h"
#include "ui/pages/now_playing_page.h"

namespace lofibox::app {

class AppRenderTarget;

[[nodiscard]] ui::pages::MainMenuView buildMainMenuProjection(const AppRenderTarget& target);
[[nodiscard]] ui::pages::NowPlayingView buildNowPlayingProjection(const AppRenderTarget& target);
[[nodiscard]] ui::pages::LyricsPageView buildLyricsProjection(const AppRenderTarget& target);
[[nodiscard]] ui::pages::ListPageView buildListProjection(const AppRenderTarget& target);
[[nodiscard]] ui::pages::AboutPageView buildAboutProjection(const AppRenderTarget& target);
[[nodiscard]] ui::pages::EqualizerPageView buildEqualizerProjection(const AppRenderTarget& target);

} // namespace lofibox::app
