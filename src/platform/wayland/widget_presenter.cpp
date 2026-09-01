// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/wayland/widget_presenter.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "core/display_profile.h"
#include "platform/wayland/layer_shell_client.h"

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif

namespace lofibox::platform::wayland {
namespace {

constexpr int kWidth = core::kDisplayWidth;
constexpr int kHeight = core::kDisplayHeight;
constexpr int kStride = kWidth * 4;
constexpr int kBufferSize = kStride * kHeight;
constexpr std::uint32_t kPrimaryPointerButton = 0x110U; // BTN_LEFT
constexpr std::uint32_t kLayerTop = 2U;
constexpr std::uint32_t kAnchorBottom = 2U;
constexpr std::uint32_t kAnchorRight = 8U;
constexpr std::uint32_t kKeyboardInteractivityNone = 0U;
constexpr std::uint32_t kKeyboardInteractivityOnDemand = 2U;
constexpr int kDefaultWidgetMargin = 16;
constexpr int kMaximumWidgetMargin = 4096;
constexpr int kDragThreshold = 6;
constexpr int kDragDebounce = 5;

struct WidgetPosition {
    int right{kDefaultWidgetMargin};
    int bottom{kDefaultWidgetMargin};
};

[[nodiscard]] std::filesystem::path widgetPositionPath()
{
    if (const char* config_home = std::getenv("XDG_CONFIG_HOME"); config_home != nullptr && *config_home != '\0') {
        return std::filesystem::path{config_home} / "lofibox" / "widget-position.conf";
    }
    if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
        return std::filesystem::path{home} / ".config" / "lofibox" / "widget-position.conf";
    }
    return {};
}

[[nodiscard]] int boundedMargin(const std::string& text, int fallback) noexcept
{
    try {
        return std::clamp(std::stoi(text), 0, kMaximumWidgetMargin);
    } catch (...) {
        return fallback;
    }
}

[[nodiscard]] WidgetPosition loadWidgetPosition()
{
    WidgetPosition position;
    const auto path = widgetPositionPath();
    if (path.empty()) {
        return position;
    }

    std::ifstream input(path);
    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind("right=", 0) == 0) {
            position.right = boundedMargin(line.substr(6), position.right);
        } else if (line.rfind("bottom=", 0) == 0) {
            position.bottom = boundedMargin(line.substr(7), position.bottom);
        }
    }
    return position;
}

void saveWidgetPosition(const WidgetPosition& position) noexcept
{
    try {
        const auto path = widgetPositionPath();
        if (path.empty()) {
            return;
        }
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            return;
        }
        std::ofstream output(path, std::ios::trunc);
        if (output) {
            output << "right=" << position.right << '\n';
            output << "bottom=" << position.bottom << '\n';
        }
    } catch (...) {
        // Position persistence is optional and must never interrupt the widget.
    }
}

