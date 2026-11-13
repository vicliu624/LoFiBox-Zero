# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.24)

project(LoFiBoxZero VERSION 0.3.9 LANGUAGES C CXX)

if(DEFINED CMAKE_EXPORT_NO_PACKAGE_REGISTRY)
    set(_lofibox_cmake_export_no_package_registry "${CMAKE_EXPORT_NO_PACKAGE_REGISTRY}")
endif()
if(DEFINED CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY)
    set(_lofibox_cmake_find_package_no_package_registry "${CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY}")
endif()
if(DEFINED FETCHCONTENT_FULLY_DISCONNECTED)
    set(_lofibox_fetchcontent_fully_disconnected "${FETCHCONTENT_FULLY_DISCONNECTED}")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

include(GNUInstallDirs)
include(CTest)
find_package(Threads REQUIRED)

set(LOFIBOX_PRIVATE_LIBDIR "lib/lofibox" CACHE PATH "Private LoFiBox runtime directory")
set(CMAKE_INSTALL_DOCDIR "${CMAKE_INSTALL_DATAROOTDIR}/doc/lofibox")

set(LOFIBOX_BUILD_DEVICE_DEFAULT OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LOFIBOX_BUILD_DEVICE_DEFAULT ON)
endif()

set(LOFIBOX_BUILD_WEBUI_DEFAULT OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LOFIBOX_BUILD_WEBUI_DEFAULT ON)
endif()

option(LOFIBOX_BUILD_DEVICE "Build the Linux framebuffer device target." ${LOFIBOX_BUILD_DEVICE_DEFAULT})
option(LOFIBOX_BUILD_X11 "Build the Linux X11 VNC/PocketFrame target." ${LOFIBOX_BUILD_DEVICE_DEFAULT})
option(LOFIBOX_BUILD_WAYLAND "Build the Linux native Wayland toplevel target." ${LOFIBOX_BUILD_DEVICE_DEFAULT})
option(LOFIBOX_BUILD_TUI "Build the ANSI terminal UI target." ON)
option(LOFIBOX_BUILD_WEBUI "Build the WebUI remote-control HTTP/WebSocket server." ${LOFIBOX_BUILD_WEBUI_DEFAULT})
option(LOFIBOX_INSTALL_LAUNCHER "Install the generic runtime-selecting lofibox launcher." ON)
option(LOFIBOX_INSTALL_DOCUMENTATION "Install the full user and engineering documentation set." ON)
option(LOFIBOX_BROWSER_TESTS "Enable optional Chromium/Weston integration tests." OFF)
option(LOFIBOX_BUILD_NATIVE_MP3_HELPER "Build the Linux native MP3 PCM producer." ON)

set(LOFIBOX_PRODUCT_PROFILE "developer" CACHE STRING "Product artifact: developer, cardputer, or debian")
set_property(CACHE LOFIBOX_PRODUCT_PROFILE PROPERTY STRINGS developer cardputer debian)
if(LOFIBOX_PRODUCT_PROFILE STREQUAL "cardputer")
    set(LOFIBOX_INSTALL_NAMESPACE "lofibox-cardputer-zero")
    set(LOFIBOX_PRIVATE_LIBDIR "lib/${LOFIBOX_INSTALL_NAMESPACE}" CACHE PATH "Private LoFiBox runtime directory" FORCE)
    set(LOFIBOX_BUILD_DEVICE ON CACHE BOOL "Build framebuffer runtime" FORCE)
    set(LOFIBOX_BUILD_WAYLAND OFF CACHE BOOL "Build Wayland runtime" FORCE)
    set(LOFIBOX_BUILD_X11 OFF CACHE BOOL "Build X11 runtime" FORCE)
elseif(LOFIBOX_PRODUCT_PROFILE STREQUAL "debian")
    set(LOFIBOX_INSTALL_NAMESPACE "lofibox")
    set(LOFIBOX_PRIVATE_LIBDIR "lib/lofibox" CACHE PATH "Private LoFiBox runtime directory" FORCE)
    set(LOFIBOX_BUILD_DEVICE OFF CACHE BOOL "Build framebuffer runtime" FORCE)
    set(LOFIBOX_BUILD_WAYLAND ON CACHE BOOL "Build Wayland runtime" FORCE)
    set(LOFIBOX_BUILD_X11 OFF CACHE BOOL "Build X11 runtime" FORCE)
elseif(NOT LOFIBOX_PRODUCT_PROFILE STREQUAL "developer")
    message(FATAL_ERROR "Unknown LOFIBOX_PRODUCT_PROFILE: ${LOFIBOX_PRODUCT_PROFILE}")
else()
    set(LOFIBOX_INSTALL_NAMESPACE "lofibox")
endif()

add_library(lofibox_zero_core STATIC
    src/audio/audio_pipeline_controller.cpp
    src/cache/cache_manager.cpp
    src/audio/decoder/audio_decoder_contract.cpp
        src/audio/output/audio_output_contract.cpp
    src/audio/dsp/audio_effect_registry.cpp
    src/audio/dsp/dsp_chain.cpp
    src/audio/dsp/realtime_dsp_engine.cpp
    src/app/input_actions.cpp
    src/app/app_command_executor.cpp
    src/app/app_lifecycle.cpp
    src/app/app_input_router.cpp
    src/app/app_page_model.cpp
    src/app/app_projection_builder.cpp
    src/app/app_renderer.cpp
    src/app/app_runtime_context.cpp
    src/application/app_service_registry.cpp
    src/application/cache_command_service.cpp
    src/application/credential_command_service.cpp
    src/application/library_mutation_service.cpp
    src/application/library_open_action_service.cpp
    src/application/library_query_service.cpp
    src/application/playback_command_service.cpp
    src/application/playback_status_query_service.cpp
    src/application/queue_command_service.cpp
    src/application/remote_browse_query_service.cpp
    src/application/runtime_diagnostic_service.cpp
    src/application/source_profile_command_service.cpp
    src/runtime/eq_runtime.cpp
    src/runtime/creator_analysis_runtime.cpp
    src/runtime/in_process_runtime_client.cpp
    src/runtime/playback_runtime.cpp
    src/runtime/queue_runtime.cpp
    src/runtime/remote_session_runtime.cpp
    src/runtime/runtime_command_bus.cpp
    src/runtime/runtime_command_dispatcher.cpp
    src/runtime/runtime_command_server.cpp
    src/runtime/runtime_event.cpp
    src/runtime/runtime_envelope_serializer.cpp
    src/runtime/runtime_host.cpp
    src/runtime/runtime_query_dispatcher.cpp
    src/runtime/runtime_result.cpp
    src/runtime/runtime_session_facade.cpp
    src/runtime/runtime_snapshot_assembler.cpp
    src/runtime/settings_runtime.cpp
    src/runtime/unix_socket_runtime_transport.cpp
    src/app/library_controller.cpp
    src/app/library_navigation_service.cpp
    src/app/library_repository.cpp
    src/app/library_scanner.cpp
    src/app/lofibox_app.cpp
    src/app/lofibox_app_runner.cpp
    src/app/media_item.cpp
    src/app/media_search_service.cpp
    src/app/remote_media_contract.cpp
    src/playback/mixed_playback_queue.cpp
    src/playback/playback_enrichment_coordinator.cpp
    src/playback/playback_backend_controller.cpp
    src/playback/playback_completion_policy.cpp
    src/playback/playback_runtime_coordinator.cpp
    src/playback/playback_session_clock.cpp
    src/playback/playback_visualization_source.cpp
    src/app/remote_profile_store.cpp
    src/app/settings_projection_builder.cpp
    src/app/source_manager_projection.cpp
    src/app/text_input_utils.cpp
    src/ui/pages/about_page.cpp
    src/ui/pages/equalizer_page.cpp
    src/ui/pages/list_page.cpp
    src/ui/pages/lyrics_page.cpp
    src/ui/pages/main_menu_page.cpp
    src/ui/pages/now_playing_page.cpp
    src/ui/effects/lyrics_spectrum_effect.cpp
    src/ui/widgets/lyrics_layout.cpp
    src/playback/playback_controller.cpp
    src/app/runtime_services.cpp
    src/metadata/enrichment_authority_policy.cpp
    src/metadata/match_confidence_guard.cpp
    src/metadata/metadata_governance.cpp
    src/metadata/metadata_merge_policy.cpp
    src/remote/common/remote_catalog_model.cpp
    src/remote/common/remote_provider_contract.cpp
    src/remote/common/remote_source_registry.cpp
    src/remote/common/stream_source_model.cpp
        src/playlist/playlist_parser.cpp
        src/plugins/plugin_manifest.cpp
        src/plugins/plugin_discovery.cpp
        src/plugins/skin_plugin_adapter.cpp
        src/plugins/plugin_runtime.cpp
        src/plugins/remote_plugin_adapter.cpp
    src/desktop/desktop_integration_boundary.cpp
    src/security/credential_policy.cpp
    src/ui/ui_primitives.cpp
    src/core/bitmap_font.cpp
    src/core/canvas.cpp
    src/library/library_governance.cpp
    src/library/library_indexer.cpp
    src/library/library_store.cpp
    src/playback/playback_stability_policy.cpp
    src/playback/streaming_playback_policy.cpp
)

