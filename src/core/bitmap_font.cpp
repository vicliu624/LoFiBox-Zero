// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/bitmap_font.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#elif defined(LOFIBOX_HAVE_FREETYPE)
#include <filesystem>
#include <optional>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYNTHESIS_H
#endif

namespace lofibox::core::bitmap_font {
namespace {

struct Glyph {
    std::array<std::uint8_t, kGlyphHeight> rows{};
};

Glyph glyphFor(char raw) noexcept
{
    switch (raw) {
    case 'a': return Glyph{{0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F}};
    case 'b': return Glyph{{0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x1E}};
    case 'c': return Glyph{{0x00, 0x00, 0x0E, 0x11, 0x10, 0x11, 0x0E}};
    case 'd': return Glyph{{0x01, 0x01, 0x0F, 0x11, 0x11, 0x11, 0x0F}};
    case 'e': return Glyph{{0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E}};
    case 'f': return Glyph{{0x06, 0x08, 0x1E, 0x08, 0x08, 0x08, 0x08}};
    case 'k': return Glyph{{0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12}};
    case 'l': return Glyph{{0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}};
    case 'n': return Glyph{{0x00, 0x00, 0x1E, 0x11, 0x11, 0x11, 0x11}};
    case 'o': return Glyph{{0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E}};
    case 'r': return Glyph{{0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10}};
    case 't': return Glyph{{0x08, 0x08, 0x1E, 0x08, 0x08, 0x09, 0x06}};
    default:
        break;
    }

    const char ch = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));

    switch (ch) {
    case 'A': return Glyph{{0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}};
    case 'B': return Glyph{{0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}};
    case 'C': return Glyph{{0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}};
    case 'D': return Glyph{{0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}};
    case 'E': return Glyph{{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}};
    case 'F': return Glyph{{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}};
    case 'G': return Glyph{{0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}};
    case 'H': return Glyph{{0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}};
    case 'I': return Glyph{{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F}};
    case 'J': return Glyph{{0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E}};
    case 'K': return Glyph{{0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}};
    case 'L': return Glyph{{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}};
    case 'M': return Glyph{{0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}};
    case 'N': return Glyph{{0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}};
    case 'O': return Glyph{{0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}};
    case 'P': return Glyph{{0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}};
    case 'Q': return Glyph{{0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}};
    case 'R': return Glyph{{0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}};
    case 'S': return Glyph{{0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}};
    case 'T': return Glyph{{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}};
    case 'U': return Glyph{{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}};
    case 'V': return Glyph{{0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}};
    case 'W': return Glyph{{0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}};
    case 'X': return Glyph{{0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}};
    case 'Y': return Glyph{{0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}};
    case 'Z': return Glyph{{0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}};
    case '0': return Glyph{{0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}};
    case '1': return Glyph{{0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}};
    case '2': return Glyph{{0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}};
    case '3': return Glyph{{0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}};
    case '4': return Glyph{{0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}};
    case '5': return Glyph{{0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}};
    case '6': return Glyph{{0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}};
    case '7': return Glyph{{0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}};
    case '8': return Glyph{{0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}};
    case '9': return Glyph{{0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}};
    case '#': return Glyph{{0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A}};
    case '$': return Glyph{{0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04}};
    case '%': return Glyph{{0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13}};
    case '&': return Glyph{{0x0C, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0D}};
    case '\'': return Glyph{{0x04, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00}};
    case '(': return Glyph{{0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}};
    case ')': return Glyph{{0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}};
    case '*': return Glyph{{0x00, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x00}};
    case '+': return Glyph{{0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}};
    case '-': return Glyph{{0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}};
    case ',': return Glyph{{0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x08}};
    case '.': return Glyph{{0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}};
    case ':': return Glyph{{0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}};
    case ';': return Glyph{{0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x08}};
    case '<': return Glyph{{0x01, 0x02, 0x04, 0x08, 0x04, 0x02, 0x01}};
    case '=': return Glyph{{0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00}};
    case '>': return Glyph{{0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10}};
    case '?': return Glyph{{0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}};
    case '@': return Glyph{{0x0E, 0x11, 0x15, 0x17, 0x10, 0x11, 0x0E}};
    case '[': return Glyph{{0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E}};
    case '\\': return Glyph{{0x10, 0x08, 0x08, 0x04, 0x02, 0x02, 0x01}};
    case ']': return Glyph{{0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E}};
    case '^': return Glyph{{0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00}};
    case '_': return Glyph{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F}};
    case '`': return Glyph{{0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00}};
    case '/': return Glyph{{0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10}};
    case '!': return Glyph{{0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}};
    case '"': return Glyph{{0x0A, 0x0A, 0x05, 0x00, 0x00, 0x00, 0x00}};
    case '{': return Glyph{{0x02, 0x04, 0x04, 0x08, 0x04, 0x04, 0x02}};
    case '|': return Glyph{{0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}};
    case '}': return Glyph{{0x08, 0x04, 0x04, 0x02, 0x04, 0x04, 0x08}};
    case ' ': return Glyph{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
    default: return Glyph{{0x1F, 0x01, 0x02, 0x04, 0x08, 0x00, 0x08}};
    }
}

