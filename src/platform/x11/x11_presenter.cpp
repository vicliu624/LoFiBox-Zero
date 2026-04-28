// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/x11/x11_presenter.h"

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "core/display_profile.h"

namespace lofibox::platform::x11 {
namespace {

[[nodiscard]] std::uint32_t maskShift(unsigned long mask) noexcept
{
    std::uint32_t shift = 0;
    while (mask != 0UL && (mask & 1UL) == 0UL) {
        mask >>= 1U;
        ++shift;
    }
    return shift;
}

[[nodiscard]] std::uint32_t maskBits(unsigned long mask) noexcept
{
    std::uint32_t bits = 0;
    while (mask != 0UL) {
        bits += static_cast<std::uint32_t>(mask & 1UL);
        mask >>= 1U;
    }
    return bits;
}

[[nodiscard]] unsigned long scaleToMask(std::uint8_t value, unsigned long mask) noexcept
{
    if (mask == 0UL) {
        return 0UL;
    }
    const auto bits = maskBits(mask);
    const auto shift = maskShift(mask);
    const unsigned long max_value = (1UL << bits) - 1UL;
    return ((static_cast<unsigned long>(value) * max_value + 127UL) / 255UL) << shift;
}

[[nodiscard]] unsigned long packPixel(core::Color color, Visual* visual) noexcept
{
    return scaleToMask(color.r, visual->red_mask)
        | scaleToMask(color.g, visual->green_mask)
        | scaleToMask(color.b, visual->blue_mask);
}

[[nodiscard]] app::InputEvent makeChar(char ch)
{
    std::string label(1, ch);
    return app::makeCharacterInput(ch, label);
}

struct MotifWmHints {
    unsigned long flags{};
    unsigned long functions{};
    unsigned long decorations{};
    long input_mode{};
    unsigned long status{};
};

void makeChromelessManagedWindow(Display* display, Window window)
{
    constexpr unsigned long kMotifHintsDecorations = 1UL << 1U;
    const Atom hints_atom = XInternAtom(display, "_MOTIF_WM_HINTS", False);
    if (hints_atom == None) {
        return;
    }

    MotifWmHints hints{};
    hints.flags = kMotifHintsDecorations;
    hints.decorations = 0;
    XChangeProperty(
        display,
        window,
        hints_atom,
        hints_atom,
        32,
        PropModeReplace,
        reinterpret_cast<const unsigned char*>(&hints),
        5);
}

[[nodiscard]] bool isSuperH(KeySym keysym, unsigned int state) noexcept
{
    return (state & Mod4Mask) != 0U && (keysym == XK_h || keysym == XK_H);
}

int ignoreRecoverableXError(Display*, XErrorEvent*) noexcept
{
    return 0;
}

void safelyFocusWindow(Display* display, Window window)
{
    XWindowAttributes attributes{};
    if (XGetWindowAttributes(display, window, &attributes) == 0 || attributes.map_state != IsViewable) {
        return;
    }

    auto* previous_handler = XSetErrorHandler(ignoreRecoverableXError);
    XSetInputFocus(display, window, RevertToParent, CurrentTime);
    XSync(display, False);
    XSetErrorHandler(previous_handler);
}

void waitUntilMapped(Display* display, Window window)
{
    XWindowAttributes attributes{};
    if (XGetWindowAttributes(display, window, &attributes) != 0 && attributes.map_state == IsViewable) {
        return;
    }

    for (;;) {
        XEvent event{};
        XWindowEvent(display, window, StructureNotifyMask, &event);
        if (event.type == MapNotify) {
            return;
        }
    }
}

[[nodiscard]] std::vector<app::InputEvent> translateKey(Display* display, XKeyEvent& key_event)
{
    KeySym keysym{};
    char text_buffer[8]{};
    const int text_length = XLookupString(&key_event, text_buffer, static_cast<int>(sizeof(text_buffer)), &keysym, nullptr);

    switch (keysym) {
    case XK_BackSpace:
        return {app::InputEvent{app::InputKey::Backspace, "BACK", '\0'}};
    case XK_Delete:
        return {app::InputEvent{app::InputKey::Delete, "DEL", '\0'}};
    case XK_Return:
    case XK_KP_Enter:
        return {app::InputEvent{app::InputKey::Enter, "OK", '\0'}};
    case XK_Tab:
        return {app::InputEvent{app::InputKey::Tab, "TAB", '\0'}};
    case XK_F1:
        return {app::InputEvent{app::InputKey::F1, "F1", '\0'}};
    case XK_F2:
        return {app::InputEvent{app::InputKey::F2, "F2", '\0'}};
    case XK_F3:
        return {app::InputEvent{app::InputKey::F3, "F3", '\0'}};
    case XK_F4:
        return {app::InputEvent{app::InputKey::F4, "F4", '\0'}};
    case XK_F5:
        return {app::InputEvent{app::InputKey::F5, "F5", '\0'}};
    case XK_F6:
        return {app::InputEvent{app::InputKey::F6, "F6", '\0'}};
    case XK_F7:
        return {app::InputEvent{app::InputKey::F7, "F7", '\0'}};
    case XK_F8:
        return {app::InputEvent{app::InputKey::F8, "F8", '\0'}};
    case XK_F9:
        return {app::InputEvent{app::InputKey::F9, "F9", '\0'}};
    case XK_F10:
        return {app::InputEvent{app::InputKey::F10, "F10", '\0'}};
    case XK_F11:
        return {app::InputEvent{app::InputKey::F11, "F11", '\0'}};
    case XK_F12:
        return {app::InputEvent{app::InputKey::F12, "F12", '\0'}};
    case XK_Home:
        return {app::InputEvent{app::InputKey::Home, "HOME", '\0'}};
    case XK_Page_Up:
        return {app::InputEvent{app::InputKey::PageUp, "PGUP", '\0'}};
    case XK_Page_Down:
        return {app::InputEvent{app::InputKey::PageDown, "PGDN", '\0'}};
    case XK_Insert:
        return {app::InputEvent{app::InputKey::Insert, "INS", '\0'}};
    case XK_Left:
        return {app::InputEvent{app::InputKey::Left, "LEFT", '\0'}};
    case XK_Right:
        return {app::InputEvent{app::InputKey::Right, "RIGHT", '\0'}};
    case XK_Up:
        return {app::InputEvent{app::InputKey::Up, "UP", '\0'}};
    case XK_Down:
        return {app::InputEvent{app::InputKey::Down, "DOWN", '\0'}};
    default:
        break;
    }

    if (text_length == 1 && static_cast<unsigned char>(text_buffer[0]) >= 0x20U && static_cast<unsigned char>(text_buffer[0]) <= 0x7EU) {
        return {makeChar(text_buffer[0])};
    }

    (void)display;
    return {};
}

} // namespace

struct X11Presenter::Impl {
    Impl()
    {
        display = XOpenDisplay(nullptr);
        if (display == nullptr) {
            throw std::runtime_error("XOpenDisplay failed");
        }

        screen = DefaultScreen(display);
        visual = DefaultVisual(display, screen);
        depth = DefaultDepth(display, screen);
        screen_width = DisplayWidth(display, screen);
        screen_height = DisplayHeight(display, screen);
        fixed_device_surface = screen_width <= core::kDisplayWidth || screen_height <= core::kDisplayHeight;
        window_x = fixed_device_surface ? 0 : std::max(0, (screen_width - core::kDisplayWidth) / 2);
        window_y = fixed_device_surface ? 0 : std::max(0, (screen_height - core::kDisplayHeight) / 2);
        window = XCreateSimpleWindow(
            display,
            RootWindow(display, screen),
            window_x,
            window_y,
            static_cast<unsigned int>(core::kDisplayWidth),
            static_cast<unsigned int>(core::kDisplayHeight),
            0,
            BlackPixel(display, screen),
            BlackPixel(display, screen));
        if (window == 0) {
            throw std::runtime_error("XCreateSimpleWindow failed");
        }

        XStoreName(display, window, "LoFiBox Zero");
        makeChromelessManagedWindow(display, window);
        XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
        XMapRaised(display, window);
        XFlush(display);
        waitUntilMapped(display, window);
        safelyFocusWindow(display, window);
        gc = XCreateGC(display, window, 0, nullptr);
        if (gc == nullptr) {
            throw std::runtime_error("XCreateGC failed");
        }

        XFlush(display);
    }