int createAnonymousFile(std::size_t size)
{
    int fd = -1;
#ifdef SYS_memfd_create
    fd = static_cast<int>(syscall(SYS_memfd_create, "lofibox-widget", MFD_CLOEXEC));
    if (fd >= 0) {
        if (ftruncate(fd, static_cast<off_t>(size)) == 0) {
            return fd;
        }
        close(fd);
    }
#endif

    char name[] = "/lofibox-widget-XXXXXX";
    fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0) {
        return -1;
    }
    shm_unlink(name);
    if (ftruncate(fd, static_cast<off_t>(size)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

[[nodiscard]] std::uint32_t packPixel(core::Color color) noexcept
{
    return (static_cast<std::uint32_t>(color.a) << 24U)
        | (static_cast<std::uint32_t>(color.r) << 16U)
        | (static_cast<std::uint32_t>(color.g) << 8U)
        | static_cast<std::uint32_t>(color.b);
}

[[nodiscard]] app::InputEvent makeChar(char ch)
{
    std::string label(1, ch);
    return app::makeCharacterInput(ch, std::move(label));
}

[[nodiscard]] app::InputEvent makeCommittedText(std::string text)
{
    return app::makeCommittedTextInput(std::move(text));
}

[[nodiscard]] std::vector<app::InputEvent> translateKeysym(xkb_keysym_t sym)
{
    switch (sym) {
    case XKB_KEY_BackSpace: return {app::InputEvent{app::InputKey::Backspace, "BACK", '\0'}};
    case XKB_KEY_Delete: return {app::InputEvent{app::InputKey::Delete, "DEL", '\0'}};
    case XKB_KEY_Return:
    case XKB_KEY_KP_Enter: return {app::InputEvent{app::InputKey::Enter, "OK", '\0'}};
    case XKB_KEY_Tab: return {app::InputEvent{app::InputKey::Tab, "TAB", '\0'}};
    case XKB_KEY_F1: return {app::InputEvent{app::InputKey::F1, "F1", '\0'}};
    case XKB_KEY_F2: return {app::InputEvent{app::InputKey::F2, "F2", '\0'}};
    case XKB_KEY_F3: return {app::InputEvent{app::InputKey::F3, "F3", '\0'}};
    case XKB_KEY_F4: return {app::InputEvent{app::InputKey::F4, "F4", '\0'}};
    case XKB_KEY_F5: return {app::InputEvent{app::InputKey::F5, "F5", '\0'}};
    case XKB_KEY_F6: return {app::InputEvent{app::InputKey::F6, "F6", '\0'}};
    case XKB_KEY_F7: return {app::InputEvent{app::InputKey::F7, "F7", '\0'}};
    case XKB_KEY_F8: return {app::InputEvent{app::InputKey::F8, "F8", '\0'}};
    case XKB_KEY_F9: return {app::InputEvent{app::InputKey::F9, "F9", '\0'}};
    case XKB_KEY_F10: return {app::InputEvent{app::InputKey::F10, "F10", '\0'}};
    case XKB_KEY_F11: return {app::InputEvent{app::InputKey::F11, "F11", '\0'}};
    case XKB_KEY_F12: return {app::InputEvent{app::InputKey::F12, "F12", '\0'}};
    case XKB_KEY_Home: return {app::InputEvent{app::InputKey::Home, "HOME", '\0'}};
    case XKB_KEY_Page_Up: return {app::InputEvent{app::InputKey::PageUp, "PGUP", '\0'}};
    case XKB_KEY_Page_Down: return {app::InputEvent{app::InputKey::PageDown, "PGDN", '\0'}};
    case XKB_KEY_Insert: return {app::InputEvent{app::InputKey::Insert, "INS", '\0'}};
    case XKB_KEY_Left: return {app::InputEvent{app::InputKey::Left, "LEFT", '\0'}};
    case XKB_KEY_Right: return {app::InputEvent{app::InputKey::Right, "RIGHT", '\0'}};
    case XKB_KEY_Up: return {app::InputEvent{app::InputKey::Up, "UP", '\0'}};
    case XKB_KEY_Down: return {app::InputEvent{app::InputKey::Down, "DOWN", '\0'}};
    default: return {};
    }
}

} // namespace

struct WidgetPresenter::Impl {
    struct Buffer {
        wl_buffer* buffer{};
        std::uint32_t* pixels{};
        bool busy{false};
    };

    enum class DragSource {
        None,
        Pointer,
        Touch,
    };

    Impl()
    {
        display = wl_display_connect(nullptr);
        if (display == nullptr) {
            throw std::runtime_error("wl_display_connect failed; lofibox-widget requires an active Wayland desktop session");
        }

        registry = wl_display_get_registry(display);
        wl_registry_add_listener(registry, &kRegistryListener, this);
        wl_display_roundtrip(display);
        wl_display_roundtrip(display);

        if (compositor == nullptr || shm == nullptr || layer_shell == nullptr) {
            throw std::runtime_error("Wayland compositor is missing wl_compositor, wl_shm, or zwlr_layer_shell_v1");
        }

        surface = wl_compositor_create_surface(compositor);
        if (surface == nullptr) {
            throw std::runtime_error("wl_compositor_create_surface failed");
        }

        layer_surface_callbacks = {
            this,
            &Impl::layerConfigure,
            &Impl::layerClosed,
        };
        layer_surface = lofibox_layer_shell_get_surface(
            layer_shell,
            surface,
            nullptr,
            kLayerTop,
            "lofibox-zero",
            &layer_surface_callbacks);
        if (layer_surface == nullptr) {
            throw std::runtime_error("zwlr_layer_shell_v1_get_layer_surface failed");
        }
        lofibox_layer_surface_set_size(layer_surface, kWidth, kHeight);
        lofibox_layer_surface_set_anchor(
            layer_surface,
            kAnchorRight | kAnchorBottom);
        const auto position = loadWidgetPosition();
        right_margin = position.right;
        bottom_margin = position.bottom;
        lofibox_layer_surface_set_margin(layer_surface, 0, right_margin, bottom_margin, 0);
        lofibox_layer_surface_set_exclusive_zone(layer_surface, 0);
        lofibox_layer_surface_set_keyboard_interactivity(
            layer_surface,
            layer_shell_version >= 4U
                ? kKeyboardInteractivityOnDemand
                : kKeyboardInteractivityNone);

        if (!createBuffer(buffers[0]) || !createBuffer(buffers[1])) {
            throw std::runtime_error("Wayland shm buffer allocation failed");
        }

        wl_surface_commit(surface);
        wl_display_flush(display);
    }

    ~Impl()
    {
        for (auto& buffer : buffers) {
            if (buffer.buffer != nullptr) {
                wl_buffer_destroy(buffer.buffer);
            }
            if (buffer.pixels != nullptr) {
                munmap(buffer.pixels, kBufferSize);
            }
        }
        if (xkb_state_ != nullptr) {
            xkb_state_unref(xkb_state_);
        }
        if (xkb_keymap_ != nullptr) {
            xkb_keymap_unref(xkb_keymap_);
        }
        if (xkb_context_ != nullptr) {
            xkb_context_unref(xkb_context_);
        }
        if (touch != nullptr) {
            wl_touch_destroy(touch);
        }
        if (pointer != nullptr) {
            wl_pointer_destroy(pointer);
        }
        if (keyboard != nullptr) {
            wl_keyboard_destroy(keyboard);
        }
        if (seat != nullptr) {
            wl_seat_destroy(seat);
        }
        if (layer_surface != nullptr) {
            lofibox_layer_surface_destroy(layer_surface);
        }
        if (surface != nullptr) {
            wl_surface_destroy(surface);
        }
        if (layer_shell != nullptr) {
            lofibox_layer_shell_destroy(layer_shell);
        }
        if (shm != nullptr) {
            wl_shm_destroy(shm);
        }
        if (compositor != nullptr) {
            wl_compositor_destroy(compositor);
        }
        if (registry != nullptr) {
            wl_registry_destroy(registry);
        }
        if (display != nullptr) {
            wl_display_disconnect(display);
        }
    }

    bool createBuffer(Buffer& buffer)
    {
        const int fd = createAnonymousFile(kBufferSize);
        if (fd < 0) {
            return false;
        }
        void* mapped = mmap(nullptr, kBufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mapped == MAP_FAILED) {
            close(fd);
            return false;
        }
        wl_shm_pool* pool = wl_shm_create_pool(shm, fd, kBufferSize);
        if (pool == nullptr) {
            munmap(mapped, kBufferSize);
            close(fd);
            return false;
        }
        buffer.buffer = wl_shm_pool_create_buffer(pool, 0, kWidth, kHeight, kStride, WL_SHM_FORMAT_ARGB8888);
        wl_shm_pool_destroy(pool);
        close(fd);
        if (buffer.buffer == nullptr) {
            munmap(mapped, kBufferSize);
            return false;
        }
        buffer.pixels = static_cast<std::uint32_t*>(mapped);
        wl_buffer_add_listener(buffer.buffer, &kBufferListener, &buffer);
        return true;
    }

    [[nodiscard]] Buffer* nextBuffer()
    {
        for (auto& buffer : buffers) {
            if (!buffer.busy) {
                return &buffer;
            }
        }
        return nullptr;
    }

    void updateKeymap(int fd, std::uint32_t size)
    {
        char* map = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
        if (map == MAP_FAILED) {
            close(fd);
            return;
        }
        if (xkb_context_ == nullptr) {
            xkb_context_ = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        }
        if (xkb_keymap_ != nullptr) {
            xkb_keymap_unref(xkb_keymap_);
        }
        if (xkb_state_ != nullptr) {
            xkb_state_unref(xkb_state_);
        }
        xkb_keymap_ = xkb_context_ == nullptr ? nullptr : xkb_keymap_new_from_string(
            xkb_context_, map, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        xkb_state_ = xkb_keymap_ == nullptr ? nullptr : xkb_state_new(xkb_keymap_);
        munmap(map, size);
        close(fd);
    }

    void handleKey(std::uint32_t key, std::uint32_t state)
    {
        if (state != WL_KEYBOARD_KEY_STATE_PRESSED || xkb_state_ == nullptr) {
            return;
        }
        const auto keycode = static_cast<xkb_keycode_t>(key + 8U);
        const xkb_keysym_t sym = xkb_state_key_get_one_sym(xkb_state_, keycode);
        auto translated = translateKeysym(sym);
        if (!translated.empty()) {
            pending_inputs.insert(pending_inputs.end(), translated.begin(), translated.end());
            return;
        }
        char utf8[64]{};
        const int len = xkb_state_key_get_utf8(xkb_state_, keycode, utf8, sizeof(utf8));
        if (len == 1 && static_cast<unsigned char>(utf8[0]) >= 0x20U && static_cast<unsigned char>(utf8[0]) <= 0x7EU) {
            pending_inputs.push_back(makeChar(utf8[0]));
        } else if (len > 0) {
            pending_inputs.push_back(makeCommittedText(std::string{utf8, static_cast<std::size_t>(len)}));
        }
    }

    void appendPointerTap(wl_fixed_t x, wl_fixed_t y, const char* label)
    {
        pending_inputs.push_back(app::makePointerTapInput(
            std::clamp(wl_fixed_to_int(x), 0, kWidth - 1),
            std::clamp(wl_fixed_to_int(y), 0, kHeight - 1),
            label));
    }

    [[nodiscard]] bool dragActive(DragSource source, int contact_id) const noexcept
    {
        return drag_source == source && drag_contact_id == contact_id;
    }

    void beginDrag(DragSource source, int contact_id, int x, int y)
    {
        if (drag_source != DragSource::None) {
            return;
        }
        drag_source = source;
        drag_contact_id = contact_id;
        drag_start_x = x;
        drag_start_y = y;
        drag_last_x = x;
        drag_last_y = y;
        drag_tap_x = x;
        drag_tap_y = y;
        drag_moved = false;
        position_dirty = false;
        drag_pending_x = 0;
        drag_pending_y = 0;
    }

    void updateDrag(DragSource source, int contact_id, int x, int y)
    {
        if (!dragActive(source, contact_id)) {
            return;
        }

        drag_tap_x = x;
        drag_tap_y = y;
        if (!drag_moved
            && std::abs(x - drag_start_x) < kDragThreshold
            && std::abs(y - drag_start_y) < kDragThreshold) {
            return;
        }
        drag_moved = true;

        drag_pending_x += x - drag_last_x;
        drag_pending_y += y - drag_last_y;

        // TDVP's touch-as-pointer route reports small ±1 px variations even
        // while a finger is stationary. Accumulate that raw displacement, but
        // wait until it is visible enough to move the compact widget without
        // visual chatter.
        if (std::abs(drag_pending_x) < kDragDebounce
            && std::abs(drag_pending_y) < kDragDebounce) {
            drag_last_x = x;
            drag_last_y = y;
            return;
        }

        const int delta_x = drag_pending_x;
        const int delta_y = drag_pending_y;
        drag_pending_x = 0;
        drag_pending_y = 0;
        const int previous_right = right_margin;
        const int previous_bottom = bottom_margin;
        right_margin = std::clamp(right_margin - delta_x, 0, kMaximumWidgetMargin);
        bottom_margin = std::clamp(bottom_margin - delta_y, 0, kMaximumWidgetMargin);
        const int applied_x = previous_right - right_margin;
        const int applied_y = previous_bottom - bottom_margin;

        // TDVP's Labwc keeps pointer coordinates in the original surface
        // reference space for the duration of a pointer grab. Do not feed the
        // margin movement back into the next delta: doing so makes one finger
        // movement grow as 10, 20, 30... pixels until the margin clamps.
        drag_last_x = x;
        drag_last_y = y;
        if (applied_x == 0 && applied_y == 0) {
            return;
        }

        lofibox_layer_surface_set_margin(layer_surface, 0, right_margin, bottom_margin, 0);
        wl_surface_commit(surface);
        wl_display_flush(display);
        position_dirty = true;
    }

    [[nodiscard]] bool finishDrag(DragSource source, int contact_id)
    {
        if (!dragActive(source, contact_id)) {
            return false;
        }

        const bool was_drag = drag_moved;
        if (position_dirty) {
            saveWidgetPosition(WidgetPosition{right_margin, bottom_margin});
        }
        drag_source = DragSource::None;
        drag_contact_id = -1;
        drag_moved = false;
        position_dirty = false;
        drag_pending_x = 0;
        drag_pending_y = 0;
        return was_drag;
    }

    static void registryGlobal(void* data, wl_registry* registry, std::uint32_t name, const char* interface, std::uint32_t version)
    {
        auto* self = static_cast<Impl*>(data);
        if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
            self->compositor = static_cast<wl_compositor*>(wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4U)));
        } else if (std::strcmp(interface, wl_shm_interface.name) == 0) {
            self->shm = static_cast<wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
        } else if (std::strcmp(interface, wl_seat_interface.name) == 0) {
            self->seat = static_cast<wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 7U)));
            wl_seat_add_listener(self->seat, &kSeatListener, self);
        } else if (std::strcmp(interface, "zwlr_layer_shell_v1") == 0) {
            self->layer_shell_version = std::min(version, 4U);
            self->layer_shell = lofibox_layer_shell_bind(registry, name, self->layer_shell_version);
        }
    }

    static void registryRemove(void*, wl_registry*, std::uint32_t) {}

    static void layerConfigure(void* data, std::uint32_t serial, std::uint32_t, std::uint32_t)
    {
        auto* self = static_cast<Impl*>(data);
        lofibox_layer_surface_ack_configure(self->layer_surface, serial);
        self->configured = true;
        self->last_canvas_pixels.clear();
    }

    static void layerClosed(void* data)
    {
        static_cast<Impl*>(data)->running = false;
    }

    static void seatCapabilities(void* data, wl_seat*, std::uint32_t capabilities)
    {
        auto* self = static_cast<Impl*>(data);
        if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0U && self->keyboard == nullptr) {
            self->keyboard = wl_seat_get_keyboard(self->seat);
            wl_keyboard_add_listener(self->keyboard, &kKeyboardListener, self);
        } else if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) == 0U && self->keyboard != nullptr) {
            wl_keyboard_destroy(self->keyboard);
            self->keyboard = nullptr;
        }
        if ((capabilities & WL_SEAT_CAPABILITY_POINTER) != 0U && self->pointer == nullptr) {
            self->pointer = wl_seat_get_pointer(self->seat);
            wl_pointer_add_listener(self->pointer, &kPointerListener, self);
        } else if ((capabilities & WL_SEAT_CAPABILITY_POINTER) == 0U && self->pointer != nullptr) {
            wl_pointer_destroy(self->pointer);
            self->pointer = nullptr;
        }
        if ((capabilities & WL_SEAT_CAPABILITY_TOUCH) != 0U && self->touch == nullptr) {
            self->touch = wl_seat_get_touch(self->seat);
            wl_touch_add_listener(self->touch, &kTouchListener, self);
        } else if ((capabilities & WL_SEAT_CAPABILITY_TOUCH) == 0U && self->touch != nullptr) {
            wl_touch_destroy(self->touch);
            self->touch = nullptr;
        }
    }

    static void seatName(void*, wl_seat*, const char*) {}
    static void keyboardKeymap(void* data, wl_keyboard*, std::uint32_t format, std::int32_t fd, std::uint32_t size)
    {
        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
            close(fd);
            return;
        }
        static_cast<Impl*>(data)->updateKeymap(fd, size);
    }
    static void keyboardEnter(void*, wl_keyboard*, std::uint32_t, wl_surface*, wl_array*) {}
    static void keyboardLeave(void*, wl_keyboard*, std::uint32_t, wl_surface*) {}
    static void keyboardKey(void* data, wl_keyboard*, std::uint32_t, std::uint32_t, std::uint32_t key, std::uint32_t state)
    {
        static_cast<Impl*>(data)->handleKey(key, state);
    }
    static void keyboardModifiers(void* data, wl_keyboard*, std::uint32_t, std::uint32_t depressed, std::uint32_t latched, std::uint32_t locked, std::uint32_t group)
    {
        auto* self = static_cast<Impl*>(data);
        if (self->xkb_state_ != nullptr) {
            xkb_state_update_mask(self->xkb_state_, depressed, latched, locked, 0, 0, group);
        }
    }
    static void keyboardRepeat(void*, wl_keyboard*, std::int32_t, std::int32_t) {}

    static void pointerEnter(void* data, wl_pointer*, std::uint32_t, wl_surface*, wl_fixed_t x, wl_fixed_t y)
    {
        auto* self = static_cast<Impl*>(data);
        self->pointer_x = x;
        self->pointer_y = y;
    }
    static void pointerLeave(void*, wl_pointer*, std::uint32_t, wl_surface*) {}
    static void pointerMotion(void* data, wl_pointer*, std::uint32_t, wl_fixed_t x, wl_fixed_t y)
    {
        auto* self = static_cast<Impl*>(data);
        self->pointer_x = x;
        self->pointer_y = y;
        self->updateDrag(DragSource::Pointer, 0, wl_fixed_to_int(x), wl_fixed_to_int(y));
    }
    static void pointerButton(void* data, wl_pointer*, std::uint32_t, std::uint32_t, std::uint32_t button, std::uint32_t state)
    {
        auto* self = static_cast<Impl*>(data);
        if (button != kPrimaryPointerButton) {
            return;
        }
        if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
            self->beginDrag(DragSource::Pointer, 0, wl_fixed_to_int(self->pointer_x), wl_fixed_to_int(self->pointer_y));
        } else if (state == WL_POINTER_BUTTON_STATE_RELEASED && self->dragActive(DragSource::Pointer, 0)) {
            if (!self->finishDrag(DragSource::Pointer, 0)) {
                self->appendPointerTap(self->pointer_x, self->pointer_y, "POINTER");
            }
        }
    }
    static void pointerAxis(void*, wl_pointer*, std::uint32_t, std::uint32_t, wl_fixed_t) {}
    static void pointerFrame(void*, wl_pointer*) {}
    static void pointerAxisSource(void*, wl_pointer*, std::uint32_t) {}
    static void pointerAxisStop(void*, wl_pointer*, std::uint32_t, std::uint32_t) {}
    static void pointerAxisDiscrete(void*, wl_pointer*, std::uint32_t, std::int32_t) {}

    static void touchDown(void* data, wl_touch*, std::uint32_t, std::uint32_t, wl_surface*, std::int32_t id, wl_fixed_t x, wl_fixed_t y)
    {
        static_cast<Impl*>(data)->beginDrag(DragSource::Touch, id, wl_fixed_to_int(x), wl_fixed_to_int(y));
    }
    static void touchUp(void* data, wl_touch*, std::uint32_t, std::uint32_t, std::int32_t id)
    {
        auto* self = static_cast<Impl*>(data);
        if (self->dragActive(DragSource::Touch, id) && !self->finishDrag(DragSource::Touch, id)) {
            self->pending_inputs.push_back(app::makePointerTapInput(
                std::clamp(self->drag_tap_x, 0, kWidth - 1),
                std::clamp(self->drag_tap_y, 0, kHeight - 1),
                "TOUCH"));
        }
    }
    static void touchMotion(void* data, wl_touch*, std::uint32_t, std::int32_t id, wl_fixed_t x, wl_fixed_t y)
    {
        static_cast<Impl*>(data)->updateDrag(DragSource::Touch, id, wl_fixed_to_int(x), wl_fixed_to_int(y));
    }
    static void touchFrame(void*, wl_touch*) {}
    static void touchCancel(void* data, wl_touch*)
    {
        auto* self = static_cast<Impl*>(data);
        if (self->drag_source == DragSource::Touch) {
            static_cast<void>(self->finishDrag(DragSource::Touch, self->drag_contact_id));
        }
    }
    static void touchShape(void*, wl_touch*, std::int32_t, wl_fixed_t, wl_fixed_t) {}
    static void touchOrientation(void*, wl_touch*, std::int32_t, wl_fixed_t) {}

    static void bufferRelease(void* data, wl_buffer*)
    {
        static_cast<Buffer*>(data)->busy = false;
    }

    static const wl_registry_listener kRegistryListener;
    static const wl_seat_listener kSeatListener;
    static const wl_keyboard_listener kKeyboardListener;
    static const wl_pointer_listener kPointerListener;
    static const wl_touch_listener kTouchListener;
    static const wl_buffer_listener kBufferListener;

    wl_display* display{};
    wl_registry* registry{};
    wl_compositor* compositor{};
    wl_shm* shm{};
    wl_seat* seat{};
    wl_keyboard* keyboard{};
    wl_pointer* pointer{};
    wl_touch* touch{};
    wl_surface* surface{};
    zwlr_layer_shell_v1* layer_shell{};
    zwlr_layer_surface_v1* layer_surface{};
    lofibox_layer_surface_callbacks layer_surface_callbacks{};
    xkb_context* xkb_context_{};
    xkb_keymap* xkb_keymap_{};
    xkb_state* xkb_state_{};
    Buffer buffers[2]{};
    std::vector<core::Color> last_canvas_pixels{};
    std::vector<app::InputEvent> pending_inputs{};
    wl_fixed_t pointer_x{};
    wl_fixed_t pointer_y{};
    int right_margin{kDefaultWidgetMargin};
    int bottom_margin{kDefaultWidgetMargin};
    DragSource drag_source{DragSource::None};
    int drag_contact_id{-1};
    int drag_start_x{};
    int drag_start_y{};
    int drag_last_x{};
    int drag_last_y{};
    int drag_pending_x{};
    int drag_pending_y{};
    int drag_tap_x{};
    int drag_tap_y{};
    bool drag_moved{false};
    bool position_dirty{false};
    std::uint32_t layer_shell_version{};
    bool configured{false};
    bool running{true};
};

