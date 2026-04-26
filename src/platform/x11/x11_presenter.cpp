// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/x11/x11_presenter.h"

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <X11/Xlib.h>
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
    case XK_Home:
        return {app::InputEvent{app::InputKey::Home, "HOME", '\0'}};
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
        window = XCreateSimpleWindow(
            display,
            RootWindow(display, screen),
            0,
            0,
            static_cast<unsigned int>(core::kDisplayWidth),
            static_cast<unsigned int>(core::kDisplayHeight),
            0,
            BlackPixel(display, screen),
            BlackPixel(display, screen));
        if (window == 0) {
            throw std::runtime_error("XCreateSimpleWindow failed");
        }

        XStoreName(display, window, "LoFiBox Zero");
        XSetWindowAttributes attributes{};
        attributes.override_redirect = True;
        XChangeWindowAttributes(display, window, CWOverrideRedirect, &attributes);
        XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask);
        XMapRaised(display, window);
        XSync(display, False);
        XSetInputFocus(display, window, RevertToParent, CurrentTime);
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
    Visual* visual{};
    int depth{};
    Window window{};
    GC gc{};
    bool running{true};
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
            auto translated = translateKey(impl_->display, event.xkey);
            impl_->pending_inputs.insert(impl_->pending_inputs.end(), translated.begin(), translated.end());
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
