// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>

#include <wayland-client.h>

struct zwlr_layer_shell_v1;
struct zwlr_layer_surface_v1;

#ifdef __cplusplus
extern "C" {
#endif

struct lofibox_layer_surface_callbacks {
    void* data;
    void (*configure)(void* data, uint32_t serial, uint32_t width, uint32_t height);
    void (*closed)(void* data);
};

struct zwlr_layer_shell_v1* lofibox_layer_shell_bind(
    struct wl_registry* registry,
    uint32_t name,
    uint32_t advertised_version);
void lofibox_layer_shell_destroy(struct zwlr_layer_shell_v1* layer_shell);

struct zwlr_layer_surface_v1* lofibox_layer_shell_get_surface(
    struct zwlr_layer_shell_v1* layer_shell,
    struct wl_surface* surface,
    struct wl_output* output,
    uint32_t layer,
    const char* surface_namespace,
    const struct lofibox_layer_surface_callbacks* callbacks);
void lofibox_layer_surface_set_size(struct zwlr_layer_surface_v1* surface, uint32_t width, uint32_t height);
void lofibox_layer_surface_set_anchor(struct zwlr_layer_surface_v1* surface, uint32_t anchor);
void lofibox_layer_surface_set_exclusive_zone(struct zwlr_layer_surface_v1* surface, int32_t zone);
void lofibox_layer_surface_set_margin(
    struct zwlr_layer_surface_v1* surface,
    int32_t top,
    int32_t right,
    int32_t bottom,
    int32_t left);
void lofibox_layer_surface_set_keyboard_interactivity(
    struct zwlr_layer_surface_v1* surface,
    uint32_t keyboard_interactivity);
void lofibox_layer_surface_ack_configure(struct zwlr_layer_surface_v1* surface, uint32_t serial);
void lofibox_layer_surface_destroy(struct zwlr_layer_surface_v1* surface);

#ifdef __cplusplus
} // extern "C"
#endif
