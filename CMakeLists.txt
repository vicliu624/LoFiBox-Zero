# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.24)

project(LoFiBoxZero VERSION 0.1.0 LANGUAGES C CXX)

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

include(GNUInstallDirs)
include(CTest)

set(LOFIBOX_PRIVATE_LIBDIR "lib/lofibox" CACHE PATH "Private LoFiBox runtime directory")
set(CMAKE_INSTALL_DOCDIR "${CMAKE_INSTALL_DATAROOTDIR}/doc/lofibox")

set(LOFIBOX_BUILD_DEVICE_DEFAULT OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LOFIBOX_BUILD_DEVICE_DEFAULT ON)
endif()

option(LOFIBOX_BUILD_DEVICE "Build the Linux framebuffer device target." ${LOFIBOX_BUILD_DEVICE_DEFAULT})
option(LOFIBOX_BUILD_X11 "Build the Linux X11 VNC/PocketFrame target." ${LOFIBOX_BUILD_DEVICE_DEFAULT})

add_library(lofibox_zero_core STATIC
    src/app/input_actions.cpp
    src/app/library_controller.cpp
    src/app/library_scanner.cpp
    src/app/lofibox_app.cpp
    src/app/lofibox_app_runner.cpp
    src/ui/pages/about_page.cpp
    src/ui/pages/equalizer_page.cpp
    src/ui/pages/list_page.cpp
    src/ui/pages/lyrics_page.cpp
    src/ui/pages/main_menu_page.cpp
    src/ui/pages/now_playing_page.cpp
    src/app/playback_controller.cpp
    src/app/runtime_services.cpp
    src/ui/ui_primitives.cpp
    src/core/bitmap_font.cpp
    src/core/canvas.cpp
)

