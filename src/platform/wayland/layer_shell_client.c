// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/wayland/layer_shell_client.h"

/*
 * The wlroots v4 protocol uses an argument literally named `namespace`.
 * wayland-scanner intentionally preserves that spelling in the C header, so
 * C++ cannot include it. Keep the generated API at this narrow C boundary;
 * WidgetPresenter consumes only the C++-safe wrapper declared above.
 */
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

static void lofibox_layer_surface_configure(
    void* data,
    struct zwlr_layer_surface_v1* surface,
    uint32_t serial,
    uint32_t width,
    uint32_t height)
{
    const struct lofibox_layer_surface_callbacks* callbacks = data;

    (void)surface;
    if (callbacks != NULL && callbacks->configure != NULL)
        callbacks->configure(callbacks->data, serial, width, height);
}

static void lofibox_layer_surface_closed(
    void* data,
    struct zwlr_layer_surface_v1* surface)
{
    const struct lofibox_layer_surface_callbacks* callbacks = data;

    (void)surface;
    if (callbacks != NULL && callbacks->closed != NULL)
        callbacks->closed(callbacks->data);
}

static const struct zwlr_layer_surface_v1_listener lofibox_layer_surface_listener = {
    .configure = lofibox_layer_surface_configure,
    .closed = lofibox_layer_surface_closed,
};

struct zwlr_layer_shell_v1* lofibox_layer_shell_bind(
    struct wl_registry* registry,
    uint32_t name,
    uint32_t advertised_version)
{
    const uint32_t version = advertised_version < 4U ? advertised_version : 4U;

    return wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, version);
}

void lofibox_layer_shell_destroy(struct zwlr_layer_shell_v1* layer_shell)
{
    if (layer_shell == NULL)
        return;

    if (wl_proxy_get_version((struct wl_proxy*)layer_shell) >= 3U)
        zwlr_layer_shell_v1_destroy(layer_shell);
    else
        wl_proxy_destroy((struct wl_proxy*)layer_shell);
}

struct zwlr_layer_surface_v1* lofibox_layer_shell_get_surface(
    struct zwlr_layer_shell_v1* layer_shell,
    struct wl_surface* surface,
    struct wl_output* output,
    uint32_t layer,
    const char* surface_namespace,
    const struct lofibox_layer_surface_callbacks* callbacks)
{
    struct zwlr_layer_surface_v1* layer_surface;

    if (layer_shell == NULL || surface == NULL || surface_namespace == NULL)
        return NULL;

    layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        layer_shell, surface, output, layer, surface_namespace);
    if (layer_surface != NULL && callbacks != NULL)
        zwlr_layer_surface_v1_add_listener(layer_surface,
                                           &lofibox_layer_surface_listener,
                                           (void*)callbacks);
    return layer_surface;
}

void lofibox_layer_surface_set_size(
    struct zwlr_layer_surface_v1* surface,
    uint32_t width,
    uint32_t height)
{
    zwlr_layer_surface_v1_set_size(surface, width, height);
}

void lofibox_layer_surface_set_anchor(
    struct zwlr_layer_surface_v1* surface,
    uint32_t anchor)
{
    zwlr_layer_surface_v1_set_anchor(surface, anchor);
}

void lofibox_layer_surface_set_exclusive_zone(
    struct zwlr_layer_surface_v1* surface,
    int32_t zone)
{
    zwlr_layer_surface_v1_set_exclusive_zone(surface, zone);
}

void lofibox_layer_surface_set_margin(
    struct zwlr_layer_surface_v1* surface,
    int32_t top,
    int32_t right,
    int32_t bottom,
    int32_t left)
{
    zwlr_layer_surface_v1_set_margin(surface, top, right, bottom, left);
}

void lofibox_layer_surface_set_keyboard_interactivity(
    struct zwlr_layer_surface_v1* surface,
    uint32_t keyboard_interactivity)
{
    zwlr_layer_surface_v1_set_keyboard_interactivity(surface, keyboard_interactivity);
}

void lofibox_layer_surface_ack_configure(
    struct zwlr_layer_surface_v1* surface,
    uint32_t serial)
{
    zwlr_layer_surface_v1_ack_configure(surface, serial);
}

void lofibox_layer_surface_destroy(struct zwlr_layer_surface_v1* surface)
{
    if (surface != NULL)
        zwlr_layer_surface_v1_destroy(surface);
}