void drawGlyph(Canvas& canvas, const Glyph& glyph, int x, int y, Color color, int scale) noexcept
{
    const int clamped_scale = std::max(scale, 1);

    for (int row = 0; row < kGlyphHeight; ++row) {
        for (int col = 0; col < kGlyphWidth; ++col) {
            const bool filled = ((glyph.rows[static_cast<std::size_t>(row)] >> (kGlyphWidth - 1 - col)) & 0x01U) != 0U;
            if (!filled) {
                continue;
            }

            for (int dy = 0; dy < clamped_scale; ++dy) {
                for (int dx = 0; dx < clamped_scale; ++dx) {
                    canvas.setPixel(x + (col * clamped_scale) + dx, y + (row * clamped_scale) + dy, color);
                }
            }
        }
    }
}

#if defined(_WIN32)
std::wstring utf8ToWide(std::string_view text)
{
    if (text.empty()) {
        return {};
    }

    const int required = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0) {
        return {};
    }

    std::wstring result(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), required);
    return result;
}

HFONT createUiFont(int scale) noexcept
{
    const int clamped_scale = std::max(scale, 1);
    const int height = -(11 * clamped_scale);
    return CreateFontW(
        height,
        0,
        0,
        0,
        FW_MEDIUM,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Noto Sans SC Medium");
}

SIZE measureWideTextGdi(std::wstring_view text, int scale) noexcept
{
    SIZE size{};
    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc) {
        return size;
    }

    HFONT font = createUiFont(scale);
    HGDIOBJ old_font = SelectObject(dc, font);
    GetTextExtentPoint32W(dc, text.data(), static_cast<int>(text.size()), &size);
    SelectObject(dc, old_font);
    DeleteObject(font);
    DeleteDC(dc);
    return size;
}

void drawWideTextGdi(Canvas& canvas, std::wstring_view text, int x, int y, Color color, int scale) noexcept
{
    if (text.empty()) {
        return;
    }

    const SIZE size = measureWideTextGdi(text, scale);
    if (size.cx <= 0 || size.cy <= 0) {
        return;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size.cx;
    bmi.bmiHeader.biHeight = -size.cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc) {
        return;
    }

    HBITMAP bitmap = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap || !bits) {
        if (bitmap) {
            DeleteObject(bitmap);
        }
        DeleteDC(dc);
        return;
    }

    HGDIOBJ old_bitmap = SelectObject(dc, bitmap);
    HFONT font = createUiFont(scale);
    HGDIOBJ old_font = SelectObject(dc, font);

    RECT rect{0, 0, size.cx, size.cy};
    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(dc, &rect, brush);
    DeleteObject(brush);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(color.r, color.g, color.b));
    DrawTextW(dc, text.data(), static_cast<int>(text.size()), &rect, DT_LEFT | DT_TOP | DT_NOPREFIX | DT_NOCLIP);

    const auto* pixels = static_cast<const std::uint8_t*>(bits);
    const int stride = size.cx * 4;
    for (int py = 0; py < size.cy; ++py) {
        for (int px = 0; px < size.cx; ++px) {
            const auto* pixel = pixels + (py * stride) + (px * 4);
            const std::uint8_t b = pixel[0];
            const std::uint8_t g = pixel[1];
            const std::uint8_t r = pixel[2];
            const std::uint8_t a = std::max({r, g, b});
            if (a == 0) {
                continue;
            }
            canvas.setPixel(x + px, y + py, rgba(r, g, b, a));
        }
    }

    SelectObject(dc, old_font);
    SelectObject(dc, old_bitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(dc);
}
#elif defined(LOFIBOX_HAVE_FREETYPE)
namespace fs = std::filesystem;