    ~Impl()
    {
        if (gc != nullptr) {
            XFreeGC(display, gc);
        }
        if (window != 0) {
            XDestroyWindow(display, window);
        }
        if (display != nullptr) {
            XCloseDisplay(display);
        }
    }

    Display* display{};
    int screen{};
    int screen_width{};
    int screen_height{};
    Visual* visual{};
    int depth{};
    Window window{};
    int window_x{};
    int window_y{};
    GC gc{};
    bool running{true};
    bool fixed_device_surface{false};
    bool dragging{false};
    int drag_root_x{};
    int drag_root_y{};
    int drag_origin_x{};
    int drag_origin_y{};
    std::vector<app::InputEvent> pending_inputs{};
};

X11Presenter::X11Presenter()
    : impl_(std::make_unique<Impl>())
{
}

X11Presenter::~X11Presenter() = default;

bool X11Presenter::pump()
{
    while (XPending(impl_->display) > 0) {
        XEvent event{};
        XNextEvent(impl_->display, &event);
        if (event.type == DestroyNotify) {
            impl_->running = false;
        } else if (event.type == KeyPress) {
            KeySym keysym{};
            char text_buffer[8]{};
            XLookupString(&event.xkey, text_buffer, static_cast<int>(sizeof(text_buffer)), &keysym, nullptr);
            if (isSuperH(keysym, event.xkey.state)) {
                XIconifyWindow(impl_->display, impl_->window, impl_->screen);
                XFlush(impl_->display);
                continue;
            }
            auto translated = translateKey(impl_->display, event.xkey);
            impl_->pending_inputs.insert(impl_->pending_inputs.end(), translated.begin(), translated.end());
        } else if (event.type == ButtonPress && event.xbutton.button == Button1 && !impl_->fixed_device_surface) {
            impl_->dragging = true;
            impl_->drag_root_x = event.xbutton.x_root;
            impl_->drag_root_y = event.xbutton.y_root;
            impl_->drag_origin_x = impl_->window_x;
            impl_->drag_origin_y = impl_->window_y;
        } else if (event.type == ButtonRelease && event.xbutton.button == Button1) {
            impl_->dragging = false;
        } else if (event.type == MotionNotify && impl_->dragging && !impl_->fixed_device_surface) {
            impl_->window_x = impl_->drag_origin_x + (event.xmotion.x_root - impl_->drag_root_x);
            impl_->window_y = impl_->drag_origin_y + (event.xmotion.y_root - impl_->drag_root_y);
            XMoveWindow(impl_->display, impl_->window, impl_->window_x, impl_->window_y);
        }
    }
    return impl_->running;
}

std::vector<app::InputEvent> X11Presenter::drainInput()
{
    auto result = std::move(impl_->pending_inputs);
    impl_->pending_inputs.clear();
    return result;
}

void X11Presenter::present(const core::Canvas& canvas)
{
    auto* image = XCreateImage(
        impl_->display,
        impl_->visual,
        static_cast<unsigned int>(impl_->depth),
        ZPixmap,
        0,
        nullptr,
        static_cast<unsigned int>(canvas.width()),
        static_cast<unsigned int>(canvas.height()),
        32,
        0);
    if (image == nullptr) {
        throw std::runtime_error("XCreateImage failed");
    }

    image->data = static_cast<char*>(std::calloc(static_cast<std::size_t>(image->bytes_per_line), static_cast<std::size_t>(image->height)));
    if (image->data == nullptr) {
        XDestroyImage(image);
        throw std::runtime_error("XCreateImage pixel allocation failed");
    }

    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            XPutPixel(image, x, y, packPixel(canvas.pixel(x, y), impl_->visual));
        }
    }

    XPutImage(impl_->display, impl_->window, impl_->gc, image, 0, 0, 0, 0, static_cast<unsigned int>(canvas.width()), static_cast<unsigned int>(canvas.height()));
    XDestroyImage(image);
    XFlush(impl_->display);
}

} // namespace lofibox::platform::x11
