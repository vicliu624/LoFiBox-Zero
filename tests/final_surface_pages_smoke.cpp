// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_page_model.h"

#include <cassert>

int main()
{
    for (const auto page : {
             lofibox::app::AppPage::Search,
             lofibox::app::AppPage::Queue,
             lofibox::app::AppPage::RemoteBrowse,
             lofibox::app::AppPage::ServerDiagnostics,
             lofibox::app::AppPage::StreamDetail,
             lofibox::app::AppPage::PlaylistEditor,
         }) {
        lofibox::app::AppPageModelInput input{};
        input.page = page;
        input.network_connected = true;
        const auto model = lofibox::app::buildAppPageModel(input);
        assert(!model.title.empty());
        assert(model.browse_list);
        assert(!model.rows.empty());
    }
    return 0;
}