std::vector<std::uint32_t> utf8ToCodepoints(std::string_view text)
{
    std::vector<std::uint32_t> codepoints{};
    for (std::size_t index = 0; index < text.size();) {
        const unsigned char lead = static_cast<unsigned char>(text[index]);
        if (lead < 0x80U) {
            codepoints.push_back(lead);
            ++index;
            continue;
        }

        std::uint32_t codepoint = 0;
        std::size_t extra = 0;
        if ((lead & 0xE0U) == 0xC0U) {
            codepoint = lead & 0x1FU;
            extra = 1;
        } else if ((lead & 0xF0U) == 0xE0U) {
            codepoint = lead & 0x0FU;
            extra = 2;
        } else if ((lead & 0xF8U) == 0xF0U) {
            codepoint = lead & 0x07U;
            extra = 3;
        } else {
            codepoints.push_back('?');
            ++index;
            continue;
        }

        if (index + extra >= text.size()) {
            codepoints.push_back('?');
            break;
        }

        bool valid = true;
        for (std::size_t offset = 1; offset <= extra; ++offset) {
            const unsigned char next = static_cast<unsigned char>(text[index + offset]);
            if ((next & 0xC0U) != 0x80U) {
                valid = false;
                break;
            }
            codepoint = (codepoint << 6) | (next & 0x3FU);
        }

        codepoints.push_back(valid ? codepoint : static_cast<std::uint32_t>('?'));
        index += valid ? (extra + 1) : 1;
    }

    return codepoints;
}

std::optional<fs::path> findLinuxFontPath()
{
    if (const char* font_path = std::getenv("LOFIBOX_FONT_PATH")) {
        std::error_code ec{};
        if (*font_path != '\0' && fs::exists(font_path, ec) && !ec) {
            return fs::path(font_path);
        }
    }

    static const std::array<fs::path, 23> candidates{
        fs::path{"/usr/share/fonts/opentype/noto/NotoSansCJK-Medium.ttc"},
        fs::path{"/usr/share/fonts/opentype/noto/NotoSansCJK-Medium.otf"},
        fs::path{"/usr/share/fonts/opentype/noto/NotoSansCJKsc-Medium.otf"},
        fs::path{"/usr/share/fonts/truetype/noto/NotoSansCJK-Medium.ttf"},
        fs::path{"/usr/share/fonts/truetype/noto/NotoSansCJK-Medium.ttc"},
        fs::path{"/usr/share/fonts/truetype/noto/NotoSansCJKsc-Medium.otf"},
        fs::path{"/usr/share/fonts/opentype/noto/NotoSansSC-Medium.otf"},
        fs::path{"/usr/share/fonts/truetype/noto/NotoSansSC-Medium.ttf"},
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.otf",
        "/usr/share/fonts/opentype/noto/NotoSansCJKsc-Regular.otf",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/noto/NotoSansCJKsc-Regular.otf",
        "/usr/share/fonts/opentype/noto/NotoSansSC-Regular.otf",
        "/usr/share/fonts/truetype/noto/NotoSansSC-Regular.ttf",
        "/usr/share/fonts/opentype/source-han-sans/SourceHanSansSC-Medium.otf",
        "/usr/share/fonts/opentype/source-han-sans/SourceHanSansCN-Medium.otf",
        "/usr/share/fonts/opentype/source-han-sans/SourceHanSansSC-Regular.otf",
        "/usr/share/fonts/opentype/source-han-sans/SourceHanSansCN-Regular.otf",
        "/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf"};

    for (const auto& candidate : candidates) {
        std::error_code ec{};
        if (fs::exists(candidate, ec) && !ec) {
            return candidate;
        }
    }
    return std::nullopt;
}