target_include_directories(lofibox_zero_core PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_features(lofibox_zero_core PUBLIC cxx_std_20)
target_compile_definitions(lofibox_zero_core PUBLIC
    LOFIBOX_ASSET_DIR="${PROJECT_SOURCE_DIR}/assets"
    LOFIBOX_SYSTEM_ASSET_DIR="${CMAKE_INSTALL_FULL_DATADIR}/lofibox/assets"
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

add_library(lofibox_zero_host_runtime STATIC
    src/platform/host/audio_playback_backend.cpp
    src/platform/host/artwork_provider.cpp
    src/platform/host/connectivity_provider.cpp
    src/platform/host/lyrics_provider.cpp
    src/platform/host/legacy_asset_loader.cpp
    src/platform/host/metadata_provider.cpp
    src/platform/host/png_canvas_loader.cpp
    src/platform/host/remote_media_runtime.cpp
    src/platform/host/runtime_cache_store.cpp
    src/platform/host/runtime_enrichment_clients.cpp
    src/platform/host/runtime_host_internal.cpp
    src/platform/host/runtime_logger.cpp
    src/platform/host/runtime_paths.cpp
    src/platform/host/runtime_process.cpp
    src/platform/host/runtime_remote_media_tool.cpp
    src/platform/host/runtime_script_bridge.cpp
    src/platform/host/runtime_services_factory.cpp
    src/platform/host/single_instance_lock.cpp
    src/platform/host/tag_writer.cpp
    src/platform/host/track_identity_provider.cpp
)
target_include_directories(lofibox_zero_host_runtime PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_features(lofibox_zero_host_runtime PUBLIC cxx_std_20)
target_link_libraries(lofibox_zero_host_runtime PUBLIC lofibox_zero_core)
target_compile_definitions(lofibox_zero_host_runtime PRIVATE
    LOFIBOX_PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    LOFIBOX_INSTALLED_HELPER_DIR="${CMAKE_INSTALL_PREFIX}/${LOFIBOX_PRIVATE_LIBDIR}"
)
if(MSVC)
    target_compile_options(lofibox_zero_host_runtime PRIVATE /W4 /permissive- /FS)
else()
    target_compile_options(lofibox_zero_host_runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(lofibox_zero_target_cli STATIC
    src/targets/cli_options.cpp
)
target_include_directories(lofibox_zero_target_cli PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_features(lofibox_zero_target_cli PUBLIC cxx_std_20)
target_compile_definitions(lofibox_zero_target_cli PUBLIC LOFIBOX_VERSION=\"${PROJECT_VERSION}\")
if(MSVC)
    target_compile_options(lofibox_zero_target_cli PRIVATE /W4 /permissive- /FS)
else()
    target_compile_options(lofibox_zero_target_cli PRIVATE -Wall -Wextra -Wpedantic)
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
    set_target_properties(lofibox_zero_x11 PROPERTIES OUTPUT_NAME lofibox)
    target_link_libraries(lofibox_zero_x11 PRIVATE
        lofibox_zero_host_runtime
        lofibox_zero_target_cli
        X11::X11
    )
    install(TARGETS lofibox_zero_x11
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()

if(LOFIBOX_BUILD_DEVICE)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(FATAL_ERROR "LOFIBOX_BUILD_DEVICE requires a Linux target environment.")
    endif()

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)

    add_executable(lofibox_zero_device
        src/platform/device/linux_evdev_keyboard.cpp
        src/platform/device/linux_framebuffer.cpp
        src/platform/device/linux_framebuffer_platform.cpp
        src/targets/device_main.cpp
    )
    target_link_libraries(lofibox_zero_device PRIVATE
        lofibox_zero_host_runtime
        lofibox_zero_target_cli
        PkgConfig::XKBCOMMON
    )
    if(TARGET lofibox_zero_x11)
        install(TARGETS lofibox_zero_device
            RUNTIME DESTINATION "${LOFIBOX_PRIVATE_LIBDIR}"
        )
    else()
        set_target_properties(lofibox_zero_device PROPERTIES OUTPUT_NAME lofibox)
        install(TARGETS lofibox_zero_device
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        )
    endif()
endif()

install(FILES assets/README.md
    DESTINATION "${CMAKE_INSTALL_DATADIR}/lofibox/assets"
)

install(DIRECTORY assets/ui/icons/
    DESTINATION "${CMAKE_INSTALL_DATADIR}/lofibox/assets/ui/icons"
)

install(FILES scripts/tag_writer.py scripts/remote_media_tool.py
    DESTINATION "${LOFIBOX_PRIVATE_LIBDIR}"
)

install(FILES data/io.github.vicliu624.lofibox.desktop
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

install(FILES data/io.github.vicliu624.lofibox.metainfo.xml
    DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
)

install(FILES data/io.github.vicliu624.lofibox.svg
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
)

install(FILES data/mime/io.github.vicliu624.lofibox.xml
    DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages"
)

install(FILES data/man/lofibox.1
    DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
)

install(DIRECTORY docs/
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)

install(FILES README.md
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)

install(FILES LICENSE COPYING
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)

if(BUILD_TESTING)
    add_executable(lofibox_app_smoke tests/hello_app_smoke.cpp)
    target_link_libraries(lofibox_app_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_app_smoke COMMAND lofibox_app_smoke)

    add_executable(lofibox_library_scan_smoke tests/library_scan_smoke.cpp)
    target_link_libraries(lofibox_library_scan_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_library_scan_smoke COMMAND lofibox_library_scan_smoke)

    add_executable(lofibox_library_controller_smoke tests/library_controller_smoke.cpp)
    target_link_libraries(lofibox_library_controller_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_library_controller_smoke COMMAND lofibox_library_controller_smoke)

    add_executable(lofibox_playback_controller_smoke tests/playback_controller_smoke.cpp)
    target_link_libraries(lofibox_playback_controller_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_playback_controller_smoke COMMAND lofibox_playback_controller_smoke)

    add_executable(lofibox_equalizer_smoke tests/equalizer_smoke.cpp)
    target_link_libraries(lofibox_equalizer_smoke PRIVATE lofibox_zero_core)
    add_test(NAME lofibox_equalizer_smoke COMMAND lofibox_equalizer_smoke)

    add_executable(lofibox_main_menu_visual_smoke tests/main_menu_visual_smoke.cpp)
    target_link_libraries(lofibox_main_menu_visual_smoke PRIVATE lofibox_zero_host_runtime)
    target_compile_definitions(lofibox_main_menu_visual_smoke PRIVATE LOFIBOX_ASSET_DIR="${PROJECT_SOURCE_DIR}/assets")
    add_test(NAME lofibox_main_menu_visual_smoke COMMAND lofibox_main_menu_visual_smoke)

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

    add_executable(lofibox_single_instance_lock_smoke tests/single_instance_lock_smoke.cpp)
    target_link_libraries(lofibox_single_instance_lock_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_single_instance_lock_smoke COMMAND lofibox_single_instance_lock_smoke)

    add_executable(lofibox_remote_media_smoke tests/remote_media_smoke.cpp)
    target_link_libraries(lofibox_remote_media_smoke PRIVATE lofibox_zero_host_runtime)
    add_test(NAME lofibox_remote_media_smoke COMMAND lofibox_remote_media_smoke)

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
  endif()