target_include_directories(lofibox_zero_core PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_features(lofibox_zero_core PUBLIC cxx_std_20)
target_link_libraries(lofibox_zero_core PUBLIC Threads::Threads)
target_compile_definitions(lofibox_zero_core PUBLIC
    LOFIBOX_VERSION="${PROJECT_VERSION}"
    LOFIBOX_ASSET_DIR="${PROJECT_SOURCE_DIR}/assets"
    LOFIBOX_SYSTEM_ASSET_DIR="${CMAKE_INSTALL_FULL_DATADIR}/${LOFIBOX_INSTALL_NAMESPACE}/assets"
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Freetype REQUIRED)
    target_link_libraries(lofibox_zero_core PUBLIC Freetype::Freetype)
    target_compile_definitions(lofibox_zero_core PRIVATE LOFIBOX_HAVE_FREETYPE=1)
endif()

if(MSVC)
    target_compile_options(lofibox_zero_core PRIVATE /W4 /permissive- /FS)
else()
    target_compile_options(lofibox_zero_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(lofibox_zero_host_lock STATIC
    src/platform/host/runtime_paths.cpp
    src/platform/host/single_instance_lock.cpp
)
target_include_directories(lofibox_zero_host_lock PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_features(lofibox_zero_host_lock PUBLIC cxx_std_20)
target_link_libraries(lofibox_zero_host_lock PUBLIC Threads::Threads)
if(MSVC)
    target_compile_options(lofibox_zero_host_lock PRIVATE /W4 /permissive- /FS)
else()
    target_compile_options(lofibox_zero_host_lock PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(lofibox_zero_host_runtime STATIC
    src/platform/host/acoustid_client.cpp
    src/audio/output/host_audio_playback_backend.cpp
    src/platform/host/artwork_provider.cpp
    src/platform/host/connectivity_provider.cpp
    src/platform/host/cover_art_archive_client.cpp
    src/platform/host/ffprobe_metadata_reader.cpp
    src/platform/host/host_runtime_service_providers.cpp
    src/platform/host/runtime_preferences_store.cpp
    src/platform/host/lyrics_provider.cpp
    src/platform/host/lyrics_pipeline_components.cpp
    src/platform/host/lyrics_source_clients.cpp
    src/platform/host/legacy_asset_loader.cpp
    src/platform/host/media_runtime_capabilities.cpp
    src/platform/host/metadata_provider.cpp
    src/platform/host/musicbrainz_client.cpp
    src/platform/host/png_canvas_loader.cpp
    src/platform/host/remote_media_runtime.cpp
    src/platform/host/runtime_cache_store.cpp
    src/platform/host/runtime_enrichment_clients.cpp
    src/platform/host/runtime_host_internal.cpp
    src/platform/host/runtime_host_tools.cpp
    src/platform/host/runtime_logger.cpp
    src/platform/host/runtime_metadata_enrichment_orchestrator.cpp
    src/platform/host/runtime_process.cpp
    src/platform/host/runtime_remote_media_tool.cpp
    src/platform/host/runtime_script_bridge.cpp
    src/platform/host/runtime_services_factory.cpp
    src/platform/host/tag_writer.cpp
    src/platform/host/track_identity_provider.cpp
    src/platform/host/xdg_remote_profile_store.cpp
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND LOFIBOX_BUILD_NATIVE_MP3_HELPER)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MPG123 QUIET IMPORTED_TARGET libmpg123)
    pkg_check_modules(SOXR QUIET IMPORTED_TARGET soxr)
    if(TARGET PkgConfig::MPG123 AND TARGET PkgConfig::SOXR)
        add_executable(lofibox_mp3_decoder src/audio/decoder/native_mp3_helper.cpp)
        set_target_properties(lofibox_mp3_decoder PROPERTIES OUTPUT_NAME lofibox-mp3-decoder)
        target_include_directories(lofibox_mp3_decoder PRIVATE "${PROJECT_SOURCE_DIR}/src")
        target_link_libraries(lofibox_mp3_decoder PRIVATE PkgConfig::MPG123 PkgConfig::SOXR Threads::Threads)
        target_compile_options(lofibox_mp3_decoder PRIVATE -Wall -Wextra -Wpedantic)
        add_dependencies(lofibox_zero_host_runtime lofibox_mp3_decoder)
        install(TARGETS lofibox_mp3_decoder RUNTIME DESTINATION "${LOFIBOX_PRIVATE_LIBDIR}")
    elseif(NOT LOFIBOX_PRODUCT_PROFILE STREQUAL "developer")
        message(FATAL_ERROR "Product packages require libmpg123-dev and libsoxr-dev for the native MP3 helper")
    else()
        message(STATUS "libmpg123-dev/libsoxr-dev unavailable; developer playback uses FFmpeg")
    endif()
endif()

target_include_directories(lofibox_zero_host_runtime PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_features(lofibox_zero_host_runtime PUBLIC cxx_std_20)
target_link_libraries(lofibox_zero_host_runtime PUBLIC lofibox_zero_host_lock lofibox_zero_core Threads::Threads)
target_compile_definitions(lofibox_zero_host_runtime PRIVATE
    LOFIBOX_PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    LOFIBOX_INSTALLED_HELPER_DIR="${CMAKE_INSTALL_PREFIX}/${LOFIBOX_PRIVATE_LIBDIR}"
    LOFIBOX_SYSTEM_PLUGIN_DIR="${CMAKE_INSTALL_FULL_DATADIR}/${LOFIBOX_INSTALL_NAMESPACE}/plugins"
)
if(MSVC)
    target_compile_options(lofibox_zero_host_runtime PRIVATE /W4 /permissive- /FS)
else()
    target_compile_options(lofibox_zero_host_runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(lofibox_zero_target_cli STATIC
    src/cli/direct_cli.cpp
    src/cli/runtime_cli.cpp
    src/targets/cli_options.cpp
)
target_include_directories(lofibox_zero_target_cli PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_features(lofibox_zero_target_cli PUBLIC cxx_std_20)
target_link_libraries(lofibox_zero_target_cli PUBLIC lofibox_zero_core)
if(MSVC)
    target_compile_options(lofibox_zero_target_cli PRIVATE /W4 /permissive- /FS)
else()
    target_compile_options(lofibox_zero_target_cli PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(LOFIBOX_BUILD_WEBUI)
    add_library(lofibox_webui STATIC
        src/webui/webui_json.cpp
        src/webui/webui_config.cpp
        src/webui/webui_runtime_adapter.cpp
        src/webui/webui_projection.cpp
        src/webui/webui_static_assets.cpp
        src/webui/webui_http_router.cpp
        src/webui/webui_ws_runtime_stream.cpp
        src/webui/webui_server.cpp
    )
    target_include_directories(lofibox_webui PUBLIC "${PROJECT_SOURCE_DIR}/src")
    target_compile_features(lofibox_webui PUBLIC cxx_std_20)
    target_link_libraries(lofibox_webui PUBLIC lofibox_zero_core)
    if(MSVC)
        target_compile_options(lofibox_webui PRIVATE /W4 /permissive- /FS)
    else()
        target_compile_options(lofibox_webui PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(LOFIBOX_BUILD_TUI)
    add_library(lofibox_zero_tui STATIC
        src/tui/tui_app.cpp
        src/tui/tui_model.cpp
        src/tui/tui_state.cpp
        src/tui/tui_view_mode.cpp
        src/tui/tui_renderer.cpp
        src/tui/tui_layout_engine.cpp
        src/tui/tui_input_router.cpp
        src/tui/tui_command_palette.cpp
        src/tui/widgets/tui_box.cpp
        src/tui/widgets/tui_text.cpp
        src/tui/widgets/tui_progress_bar.cpp
        src/tui/widgets/tui_spectrum.cpp
        src/tui/widgets/tui_vu_meter.cpp
        src/tui/widgets/tui_lyrics_view.cpp
        src/tui/widgets/tui_queue_view.cpp
        src/tui/widgets/tui_eq_view.cpp
        src/tui/widgets/tui_status_line.cpp
        src/tui/widgets/tui_help_overlay.cpp
        src/tui/pages/dashboard_tui_page.cpp
        src/tui/pages/now_playing_tui_page.cpp
        src/tui/pages/lyrics_tui_page.cpp
        src/tui/pages/spectrum_tui_page.cpp
        src/tui/pages/queue_tui_page.cpp
        src/tui/pages/library_tui_page.cpp
        src/tui/pages/sources_tui_page.cpp
        src/tui/pages/eq_tui_page.cpp
        src/tui/pages/dsp_tui_page.cpp
        src/tui/pages/diagnostics_tui_page.cpp
        src/tui/pages/creator_tui_page.cpp
        src/tui/pages/help_tui_page.cpp
        src/tui/pages/command_palette_tui_page.cpp
        src/platform/tui/terminal_presenter.cpp
        src/platform/tui/terminal_input.cpp
        src/platform/tui/terminal_capabilities.cpp
        src/platform/tui/ansi_escape.cpp
    )
    target_include_directories(lofibox_zero_tui PUBLIC "${PROJECT_SOURCE_DIR}/src")
    target_compile_features(lofibox_zero_tui PUBLIC cxx_std_20)
    target_link_libraries(lofibox_zero_tui PUBLIC lofibox_zero_core)
    if(MSVC)
        target_compile_options(lofibox_zero_tui PRIVATE /W4 /permissive- /FS)
    else()
        target_compile_options(lofibox_zero_tui PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_executable(lofibox_zero_tui_bin
        src/targets/tui_main.cpp
    )
    set_target_properties(lofibox_zero_tui_bin PROPERTIES OUTPUT_NAME "${LOFIBOX_INSTALL_NAMESPACE}-tui")
    target_link_libraries(lofibox_zero_tui_bin PRIVATE lofibox_zero_tui)
    install(TARGETS lofibox_zero_tui_bin
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()

if(LOFIBOX_BUILD_X11)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(FATAL_ERROR "LOFIBOX_BUILD_X11 requires a Linux target environment.")
    endif()

    find_package(X11 REQUIRED)

    add_executable(lofibox_zero_x11
        src/platform/x11/x11_presenter.cpp
        src/targets/x11_main.cpp
    )
    set_target_properties(lofibox_zero_x11 PROPERTIES OUTPUT_NAME lofibox-x11)
    target_link_libraries(lofibox_zero_x11 PRIVATE
        lofibox_zero_host_runtime
        lofibox_zero_target_cli
        X11::X11
    )
    if(TARGET lofibox_zero_tui)
        target_link_libraries(lofibox_zero_x11 PRIVATE lofibox_zero_tui)
        target_compile_definitions(lofibox_zero_x11 PRIVATE LOFIBOX_HAVE_TUI=1)
    endif()
    if(TARGET lofibox_webui)
        target_link_libraries(lofibox_zero_x11 PRIVATE lofibox_zero_core lofibox_webui)
        target_compile_definitions(lofibox_zero_x11 PRIVATE LOFIBOX_HAVE_WEBUI=1)
    endif()
    install(TARGETS lofibox_zero_x11
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()

if(LOFIBOX_BUILD_WAYLAND)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(FATAL_ERROR "LOFIBOX_BUILD_WAYLAND requires a Linux target environment.")
    endif()

    # A normal Linux developer install obtains the scanner and XML files through
    # pkg-config.  The TDVP feed build instead uses the matching Buildroot host
    # tools and protocol sources, which keeps the generated client ABI tied to
    # the firmware SDK instead of whatever happens to be installed on the build
    # machine.
    set(LOFIBOX_WAYLAND_SCANNER_EXECUTABLE "" CACHE FILEPATH
        "Path to the wayland-scanner executable used to generate client protocols")
    set(LOFIBOX_WAYLAND_PROTOCOLS_DIR "" CACHE PATH
        "Directory containing stable/ and unstable/ Wayland protocol XML files")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
    pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)

    if(LOFIBOX_WAYLAND_SCANNER_EXECUTABLE)
        set(WAYLAND_SCANNER_EXECUTABLE "${LOFIBOX_WAYLAND_SCANNER_EXECUTABLE}")
    else()
        find_program(WAYLAND_SCANNER_EXECUTABLE wayland-scanner REQUIRED)
    endif()
    if(NOT EXISTS "${WAYLAND_SCANNER_EXECUTABLE}")
        message(FATAL_ERROR "wayland-scanner not found at ${WAYLAND_SCANNER_EXECUTABLE}")
    endif()

    if(LOFIBOX_WAYLAND_PROTOCOLS_DIR)
        set(WAYLAND_PROTOCOLS_DIR "${LOFIBOX_WAYLAND_PROTOCOLS_DIR}")
    else()
        pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
        pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
    endif()
    set(LOFIBOX_XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
    if(NOT EXISTS "${LOFIBOX_XDG_SHELL_XML}")
        message(FATAL_ERROR "xdg-shell.xml not found at ${LOFIBOX_XDG_SHELL_XML}")
    endif()

    set(LOFIBOX_WAYLAND_PROTOCOL_DIR "${CMAKE_BINARY_DIR}/generated/wayland")
    set(LOFIBOX_XDG_SHELL_HEADER "${LOFIBOX_WAYLAND_PROTOCOL_DIR}/xdg-shell-client-protocol.h")
    set(LOFIBOX_XDG_SHELL_SOURCE "${LOFIBOX_WAYLAND_PROTOCOL_DIR}/xdg-shell-protocol.c")
    set(LOFIBOX_XDG_DECORATION_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml")
    set(LOFIBOX_XDG_DECORATION_HEADER "${LOFIBOX_WAYLAND_PROTOCOL_DIR}/xdg-decoration-unstable-v1-client-protocol.h")
    set(LOFIBOX_XDG_DECORATION_SOURCE "${LOFIBOX_WAYLAND_PROTOCOL_DIR}/xdg-decoration-unstable-v1-protocol.c")
    set(LOFIBOX_WAYLAND_PROTOCOL_SOURCES
        "${LOFIBOX_XDG_SHELL_SOURCE}"
        "${LOFIBOX_XDG_SHELL_HEADER}"
        "${LOFIBOX_XDG_DECORATION_SOURCE}"
        "${LOFIBOX_XDG_DECORATION_HEADER}"
    )
    file(MAKE_DIRECTORY "${LOFIBOX_WAYLAND_PROTOCOL_DIR}")

    if(NOT EXISTS "${LOFIBOX_XDG_DECORATION_XML}")
        message(FATAL_ERROR "xdg-decoration-unstable-v1.xml not found at ${LOFIBOX_XDG_DECORATION_XML}")
    endif()

    add_custom_command(
        OUTPUT "${LOFIBOX_XDG_SHELL_HEADER}"
        COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" client-header
                "${LOFIBOX_XDG_SHELL_XML}"
                "${LOFIBOX_XDG_SHELL_HEADER}"
        DEPENDS "${LOFIBOX_XDG_SHELL_XML}"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${LOFIBOX_XDG_SHELL_SOURCE}"
        COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" private-code
                "${LOFIBOX_XDG_SHELL_XML}"
                "${LOFIBOX_XDG_SHELL_SOURCE}"
        DEPENDS "${LOFIBOX_XDG_SHELL_XML}"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${LOFIBOX_XDG_DECORATION_HEADER}"
        COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" client-header
                "${LOFIBOX_XDG_DECORATION_XML}"
                "${LOFIBOX_XDG_DECORATION_HEADER}"
        DEPENDS "${LOFIBOX_XDG_DECORATION_XML}"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${LOFIBOX_XDG_DECORATION_SOURCE}"
        COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" private-code
                "${LOFIBOX_XDG_DECORATION_XML}"
                "${LOFIBOX_XDG_DECORATION_SOURCE}"
        DEPENDS "${LOFIBOX_XDG_DECORATION_XML}"
        VERBATIM
    )

    add_library(lofibox_wayland_protocol STATIC
        ${LOFIBOX_WAYLAND_PROTOCOL_SOURCES}
    )
    target_include_directories(lofibox_wayland_protocol PUBLIC "${LOFIBOX_WAYLAND_PROTOCOL_DIR}")
    target_link_libraries(lofibox_wayland_protocol PUBLIC PkgConfig::WAYLAND_CLIENT)

    if(LOFIBOX_BUILD_WAYLAND)
        add_executable(lofibox_zero_wayland
            src/platform/wayland/wayland_presenter.cpp
            src/targets/wayland_main.cpp
        )
        set_target_properties(lofibox_zero_wayland PROPERTIES OUTPUT_NAME lofibox-wayland)
        target_link_libraries(lofibox_zero_wayland PRIVATE
            lofibox_zero_host_runtime
            lofibox_zero_target_cli
            lofibox_wayland_protocol
            PkgConfig::WAYLAND_CLIENT
            PkgConfig::XKBCOMMON
            rt
        )
        target_include_directories(lofibox_zero_wayland PRIVATE "${LOFIBOX_WAYLAND_PROTOCOL_DIR}")
        if(TARGET lofibox_zero_tui)
            target_link_libraries(lofibox_zero_wayland PRIVATE lofibox_zero_tui)
            target_compile_definitions(lofibox_zero_wayland PRIVATE LOFIBOX_HAVE_TUI=1)
        endif()
        if(TARGET lofibox_webui)
            target_link_libraries(lofibox_zero_wayland PRIVATE lofibox_zero_core lofibox_webui)
            target_compile_definitions(lofibox_zero_wayland PRIVATE LOFIBOX_HAVE_WEBUI=1)
        endif()
        install(TARGETS lofibox_zero_wayland
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        )
    endif()

endif()

if(LOFIBOX_BUILD_DEVICE)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(FATAL_ERROR "LOFIBOX_BUILD_DEVICE requires a Linux target environment.")
    endif()

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)

    add_executable(lofibox_zero_device
        src/platform/device/cardputer_rhythm_lights.cpp
        src/platform/device/linux_evdev_keyboard.cpp
        src/platform/device/linux_framebuffer.cpp
        src/platform/device/framebuffer_blitter.cpp
        src/platform/device/linux_framebuffer_platform.cpp
        src/targets/device_main.cpp
    )
    target_link_libraries(lofibox_zero_device PRIVATE
        lofibox_zero_host_runtime
        lofibox_zero_target_cli
        lofibox_zero_core
        PkgConfig::XKBCOMMON
    )
    if(TARGET lofibox_zero_tui)
        target_link_libraries(lofibox_zero_device PRIVATE lofibox_zero_tui)
        target_compile_definitions(lofibox_zero_device PRIVATE LOFIBOX_HAVE_TUI=1)
    endif()
    if(TARGET lofibox_webui)
        target_link_libraries(lofibox_zero_device PRIVATE lofibox_webui)
        target_compile_definitions(lofibox_zero_device PRIVATE LOFIBOX_HAVE_WEBUI=1)
    endif()
    install(TARGETS lofibox_zero_device
        RUNTIME DESTINATION "${LOFIBOX_PRIVATE_LIBDIR}"
    )
endif()

if(LOFIBOX_INSTALL_LAUNCHER)
    if(LOFIBOX_PRODUCT_PROFILE STREQUAL "cardputer")
        set(LOFIBOX_LAUNCHER data/lofibox-cardputer-launcher)
    elseif(LOFIBOX_PRODUCT_PROFILE STREQUAL "debian")
        set(LOFIBOX_LAUNCHER data/lofibox-debian-launcher)
    else()
        set(LOFIBOX_LAUNCHER data/lofibox-launcher)
    endif()
    install(PROGRAMS ${LOFIBOX_LAUNCHER}
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
        RENAME "${LOFIBOX_INSTALL_NAMESPACE}"
    )
endif()

install(FILES assets/README.md
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${LOFIBOX_INSTALL_NAMESPACE}/assets"
)

install(DIRECTORY assets/ui/icons/
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${LOFIBOX_INSTALL_NAMESPACE}/assets/ui/icons"
)

install(DIRECTORY data/plugins/
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${LOFIBOX_INSTALL_NAMESPACE}/plugins"
)

# Generated Python version module — single source of truth for Python helpers.
configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/version.py.in"
    "${CMAKE_BINARY_DIR}/generated/version.py"
    @ONLY
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated/man")
configure_file(
    "${PROJECT_SOURCE_DIR}/data/man/lofibox.1.in"
    "${CMAKE_BINARY_DIR}/generated/man/lofibox.1"
    @ONLY
)

install(FILES "${CMAKE_BINARY_DIR}/generated/version.py"
    DESTINATION "${LOFIBOX_PRIVATE_LIBDIR}"
)

install(FILES scripts/tag_writer.py scripts/remote_media_tool.py
    DESTINATION "${LOFIBOX_PRIVATE_LIBDIR}"
)

install(DIRECTORY src/remote/
    DESTINATION "${LOFIBOX_PRIVATE_LIBDIR}/remote"
    FILES_MATCHING
    PATTERN "*.py"
    PATTERN "__pycache__" EXCLUDE
)

if(NOT LOFIBOX_PRODUCT_PROFILE STREQUAL "cardputer")
    install(FILES data/io.github.vicliu624.lofibox.desktop
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
    )
endif()

if(TARGET lofibox_zero_device)
    install(PROGRAMS data/lofibox-cardputer-zero-applaunch
        DESTINATION "${LOFIBOX_PRIVATE_LIBDIR}"
        RENAME lofibox-applaunch
    )

    # APPLaunch metadata belongs to the appliance product, whose private
    # paths and namespace are fixed. Developer installs retain their generic
    # launcher and desktop entry instead of advertising an appliance path.
    if(LOFIBOX_PRODUCT_PROFILE STREQUAL "cardputer")
    install(FILES data/lofibox-cardputer-zero.desktop
        DESTINATION "${CMAKE_INSTALL_DATADIR}/APPLaunch/applications"
        RENAME "${LOFIBOX_INSTALL_NAMESPACE}.desktop"
    )

    install(FILES assets/ui/icons/cardputer-zero/lofibox.png
        DESTINATION "${CMAKE_INSTALL_DATADIR}/APPLaunch/share/images"
        RENAME "${LOFIBOX_INSTALL_NAMESPACE}.png"
    )
    endif()
endif()

if(NOT LOFIBOX_PRODUCT_PROFILE STREQUAL "cardputer")
install(FILES data/io.github.vicliu624.lofibox.metainfo.xml
    DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
)

install(FILES data/io.github.vicliu624.lofibox.svg
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
)

install(FILES assets/ui/icons/legacy-lofibox/logo.png
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/180x180/apps"
    RENAME io.github.vicliu624.lofibox.png
)

# The TDVP panel resolves ordinary hicolor raster directories reliably, while
# a manually deployed SVG can sit outside its icon search path.  Keep a
# standard-size PNG alongside the scalable and high-resolution projections so
# menu implementations that do not select SVG still find the desktop-entry
# icon by name.
install(FILES assets/ui/icons/cardputer-zero/lofibox.png
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/64x64/apps"
    RENAME io.github.vicliu624.lofibox.png
)

install(FILES data/mime/io.github.vicliu624.lofibox.xml
    DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages"
)
endif()

install(FILES "${CMAKE_BINARY_DIR}/generated/man/lofibox.1"
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
    RENAME "${LOFIBOX_INSTALL_NAMESPACE}.1"
)

if(LOFIBOX_INSTALL_DOCUMENTATION)
    install(DIRECTORY docs/
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    )

    install(FILES README.md
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    )

    install(FILES LICENSE COPYING
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    )
endif()

if(BUILD_TESTING)
    if(LOFIBOX_BROWSER_TESTS)
        if(NOT TARGET lofibox_zero_wayland OR NOT TARGET lofibox_webui)
            message(FATAL_ERROR "Browser tests require the Wayland and WebUI targets.")
        endif()
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        find_program(LOFIBOX_TEST_CHROMIUM NAMES chromium chromium-browser REQUIRED)
        find_program(LOFIBOX_TEST_WESTON NAMES weston REQUIRED)
        add_test(NAME lofibox_webui_browser_smoke
            COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/webui_browser_smoke.py"
                --player "$<TARGET_FILE:lofibox_zero_wayland>"
                --browser "${LOFIBOX_TEST_CHROMIUM}" --weston "${LOFIBOX_TEST_WESTON}"
                --output "${CMAKE_CURRENT_BINARY_DIR}/browser-results")
        set_tests_properties(lofibox_webui_browser_smoke PROPERTIES TIMEOUT 90)
    endif()
    add_executable(lofibox_library_rows_smoke tests/library_rows_smoke.cpp)
    target_link_libraries(lofibox_library_rows_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_library_rows_smoke COMMAND lofibox_library_rows_smoke)
    add_executable(lofibox_library_sort_smoke tests/library_sort_smoke.cpp)
    target_link_libraries(lofibox_library_sort_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_library_sort_smoke COMMAND lofibox_library_sort_smoke)
    add_executable(lofibox_library_external_access_smoke tests/library_external_access_smoke.cpp)
    target_link_libraries(lofibox_library_external_access_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_library_external_access_smoke COMMAND lofibox_library_external_access_smoke)
    set_tests_properties(lofibox_library_external_access_smoke PROPERTIES TIMEOUT 20)
    add_executable(lofibox_runtime_cache_concurrency_smoke tests/runtime_cache_concurrency_smoke.cpp)
    target_link_libraries(lofibox_runtime_cache_concurrency_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_runtime_cache_concurrency_smoke COMMAND lofibox_runtime_cache_concurrency_smoke)
    set_tests_properties(lofibox_runtime_cache_concurrency_smoke PROPERTIES TIMEOUT 15)
    add_executable(lofibox_gui_page_matrix_smoke tests/gui_page_matrix_smoke.cpp)
    target_link_libraries(lofibox_gui_page_matrix_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_gui_page_matrix_smoke COMMAND lofibox_gui_page_matrix_smoke)
    set_tests_properties(lofibox_gui_page_matrix_smoke PROPERTIES TIMEOUT 20)
    add_executable(lofibox_queue_playlist_interaction_smoke tests/queue_playlist_interaction_smoke.cpp)
    target_link_libraries(lofibox_queue_playlist_interaction_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_queue_playlist_interaction_smoke COMMAND lofibox_queue_playlist_interaction_smoke)
    add_executable(lofibox_playback_enrichment_worker_smoke tests/playback_enrichment_worker_smoke.cpp)
    target_link_libraries(lofibox_playback_enrichment_worker_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_playback_enrichment_worker_smoke COMMAND lofibox_playback_enrichment_worker_smoke)
    set_tests_properties(lofibox_playback_enrichment_worker_smoke PROPERTIES TIMEOUT 10)
    add_executable(lofibox_playback_start_responsiveness_smoke tests/playback_start_responsiveness_smoke.cpp)
    target_link_libraries(lofibox_playback_start_responsiveness_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_playback_start_responsiveness_smoke COMMAND lofibox_playback_start_responsiveness_smoke)
    set_tests_properties(lofibox_playback_start_responsiveness_smoke PROPERTIES TIMEOUT 15)
    add_executable(lofibox_frame_deadline_smoke tests/frame_deadline_smoke.cpp)
    target_include_directories(lofibox_frame_deadline_smoke PRIVATE "${PROJECT_SOURCE_DIR}/src")
    add_test(NAME lofibox_frame_deadline_smoke COMMAND lofibox_frame_deadline_smoke)

    add_executable(lofibox_canvas_copy_smoke tests/canvas_copy_smoke.cpp)
    target_link_libraries(lofibox_canvas_copy_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_canvas_copy_smoke COMMAND lofibox_canvas_copy_smoke)

    add_executable(lofibox_runtime_preferences_smoke tests/runtime_preferences_smoke.cpp)
    target_link_libraries(lofibox_runtime_preferences_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_runtime_preferences_smoke COMMAND lofibox_runtime_preferences_smoke)
    set_tests_properties(lofibox_runtime_preferences_smoke PROPERTIES TIMEOUT 10)

    add_executable(lofibox_app_smoke tests/hello_app_smoke.cpp)
    target_link_libraries(lofibox_app_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_app_smoke COMMAND lofibox_app_smoke)

    add_executable(lofibox_app_command_executor_smoke tests/app_command_executor_smoke.cpp)
    target_link_libraries(lofibox_app_command_executor_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_app_command_executor_smoke COMMAND lofibox_app_command_executor_smoke)

    add_executable(lofibox_application_command_boundary_smoke tests/application_command_boundary_smoke.cpp)
    target_link_libraries(lofibox_application_command_boundary_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_application_command_boundary_smoke COMMAND lofibox_application_command_boundary_smoke)

    add_executable(lofibox_runtime_command_bus_smoke tests/runtime_command_bus_smoke.cpp)
    target_link_libraries(lofibox_runtime_command_bus_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_runtime_command_bus_smoke COMMAND lofibox_runtime_command_bus_smoke)

    add_executable(lofibox_runtime_domains_smoke tests/runtime_domains_smoke.cpp)
    target_link_libraries(lofibox_runtime_domains_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_runtime_domains_smoke COMMAND lofibox_runtime_domains_smoke)

    add_executable(lofibox_runtime_client_server_smoke tests/runtime_client_server_smoke.cpp)
    target_link_libraries(lofibox_runtime_client_server_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_runtime_client_server_smoke COMMAND lofibox_runtime_client_server_smoke)

    add_executable(lofibox_runtime_host_smoke tests/runtime_host_smoke.cpp)
    target_link_libraries(lofibox_runtime_host_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_runtime_host_smoke COMMAND lofibox_runtime_host_smoke)

    add_executable(lofibox_runtime_transport_socket_smoke tests/runtime_transport_socket_smoke.cpp)
    target_link_libraries(lofibox_runtime_transport_socket_smoke PRIVATE lofibox_zero_core lofibox_zero_target_cli)
    add_test(NAME lofibox_runtime_transport_socket_smoke COMMAND lofibox_runtime_transport_socket_smoke)

    add_executable(lofibox_runtime_event_stream_smoke tests/runtime_event_stream_smoke.cpp)
    target_link_libraries(lofibox_runtime_event_stream_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_runtime_event_stream_smoke COMMAND lofibox_runtime_event_stream_smoke)

    add_executable(lofibox_runtime_cli_smoke tests/runtime_cli_smoke.cpp)
    target_link_libraries(lofibox_runtime_cli_smoke PRIVATE lofibox_zero_target_cli)
    add_test(NAME lofibox_runtime_cli_smoke COMMAND lofibox_runtime_cli_smoke)

    add_executable(lofibox_runtime_stream_playback_smoke tests/runtime_stream_playback_smoke.cpp)
    target_link_libraries(lofibox_runtime_stream_playback_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_runtime_stream_playback_smoke COMMAND lofibox_runtime_stream_playback_smoke)

    add_executable(lofibox_runtime_numeric_wire_smoke tests/runtime_numeric_wire_smoke.cpp)
    target_link_libraries(lofibox_runtime_numeric_wire_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_runtime_numeric_wire_smoke COMMAND lofibox_runtime_numeric_wire_smoke)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_executable(lofibox_runtime_play_input_smoke tests/runtime_play_input_smoke.cpp)
        target_link_libraries(lofibox_runtime_play_input_smoke PRIVATE lofibox_zero_target_cli)
        add_test(NAME lofibox_runtime_play_input_smoke COMMAND lofibox_runtime_play_input_smoke)
        set_tests_properties(lofibox_runtime_play_input_smoke PROPERTIES TIMEOUT 15)
    endif()

    add_executable(lofibox_cli_options_smoke tests/cli_options_smoke.cpp)
    target_link_libraries(lofibox_cli_options_smoke PRIVATE lofibox_zero_target_cli)
    add_test(NAME lofibox_cli_options_smoke COMMAND lofibox_cli_options_smoke)

    if(TARGET lofibox_zero_tui)
        add_executable(lofibox_tui_progress_bar_smoke tests/tui_progress_bar_smoke.cpp)
        target_link_libraries(lofibox_tui_progress_bar_smoke PRIVATE lofibox_zero_tui)
        add_test(NAME lofibox_tui_progress_bar_smoke COMMAND lofibox_tui_progress_bar_smoke)
        add_executable(lofibox_tui_timers_smoke tests/tui_timers_smoke.cpp)
        target_link_libraries(lofibox_tui_timers_smoke PRIVATE lofibox_zero_tui)
        add_test(NAME lofibox_tui_timers_smoke COMMAND lofibox_tui_timers_smoke)

        add_executable(lofibox_tui_spectrum_renderer_smoke tests/tui_spectrum_renderer_smoke.cpp)
        target_link_libraries(lofibox_tui_spectrum_renderer_smoke PRIVATE lofibox_zero_tui)
        add_test(NAME lofibox_tui_spectrum_renderer_smoke COMMAND lofibox_tui_spectrum_renderer_smoke)

        add_executable(lofibox_tui_lyrics_view_smoke tests/tui_lyrics_view_smoke.cpp)
        target_link_libraries(lofibox_tui_lyrics_view_smoke PRIVATE lofibox_zero_tui)
        add_test(NAME lofibox_tui_lyrics_view_smoke COMMAND lofibox_tui_lyrics_view_smoke)

        add_executable(lofibox_tui_layout_engine_smoke tests/tui_layout_engine_smoke.cpp)
        target_link_libraries(lofibox_tui_layout_engine_smoke PRIVATE lofibox_zero_tui)
        add_test(NAME lofibox_tui_layout_engine_smoke COMMAND lofibox_tui_layout_engine_smoke)

        add_executable(lofibox_tui_input_router_smoke tests/tui_input_router_smoke.cpp)
        target_link_libraries(lofibox_tui_input_router_smoke PRIVATE lofibox_zero_tui)
        add_test(NAME lofibox_tui_input_router_smoke COMMAND lofibox_tui_input_router_smoke)
        add_executable(lofibox_tui_utf8_input_smoke tests/tui_utf8_input_smoke.cpp)
        target_link_libraries(lofibox_tui_utf8_input_smoke PRIVATE lofibox_zero_tui)
        add_test(NAME lofibox_tui_utf8_input_smoke COMMAND lofibox_tui_utf8_input_smoke)

        add_executable(lofibox_tui_dashboard_render_smoke tests/tui_dashboard_render_smoke.cpp)
        target_link_libraries(lofibox_tui_dashboard_render_smoke PRIVATE lofibox_zero_tui)
        add_test(NAME lofibox_tui_dashboard_render_smoke COMMAND lofibox_tui_dashboard_render_smoke)

        add_executable(lofibox_tui_runtime_projection_smoke tests/tui_runtime_projection_smoke.cpp)
        target_link_libraries(lofibox_tui_runtime_projection_smoke PRIVATE lofibox_zero_tui)
        add_test(NAME lofibox_tui_runtime_projection_smoke COMMAND lofibox_tui_runtime_projection_smoke)

        add_executable(lofibox_tui_terminal_capabilities_smoke tests/tui_terminal_capabilities_smoke.cpp)
        target_link_libraries(lofibox_tui_terminal_capabilities_smoke PRIVATE lofibox_zero_tui)
        add_test(NAME lofibox_tui_terminal_capabilities_smoke COMMAND lofibox_tui_terminal_capabilities_smoke)
    endif()

    add_executable(lofibox_runtime_seek_stop_queueclear_smoke tests/runtime_seek_stop_queueclear_smoke.cpp)
    target_link_libraries(lofibox_runtime_seek_stop_queueclear_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_runtime_seek_stop_queueclear_smoke COMMAND lofibox_runtime_seek_stop_queueclear_smoke)

    add_executable(lofibox_runtime_settings_apply_smoke tests/runtime_settings_apply_smoke.cpp)
    target_link_libraries(lofibox_runtime_settings_apply_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_runtime_settings_apply_smoke COMMAND lofibox_runtime_settings_apply_smoke)
    add_executable(lofibox_session_timers_smoke tests/session_timers_smoke.cpp)
    target_link_libraries(lofibox_session_timers_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_session_timers_smoke COMMAND lofibox_session_timers_smoke)
    add_executable(lofibox_timer_gui_smoke tests/timer_gui_smoke.cpp)
    target_link_libraries(lofibox_timer_gui_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_timer_gui_smoke COMMAND lofibox_timer_gui_smoke)

    add_executable(lofibox_runtime_shutdown_reload_smoke tests/runtime_shutdown_reload_smoke.cpp)
    target_link_libraries(lofibox_runtime_shutdown_reload_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_runtime_shutdown_reload_smoke COMMAND lofibox_runtime_shutdown_reload_smoke)

    add_executable(lofibox_desktop_open_runtime_command_smoke tests/desktop_open_runtime_command_smoke.cpp)
    target_link_libraries(lofibox_desktop_open_runtime_command_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_desktop_open_runtime_command_smoke COMMAND lofibox_desktop_open_runtime_command_smoke)

    add_executable(lofibox_remote_browse_query_service_smoke tests/remote_browse_query_service_smoke.cpp)
    target_link_libraries(lofibox_remote_browse_query_service_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_remote_browse_query_service_smoke COMMAND lofibox_remote_browse_query_service_smoke)

    add_executable(lofibox_plugin_discovery_smoke tests/plugin_discovery_smoke.cpp)
    target_link_libraries(lofibox_plugin_discovery_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_plugin_discovery_smoke COMMAND lofibox_plugin_discovery_smoke)

    add_executable(lofibox_plugin_runtime_smoke tests/plugin_runtime_smoke.cpp)
    target_link_libraries(lofibox_plugin_runtime_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_plugin_runtime_smoke COMMAND lofibox_plugin_runtime_smoke)

    add_executable(lofibox_direct_cli_smoke tests/direct_cli_smoke.cpp)
    target_link_libraries(lofibox_direct_cli_smoke PRIVATE lofibox_zero_target_cli)
    add_test(NAME lofibox_direct_cli_smoke COMMAND lofibox_direct_cli_smoke)

    add_executable(lofibox_app_input_router_smoke tests/app_input_router_smoke.cpp)
    target_link_libraries(lofibox_app_input_router_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_app_input_router_smoke COMMAND lofibox_app_input_router_smoke)

    add_executable(lofibox_text_input_utils_smoke tests/text_input_utils_smoke.cpp)
    target_link_libraries(lofibox_text_input_utils_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_text_input_utils_smoke COMMAND lofibox_text_input_utils_smoke)

    add_executable(lofibox_app_lifecycle_smoke tests/app_lifecycle_smoke.cpp)
    target_link_libraries(lofibox_app_lifecycle_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_app_lifecycle_smoke COMMAND lofibox_app_lifecycle_smoke)

    add_executable(lofibox_app_page_model_smoke tests/app_page_model_smoke.cpp)
    target_link_libraries(lofibox_app_page_model_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_app_page_model_smoke COMMAND lofibox_app_page_model_smoke)

    add_executable(lofibox_library_scan_smoke tests/library_scan_smoke.cpp)
    target_link_libraries(lofibox_library_scan_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_library_scan_smoke COMMAND lofibox_library_scan_smoke)

    add_executable(lofibox_local_root_source_profile_smoke tests/local_root_source_profile_smoke.cpp)
    target_link_libraries(lofibox_local_root_source_profile_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_local_root_source_profile_smoke COMMAND lofibox_local_root_source_profile_smoke)

    add_executable(lofibox_library_controller_smoke tests/library_controller_smoke.cpp)
    target_link_libraries(lofibox_library_controller_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_library_controller_smoke COMMAND lofibox_library_controller_smoke)

    add_executable(lofibox_playback_controller_smoke tests/playback_controller_smoke.cpp)
    target_link_libraries(lofibox_playback_controller_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_playback_controller_smoke COMMAND lofibox_playback_controller_smoke)

    add_executable(lofibox_metadata_merge_policy_smoke tests/metadata_merge_policy_smoke.cpp)
    target_link_libraries(lofibox_metadata_merge_policy_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_metadata_merge_policy_smoke COMMAND lofibox_metadata_merge_policy_smoke)

    add_executable(lofibox_enrichment_authority_policy_smoke tests/enrichment_authority_policy_smoke.cpp)
    target_link_libraries(lofibox_enrichment_authority_policy_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_enrichment_authority_policy_smoke COMMAND lofibox_enrichment_authority_policy_smoke)

    add_executable(lofibox_library_repository_smoke tests/library_repository_smoke.cpp)
    target_link_libraries(lofibox_library_repository_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_library_repository_smoke COMMAND lofibox_library_repository_smoke)

    add_executable(lofibox_library_store_smoke tests/library_store_smoke.cpp)
    target_link_libraries(lofibox_library_store_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_library_store_smoke COMMAND lofibox_library_store_smoke)

    add_executable(lofibox_library_incremental_scan_smoke tests/library_incremental_scan_smoke.cpp)
    target_link_libraries(lofibox_library_incremental_scan_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_library_incremental_scan_smoke COMMAND lofibox_library_incremental_scan_smoke)

    add_executable(lofibox_media_search_queue_smoke tests/media_search_queue_smoke.cpp)
    target_link_libraries(lofibox_media_search_queue_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_media_search_queue_smoke COMMAND lofibox_media_search_queue_smoke)

    add_executable(lofibox_audio_pipeline_controller_smoke tests/audio_pipeline_controller_smoke.cpp)
    target_link_libraries(lofibox_audio_pipeline_controller_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_audio_pipeline_controller_smoke COMMAND lofibox_audio_pipeline_controller_smoke)

    add_executable(lofibox_audio_child_lifecycle_smoke tests/audio_child_lifecycle_smoke.cpp)
    target_link_libraries(lofibox_audio_child_lifecycle_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_audio_child_lifecycle_smoke COMMAND lofibox_audio_child_lifecycle_smoke)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(TARGET lofibox_mp3_decoder)
            find_package(Python3 COMPONENTS Interpreter REQUIRED)
            find_program(LOFIBOX_TEST_FFMPEG_EXECUTABLE ffmpeg)
            if(LOFIBOX_TEST_FFMPEG_EXECUTABLE)
                add_executable(lofibox_audio_pcm_contract_smoke tests/audio_pcm_contract_smoke.cpp)
                target_link_libraries(lofibox_audio_pcm_contract_smoke PRIVATE lofibox_zero_host_runtime)
                add_test(NAME lofibox_audio_pcm_contract_smoke COMMAND lofibox_audio_pcm_contract_smoke
                    "$<TARGET_FILE:lofibox_mp3_decoder>" "${LOFIBOX_TEST_FFMPEG_EXECUTABLE}")
                set_tests_properties(lofibox_audio_pcm_contract_smoke PROPERTIES TIMEOUT 25)
                add_test(NAME lofibox_native_mp3_helper_smoke COMMAND "${Python3_EXECUTABLE}"
                    "${PROJECT_SOURCE_DIR}/tests/native_mp3_helper_smoke.py"
                    "$<TARGET_FILE:lofibox_mp3_decoder>" "${LOFIBOX_TEST_FFMPEG_EXECUTABLE}")
                set_tests_properties(lofibox_native_mp3_helper_smoke PROPERTIES TIMEOUT 45)
            endif()
        endif()
        add_executable(lofibox_audio_restart_lifecycle_smoke tests/audio_restart_lifecycle_smoke.cpp)
        add_executable(lofibox_fd_ownership_smoke tests/fd_ownership_smoke.cpp src/platform/device/linux_framebuffer.cpp)
        target_link_libraries(lofibox_fd_ownership_smoke PRIVATE lofibox_zero_host_runtime)
        add_test(NAME lofibox_fd_ownership_smoke COMMAND lofibox_fd_ownership_smoke)
        set_tests_properties(lofibox_fd_ownership_smoke PROPERTIES TIMEOUT 20)
        add_executable(lofibox_runtime_socket_lifecycle_smoke tests/runtime_socket_lifecycle_smoke.cpp)
        target_link_libraries(lofibox_runtime_socket_lifecycle_smoke PRIVATE lofibox_zero_core)
        add_test(NAME lofibox_runtime_socket_lifecycle_smoke COMMAND lofibox_runtime_socket_lifecycle_smoke)
        set_tests_properties(lofibox_runtime_socket_lifecycle_smoke PROPERTIES TIMEOUT 15)
        target_link_libraries(lofibox_audio_restart_lifecycle_smoke PRIVATE lofibox_zero_host_runtime)
        add_test(NAME lofibox_audio_restart_lifecycle_smoke COMMAND lofibox_audio_restart_lifecycle_smoke)
        set_tests_properties(lofibox_audio_restart_lifecycle_smoke PROPERTIES TIMEOUT 25)
        add_executable(lofibox_tag_writer_capability_smoke tests/tag_writer_capability_smoke.cpp)
        target_link_libraries(lofibox_tag_writer_capability_smoke PRIVATE lofibox_zero_host_runtime)
        add_test(NAME lofibox_tag_writer_capability_smoke COMMAND lofibox_tag_writer_capability_smoke)

        add_executable(lofibox_runtime_process_smoke tests/runtime_process_smoke.cpp)
        target_link_libraries(lofibox_runtime_process_smoke PRIVATE lofibox_zero_host_runtime)
        add_test(NAME lofibox_runtime_process_smoke COMMAND lofibox_runtime_process_smoke)
        add_executable(lofibox_system_media_key_handler_smoke tests/system_media_key_handler_smoke.cpp)
        target_link_libraries(lofibox_system_media_key_handler_smoke PRIVATE lofibox_zero_host_runtime)
        add_test(NAME lofibox_system_media_key_handler_smoke COMMAND lofibox_system_media_key_handler_smoke)
        set_tests_properties(lofibox_system_media_key_handler_smoke PROPERTIES TIMEOUT 10)
        add_executable(lofibox_cardputer_rhythm_lights_smoke
            tests/cardputer_rhythm_lights_smoke.cpp
            src/platform/device/cardputer_rhythm_lights.cpp
        )
        target_link_libraries(lofibox_cardputer_rhythm_lights_smoke PRIVATE lofibox_zero_core)
        add_test(NAME lofibox_cardputer_rhythm_lights_smoke COMMAND lofibox_cardputer_rhythm_lights_smoke)
        set_tests_properties(lofibox_cardputer_rhythm_lights_smoke PROPERTIES TIMEOUT 10)
    endif()

    add_executable(lofibox_host_audio_realtime_dsp_smoke tests/host_audio_realtime_dsp_smoke.cpp)
    target_link_libraries(lofibox_host_audio_realtime_dsp_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_host_audio_realtime_dsp_smoke COMMAND lofibox_host_audio_realtime_dsp_smoke)

    add_executable(lofibox_equalizer_smoke tests/equalizer_smoke.cpp)
    target_link_libraries(lofibox_equalizer_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_equalizer_smoke COMMAND lofibox_equalizer_smoke)

    add_executable(lofibox_dsp_chain_smoke tests/dsp_chain_smoke.cpp)
    target_link_libraries(lofibox_dsp_chain_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_dsp_chain_smoke COMMAND lofibox_dsp_chain_smoke)

    add_executable(lofibox_realtime_dsp_engine_smoke tests/realtime_dsp_engine_smoke.cpp)
    target_link_libraries(lofibox_realtime_dsp_engine_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_realtime_dsp_engine_smoke COMMAND lofibox_realtime_dsp_engine_smoke)

    add_executable(lofibox_dsp_profile_manager_smoke tests/dsp_profile_manager_smoke.cpp)
    target_link_libraries(lofibox_dsp_profile_manager_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_dsp_profile_manager_smoke COMMAND lofibox_dsp_profile_manager_smoke)

    add_executable(lofibox_cache_manager_smoke tests/cache_manager_smoke.cpp)
    target_link_libraries(lofibox_cache_manager_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_cache_manager_smoke COMMAND lofibox_cache_manager_smoke)

    add_executable(lofibox_streaming_completion_smoke tests/streaming_completion_smoke.cpp)
    target_link_libraries(lofibox_streaming_completion_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_streaming_completion_smoke COMMAND lofibox_streaming_completion_smoke)

    add_executable(lofibox_remote_browse_playback_flow_smoke tests/remote_browse_playback_flow_smoke.cpp)
    target_link_libraries(lofibox_remote_browse_playback_flow_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_remote_browse_playback_flow_smoke COMMAND lofibox_remote_browse_playback_flow_smoke)

    add_executable(lofibox_library_governance_smoke tests/library_governance_smoke.cpp)
    target_link_libraries(lofibox_library_governance_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_library_governance_smoke COMMAND lofibox_library_governance_smoke)

    add_executable(lofibox_metadata_governance_smoke tests/metadata_governance_smoke.cpp)
    target_link_libraries(lofibox_metadata_governance_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_metadata_governance_smoke COMMAND lofibox_metadata_governance_smoke)

    add_executable(lofibox_playback_stability_smoke tests/playback_stability_smoke.cpp)
    target_link_libraries(lofibox_playback_stability_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_playback_stability_smoke COMMAND lofibox_playback_stability_smoke)

    add_executable(lofibox_desktop_security_completion_smoke tests/desktop_security_completion_smoke.cpp)
    target_link_libraries(lofibox_desktop_security_completion_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_desktop_security_completion_smoke COMMAND lofibox_desktop_security_completion_smoke)

    add_executable(lofibox_cardputer_applaunch_metadata_smoke tests/cardputer_applaunch_metadata_smoke.cpp)
    target_compile_definitions(lofibox_cardputer_applaunch_metadata_smoke PRIVATE LOFIBOX_PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    add_test(NAME lofibox_cardputer_applaunch_metadata_smoke COMMAND lofibox_cardputer_applaunch_metadata_smoke)

    if(LOFIBOX_BUILD_DEVICE)
        add_executable(lofibox_framebuffer_backlight_smoke tests/framebuffer_backlight_smoke.cpp)
        target_link_libraries(lofibox_framebuffer_backlight_smoke PRIVATE lofibox_zero_core)
        add_test(NAME lofibox_framebuffer_backlight_smoke COMMAND lofibox_framebuffer_backlight_smoke)
        add_executable(lofibox_framebuffer_blitter_smoke
            tests/framebuffer_blitter_smoke.cpp
            src/platform/device/framebuffer_blitter.cpp
        )
        target_link_libraries(lofibox_framebuffer_blitter_smoke PRIVATE lofibox_zero_core)
        add_test(NAME lofibox_framebuffer_blitter_smoke COMMAND lofibox_framebuffer_blitter_smoke)

        add_executable(lofibox_linux_evdev_keyboard_smoke
            tests/linux_evdev_keyboard_smoke.cpp
            src/platform/device/linux_evdev_keyboard.cpp
        )
        target_link_libraries(lofibox_linux_evdev_keyboard_smoke PRIVATE lofibox_zero_core PkgConfig::XKBCOMMON)
        add_test(NAME lofibox_linux_evdev_keyboard_smoke COMMAND lofibox_linux_evdev_keyboard_smoke)
    endif()

    add_executable(lofibox_packaging_version_metadata_smoke tests/packaging_version_metadata_smoke.cpp)
    target_compile_definitions(lofibox_packaging_version_metadata_smoke PRIVATE
        LOFIBOX_PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
        LOFIBOX_PROJECT_VERSION="${PROJECT_VERSION}"
    )
    add_test(NAME lofibox_packaging_version_metadata_smoke COMMAND lofibox_packaging_version_metadata_smoke)

    add_executable(lofibox_final_surface_pages_smoke tests/final_surface_pages_smoke.cpp)
    target_link_libraries(lofibox_final_surface_pages_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_final_surface_pages_smoke COMMAND lofibox_final_surface_pages_smoke)

    add_executable(lofibox_main_menu_visual_smoke tests/main_menu_visual_smoke.cpp)
    target_link_libraries(lofibox_main_menu_visual_smoke PRIVATE lofibox_zero_host_runtime)
    target_compile_definitions(lofibox_main_menu_visual_smoke PRIVATE LOFIBOX_ASSET_DIR="${PROJECT_SOURCE_DIR}/assets")
    add_test(NAME lofibox_main_menu_visual_smoke COMMAND lofibox_main_menu_visual_smoke)

    add_executable(lofibox_logo_asset_transparency_smoke tests/logo_asset_transparency_smoke.cpp)
    target_link_libraries(lofibox_logo_asset_transparency_smoke PRIVATE lofibox_zero_host_runtime)
    target_compile_definitions(lofibox_logo_asset_transparency_smoke PRIVATE LOFIBOX_ASSET_DIR="${PROJECT_SOURCE_DIR}/assets")
    add_test(NAME lofibox_logo_asset_transparency_smoke COMMAND lofibox_logo_asset_transparency_smoke)

    add_executable(lofibox_main_menu_shortcuts_smoke tests/main_menu_shortcuts_smoke.cpp)
    target_link_libraries(lofibox_main_menu_shortcuts_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_main_menu_shortcuts_smoke COMMAND lofibox_main_menu_shortcuts_smoke)

    add_executable(lofibox_list_focus_visual_smoke tests/list_focus_visual_smoke.cpp)
    target_link_libraries(lofibox_list_focus_visual_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_list_focus_visual_smoke COMMAND lofibox_list_focus_visual_smoke)

    add_executable(lofibox_list_help_modal_smoke tests/list_help_modal_smoke.cpp)
    target_link_libraries(lofibox_list_help_modal_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_list_help_modal_smoke COMMAND lofibox_list_help_modal_smoke)

    add_executable(lofibox_network_settings_smoke tests/network_settings_smoke.cpp)
    target_link_libraries(lofibox_network_settings_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_network_settings_smoke COMMAND lofibox_network_settings_smoke)

    add_executable(lofibox_projection_desktop_boundary_smoke tests/projection_desktop_boundary_smoke.cpp)
    target_link_libraries(lofibox_projection_desktop_boundary_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_projection_desktop_boundary_smoke COMMAND lofibox_projection_desktop_boundary_smoke)

    add_executable(lofibox_source_manager_projection_smoke tests/source_manager_projection_smoke.cpp)
    target_link_libraries(lofibox_source_manager_projection_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_source_manager_projection_smoke COMMAND lofibox_source_manager_projection_smoke)

    add_executable(lofibox_embedded_metadata_smoke tests/embedded_metadata_smoke.cpp)
    target_link_libraries(lofibox_embedded_metadata_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_embedded_metadata_smoke COMMAND lofibox_embedded_metadata_smoke)

    add_executable(lofibox_embedded_artwork_smoke tests/embedded_artwork_smoke.cpp)
    target_link_libraries(lofibox_embedded_artwork_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_embedded_artwork_smoke COMMAND lofibox_embedded_artwork_smoke)

    add_executable(lofibox_now_playing_artwork_smoke tests/now_playing_artwork_smoke.cpp)
    target_link_libraries(lofibox_now_playing_artwork_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_now_playing_artwork_smoke COMMAND lofibox_now_playing_artwork_smoke)

    add_executable(lofibox_now_playing_tidal_spectrum_smoke tests/now_playing_tidal_spectrum_smoke.cpp)
    target_link_libraries(lofibox_now_playing_tidal_spectrum_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_now_playing_tidal_spectrum_smoke COMMAND lofibox_now_playing_tidal_spectrum_smoke)

    add_executable(lofibox_lyrics_page_smoke tests/lyrics_page_smoke.cpp)
    target_link_libraries(lofibox_lyrics_page_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_lyrics_page_smoke COMMAND lofibox_lyrics_page_smoke)

    add_executable(lofibox_root_directory_artwork_guard_smoke tests/root_directory_artwork_guard_smoke.cpp)
    target_link_libraries(lofibox_root_directory_artwork_guard_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_root_directory_artwork_guard_smoke COMMAND lofibox_root_directory_artwork_guard_smoke)

    add_executable(lofibox_bitmap_font_utf8_smoke tests/bitmap_font_utf8_smoke.cpp)
    target_link_libraries(lofibox_bitmap_font_utf8_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_bitmap_font_utf8_smoke COMMAND lofibox_bitmap_font_utf8_smoke)

    add_executable(lofibox_tag_writeback_smoke tests/tag_writeback_smoke.cpp)
    target_link_libraries(lofibox_tag_writeback_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_tag_writeback_smoke COMMAND lofibox_tag_writeback_smoke)

    add_executable(lofibox_lyrics_writeback_policy_smoke tests/lyrics_writeback_policy_smoke.cpp)
    target_link_libraries(lofibox_lyrics_writeback_policy_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_lyrics_writeback_policy_smoke COMMAND lofibox_lyrics_writeback_policy_smoke)

    add_executable(lofibox_single_instance_lock_smoke tests/single_instance_lock_smoke.cpp)
    target_link_libraries(lofibox_single_instance_lock_smoke PRIVATE lofibox_zero_host_lock)
    add_test(NAME lofibox_single_instance_lock_smoke COMMAND lofibox_single_instance_lock_smoke)
    set_tests_properties(lofibox_single_instance_lock_smoke PROPERTIES TIMEOUT 10)

    add_executable(lofibox_remote_media_smoke tests/remote_media_smoke.cpp)
    target_link_libraries(lofibox_remote_media_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_remote_media_smoke COMMAND lofibox_remote_media_smoke)

    add_executable(lofibox_real_emby_integration_smoke tests/real_emby_integration_smoke.cpp)
    target_link_libraries(lofibox_real_emby_integration_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_real_emby_integration_smoke COMMAND lofibox_real_emby_integration_smoke)

    add_executable(lofibox_real_remote_media_integration_smoke tests/real_remote_media_integration_smoke.cpp)
    target_link_libraries(lofibox_real_remote_media_integration_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_real_remote_media_integration_smoke COMMAND lofibox_real_remote_media_integration_smoke)

    add_executable(lofibox_real_remote_lab_integration_smoke tests/real_remote_lab_integration_smoke.cpp)
    target_link_libraries(lofibox_real_remote_lab_integration_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_real_remote_lab_integration_smoke COMMAND lofibox_real_remote_lab_integration_smoke)

    add_executable(lofibox_remote_source_security_smoke tests/remote_source_security_smoke.cpp)
    target_link_libraries(lofibox_remote_source_security_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_remote_source_security_smoke COMMAND lofibox_remote_source_security_smoke)

    add_executable(lofibox_remote_profile_store_smoke tests/remote_profile_store_smoke.cpp)
    target_link_libraries(lofibox_remote_profile_store_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_remote_profile_store_smoke COMMAND lofibox_remote_profile_store_smoke)

    add_executable(lofibox_runtime_service_provider_smoke tests/runtime_service_provider_smoke.cpp)
    target_link_libraries(lofibox_runtime_service_provider_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_runtime_service_provider_smoke COMMAND lofibox_runtime_service_provider_smoke)

    add_executable(lofibox_filename_metadata_guess_smoke tests/filename_metadata_guess_smoke.cpp)
    target_link_libraries(lofibox_filename_metadata_guess_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_filename_metadata_guess_smoke COMMAND lofibox_filename_metadata_guess_smoke)

    add_executable(lofibox_lrclib_search_fallback_smoke tests/lrclib_search_fallback_smoke.cpp)
    target_link_libraries(lofibox_lrclib_search_fallback_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_lrclib_search_fallback_smoke COMMAND lofibox_lrclib_search_fallback_smoke)

    add_executable(lofibox_xdg_paths_smoke tests/xdg_paths_smoke.cpp)
    target_link_libraries(lofibox_xdg_paths_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_xdg_paths_smoke COMMAND lofibox_xdg_paths_smoke)
    set_tests_properties(lofibox_xdg_paths_smoke PROPERTIES
        ENVIRONMENT "XDG_CONFIG_HOME=/tmp/lofibox-xdg-config;XDG_DATA_HOME=/tmp/lofibox-xdg-data;XDG_CACHE_HOME=/tmp/lofibox-xdg-cache;XDG_STATE_HOME=/tmp/lofibox-xdg-state;XDG_RUNTIME_DIR=/tmp/lofibox-xdg-runtime"
    )

    add_executable(lofibox_helper_script_path_smoke tests/helper_script_path_smoke.cpp)
    target_link_libraries(lofibox_helper_script_path_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_helper_script_path_smoke COMMAND lofibox_helper_script_path_smoke)

    if(TARGET lofibox_webui)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            add_executable(lofibox_optional_interfaces_smoke tests/optional_interfaces_smoke.cpp)
            target_link_libraries(lofibox_optional_interfaces_smoke PRIVATE lofibox_webui lofibox_zero_host_runtime)
            add_test(NAME lofibox_optional_interfaces_smoke COMMAND lofibox_optional_interfaces_smoke)
            set_tests_properties(lofibox_optional_interfaces_smoke PROPERTIES TIMEOUT 45)
            add_executable(lofibox_offline_enrichment_smoke tests/offline_enrichment_smoke.cpp)
            target_link_libraries(lofibox_offline_enrichment_smoke PRIVATE lofibox_zero_host_runtime)
            add_test(NAME lofibox_offline_enrichment_smoke COMMAND lofibox_offline_enrichment_smoke)
            set_tests_properties(lofibox_offline_enrichment_smoke PROPERTIES TIMEOUT 12)
            add_executable(lofibox_offline_embedded_lyrics_smoke tests/offline_embedded_lyrics_smoke.cpp)
            target_link_libraries(lofibox_offline_embedded_lyrics_smoke PRIVATE lofibox_zero_host_runtime)
            add_test(NAME lofibox_offline_embedded_lyrics_smoke COMMAND lofibox_offline_embedded_lyrics_smoke)
            set_tests_properties(lofibox_offline_embedded_lyrics_smoke PROPERTIES TIMEOUT 10)
        endif()
        add_executable(lofibox_webui_smoke tests/webui_smoke.cpp)
        target_link_libraries(lofibox_webui_smoke PRIVATE lofibox_webui)
        add_test(NAME lofibox_webui_smoke COMMAND lofibox_webui_smoke)
        add_executable(lofibox_webui_server_smoke tests/webui_server_smoke.cpp)
        target_link_libraries(lofibox_webui_server_smoke PRIVATE lofibox_webui lofibox_zero_host_runtime)
        add_test(NAME lofibox_webui_server_smoke COMMAND lofibox_webui_server_smoke)
        set_tests_properties(lofibox_webui_server_smoke PROPERTIES TIMEOUT 20)
    endif()

    # Release tests must execute their assertions; only the product libraries
    # should inherit NDEBUG from the optimized build configuration.
    get_property(_lofibox_test_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(_lofibox_test_target IN LISTS _lofibox_test_targets)
        if(_lofibox_test_target MATCHES "_smoke$")
            if(MSVC)
                target_compile_options(${_lofibox_test_target} PRIVATE /UNDEBUG)
            else()
                target_compile_options(${_lofibox_test_target} PRIVATE -UNDEBUG)
            endif()
        endif()
    endforeach()
  endif()