const wl_registry_listener WidgetPresenter::Impl::kRegistryListener{WidgetPresenter::Impl::registryGlobal, WidgetPresenter::Impl::registryRemove};
const wl_seat_listener WidgetPresenter::Impl::kSeatListener{WidgetPresenter::Impl::seatCapabilities, WidgetPresenter::Impl::seatName};
const wl_keyboard_listener WidgetPresenter::Impl::kKeyboardListener{
    WidgetPresenter::Impl::keyboardKeymap,
    WidgetPresenter::Impl::keyboardEnter,
    WidgetPresenter::Impl::keyboardLeave,
    WidgetPresenter::Impl::keyboardKey,
    WidgetPresenter::Impl::keyboardModifiers,
    WidgetPresenter::Impl::keyboardRepeat,
};
const wl_pointer_listener WidgetPresenter::Impl::kPointerListener{
    WidgetPresenter::Impl::pointerEnter,
    WidgetPresenter::Impl::pointerLeave,
    WidgetPresenter::Impl::pointerMotion,
    WidgetPresenter::Impl::pointerButton,
    WidgetPresenter::Impl::pointerAxis,
    WidgetPresenter::Impl::pointerFrame,
    WidgetPresenter::Impl::pointerAxisSource,
    WidgetPresenter::Impl::pointerAxisStop,
    WidgetPresenter::Impl::pointerAxisDiscrete,
};
const wl_touch_listener WidgetPresenter::Impl::kTouchListener{
    WidgetPresenter::Impl::touchDown,
    WidgetPresenter::Impl::touchUp,
    WidgetPresenter::Impl::touchMotion,
    WidgetPresenter::Impl::touchFrame,
    WidgetPresenter::Impl::touchCancel,
    WidgetPresenter::Impl::touchShape,
    WidgetPresenter::Impl::touchOrientation,
};
const wl_buffer_listener WidgetPresenter::Impl::kBufferListener{WidgetPresenter::Impl::bufferRelease};