Color alphaBlend(Color dst, Color src, std::uint8_t opacity) noexcept
{
    const float src_alpha = (static_cast<float>(src.a) / 255.0f) * (static_cast<float>(opacity) / 255.0f);
    const float dst_alpha = static_cast<float>(dst.a) / 255.0f;
    const float out_alpha = src_alpha + (dst_alpha * (1.0f - src_alpha));
    if (out_alpha <= 0.0f) {
        return rgba(0, 0, 0, 0);
    }

    const auto blend = [&](std::uint8_t dst_c, std::uint8_t src_c) -> std::uint8_t {
        const float out =
            ((static_cast<float>(src_c) * src_alpha) + (static_cast<float>(dst_c) * dst_alpha * (1.0f - src_alpha))) / out_alpha;
        return static_cast<std::uint8_t>(std::clamp(out, 0.0f, 255.0f));
    };

    return rgba(
        blend(dst.r, src.r),
        blend(dst.g, src.g),
        blend(dst.b, src.b),
        static_cast<std::uint8_t>(std::clamp(out_alpha * 255.0f, 0.0f, 255.0f)));
}

struct FreetypeFace {
    FT_Library library{};
    FT_Face face{};
    bool valid{false};
    int pixel_size{0};
    int ascent{0};
    int line_height{0};

    explicit FreetypeFace(int scale)
    {
        pixel_size = std::max(scale, 1) * 11;
        if (FT_Init_FreeType(&library) != 0) {
            return;
        }

        const auto font_path = findLinuxFontPath();
        if (!font_path || FT_New_Face(library, font_path->string().c_str(), 0, &face) != 0) {
            if (library) {
                FT_Done_FreeType(library);
                library = nullptr;
            }
            return;
        }

        if (FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(pixel_size)) != 0) {
            FT_Done_Face(face);
            FT_Done_FreeType(library);
            face = nullptr;
            library = nullptr;
            return;
        }

        ascent = static_cast<int>(face->size->metrics.ascender >> 6);
        line_height = std::max(static_cast<int>(face->size->metrics.height >> 6), pixel_size + 1);
        valid = true;
    }

    ~FreetypeFace()
    {
        if (face) {
            FT_Done_Face(face);
        }
        if (library) {
            FT_Done_FreeType(library);
        }
    }
};

int measureUtf8TextFreeType(std::string_view text, int scale) noexcept
{
    FreetypeFace ft(scale);
    if (!ft.valid) {
        return 0;
    }

    int line_width = 0;
    int max_width = 0;
    for (const auto codepoint : utf8ToCodepoints(text)) {
        if (codepoint == '\n') {
            max_width = std::max(max_width, line_width);
            line_width = 0;
            continue;
        }

        if (FT_Load_Char(ft.face, codepoint, FT_LOAD_DEFAULT) == 0) {
            line_width += static_cast<int>(ft.face->glyph->advance.x >> 6);
        } else {
            line_width += ft.pixel_size / 2;
        }
    }

    return std::max(max_width, line_width);
}

