// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/png_canvas_loader.h"

#include <filesystem>
#include <optional>
#include <vector>

#include "core/color.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")
#else
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "third_party/stb_image.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#endif

namespace fs = std::filesystem;

namespace lofibox::platform::host {
namespace {

#if defined(_WIN32)
struct CoInitializer {
    HRESULT result{CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)};

    ~CoInitializer()
    {
        if (SUCCEEDED(result)) {
            CoUninitialize();
        }
    }
};

std::optional<core::Canvas> tryLoadWicPng(const fs::path& path)
{
    CoInitializer com{};
    if (FAILED(com.result) && com.result != RPC_E_CHANGED_MODE) {
        return std::nullopt;
    }

    IWICImagingFactory* factory = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) {
        return std::nullopt;
    }

    IWICBitmapDecoder* decoder = nullptr;
    if (FAILED(factory->CreateDecoderFromFilename(
            path.wstring().c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &decoder))) {
        factory->Release();
        return std::nullopt;
    }

    IWICBitmapFrameDecode* frame = nullptr;
    if (FAILED(decoder->GetFrame(0, &frame))) {
        decoder->Release();
        factory->Release();
        return std::nullopt;
    }

    IWICFormatConverter* converter = nullptr;
    if (FAILED(factory->CreateFormatConverter(&converter))) {
        frame->Release();
        decoder->Release();
        factory->Release();
        return std::nullopt;
    }

    if (FAILED(converter->Initialize(
            frame,
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom))) {
        converter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        return std::nullopt;
    }

    UINT width = 0;
    UINT height = 0;
    if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0) {
        converter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        return std::nullopt;
    }

    core::Canvas canvas{static_cast<int>(width), static_cast<int>(height)};
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width * height * 4));
    const UINT stride = width * 4;
    const UINT buffer_size = stride * height;
    if (FAILED(converter->CopyPixels(nullptr, stride, buffer_size, pixels.data()))) {
        converter->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        return std::nullopt;
    }

    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            const auto* pixel = pixels.data() + (static_cast<std::size_t>(y) * stride) + (static_cast<std::size_t>(x) * 4);
            canvas.setPixel(static_cast<int>(x), static_cast<int>(y), core::rgba(pixel[0], pixel[1], pixel[2], pixel[3]));
        }
    }

    converter->Release();
    frame->Release();
    decoder->Release();
    factory->Release();
    return canvas;
}
#endif

} // namespace

std::optional<core::Canvas> loadPngCanvas(const fs::path& path)
{
    std::error_code ec{};
    if (!fs::exists(path, ec) || ec) {
        return std::nullopt;
    }

#if defined(_WIN32)
    return tryLoadWicPng(path);
#else
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr || width <= 0 || height <= 0) {
        return std::nullopt;
    }

    core::Canvas canvas{width, height};
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const auto* pixel = pixels + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4) + (static_cast<std::size_t>(x) * 4);
            canvas.setPixel(x, y, core::rgba(pixel[0], pixel[1], pixel[2], pixel[3]));
        }
    }

    stbi_image_free(pixels);
    return canvas;
#endif
}

} // namespace lofibox::platform::host