WidgetPresenter::WidgetPresenter()
    : impl_(std::make_unique<Impl>())
{
}

WidgetPresenter::~WidgetPresenter() = default;

bool WidgetPresenter::pump()
{
    while (wl_display_prepare_read(impl_->display) != 0) {
        wl_display_dispatch_pending(impl_->display);
    }
    wl_display_flush(impl_->display);

    pollfd pfd{};
    pfd.fd = wl_display_get_fd(impl_->display);
    pfd.events = POLLIN;
    const int rc = poll(&pfd, 1, 0);
    if (rc > 0 && (pfd.revents & POLLIN) != 0) {
        wl_display_read_events(impl_->display);
        wl_display_dispatch_pending(impl_->display);
    } else {
        wl_display_cancel_read(impl_->display);
        wl_display_dispatch_pending(impl_->display);
    }
    return impl_->running && wl_display_get_error(impl_->display) == 0;
}

std::vector<app::InputEvent> WidgetPresenter::drainInput()
{
    auto result = std::move(impl_->pending_inputs);
    impl_->pending_inputs.clear();
    return result;
}

void WidgetPresenter::present(const core::Canvas& canvas)
{
    if (!impl_->configured || canvas.width() != kWidth || canvas.height() != kHeight) {
        return;
    }

    const auto& source = canvas.pixels();
    if (!impl_->last_canvas_pixels.empty() && impl_->last_canvas_pixels == source) {
        return;
    }
    auto* buffer = impl_->nextBuffer();
    if (buffer == nullptr) {
        return;
    }

    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            buffer->pixels[y * kWidth + x] = packPixel(source[static_cast<std::size_t>(y * kWidth + x)]);
        }
    }

    wl_surface_attach(impl_->surface, buffer->buffer, 0, 0);
    wl_surface_damage_buffer(impl_->surface, 0, 0, kWidth, kHeight);
    wl_surface_commit(impl_->surface);
    buffer->busy = true;
    impl_->last_canvas_pixels = source;
    wl_display_flush(impl_->display);
}

} // namespace lofibox::platform::wayland