void drawUtf8TextFreeType(Canvas& canvas, std::string_view text, int x, int y, Color color, int scale) noexcept
{
    FreetypeFace ft(scale);
    if (!ft.valid) {
        return;
    }

    int cursor_x = x;
    int cursor_y = y;
    int baseline = cursor_y + ft.ascent;
    for (const auto codepoint : utf8ToCodepoints(text)) {
        if (codepoint == '\n') {
            cursor_x = x;
            cursor_y += ft.line_height;
            baseline = cursor_y + ft.ascent;
            continue;
        }

        if (FT_Load_Char(ft.face, codepoint, FT_LOAD_DEFAULT) != 0) {
            cursor_x += ft.pixel_size / 2;
            continue;
        }
        FT_GlyphSlot_Embolden(ft.face->glyph);
        if (FT_Render_Glyph(ft.face->glyph, FT_RENDER_MODE_NORMAL) != 0) {
            cursor_x += ft.pixel_size / 2;
            continue;
        }

        const FT_Bitmap& bitmap = ft.face->glyph->bitmap;
        const int draw_x = cursor_x + ft.face->glyph->bitmap_left;
        const int draw_y = baseline - ft.face->glyph->bitmap_top;

        for (int row = 0; row < static_cast<int>(bitmap.rows); ++row) {
            for (int col = 0; col < static_cast<int>(bitmap.width); ++col) {
                const auto alpha = bitmap.buffer[(row * bitmap.pitch) + col];
                if (alpha == 0) {
                    continue;
                }
                const int px = draw_x + col;
                const int py = draw_y + row;
                const auto dst = canvas.pixel(px, py);
                canvas.setPixel(px, py, alphaBlend(dst, color, alpha));
            }
        }

        const int advance = static_cast<int>(ft.face->glyph->advance.x >> 6);
        cursor_x += advance > 0 ? advance : static_cast<int>(bitmap.width);
    }
}
#endif

} // namespace

int measureText(std::string_view text, int scale) noexcept
{
#if defined(_WIN32)
    const auto wide = utf8ToWide(text);
    const auto size = measureWideTextGdi(wide, scale);
    if (size.cx > 0) {
        return size.cx;
    }
#elif defined(LOFIBOX_HAVE_FREETYPE)
    if (const int width = measureUtf8TextFreeType(text, scale); width > 0) {
        return width;
    }
#endif

    const int clamped_scale = std::max(scale, 1);
    int line_width = 0;
    int max_width = 0;

    for (const char ch : text) {
        if (ch == '\n') {
            max_width = std::max(max_width, line_width);
            line_width = 0;
            continue;
        }

        line_width += (kGlyphWidth + kGlyphSpacing) * clamped_scale;
    }

    max_width = std::max(max_width, line_width);
    return max_width > 0 ? max_width - (kGlyphSpacing * clamped_scale) : 0;
}

int lineHeight(int scale) noexcept
{
#if defined(_WIN32)
    const auto size = measureWideTextGdi(L"M", scale);
    if (size.cy > 0) {
        return size.cy + std::max(scale, 1);
    }
#elif defined(LOFIBOX_HAVE_FREETYPE)
    FreetypeFace ft(scale);
    if (ft.valid) {
        return ft.line_height;
    }
#endif

    const int clamped_scale = std::max(scale, 1);
    return (kGlyphHeight + 2) * clamped_scale;
}

void drawText(Canvas& canvas, std::string_view text, int x, int y, Color color, int scale) noexcept
{
#if defined(_WIN32)
    if (const auto wide = utf8ToWide(text); !wide.empty()) {
        drawWideTextGdi(canvas, wide, x, y, color, scale);
        return;
    }
#elif defined(LOFIBOX_HAVE_FREETYPE)
    drawUtf8TextFreeType(canvas, text, x, y, color, scale);
    return;
#endif

    const int clamped_scale = std::max(scale, 1);
    int cursor_x = x;
    int cursor_y = y;

    for (const char ch : text) {
        if (ch == '\n') {
            cursor_x = x;
            cursor_y += lineHeight(clamped_scale);
            continue;
        }

        drawGlyph(canvas, glyphFor(ch), cursor_x, cursor_y, color, clamped_scale);
        cursor_x += (kGlyphWidth + kGlyphSpacing) * clamped_scale;
    }
}

} // namespace lofibox::core::bitmap_font
