#include "UI/Shell/LauncherLogo.h"

#include "KindUI/Rendering/IconRenderer.h"
#include "KindUI/Rendering/OverlayRenderer.h"
#include "Util/PathUtils.h"

#include "Core/Logger.h"

#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincodec.h>
#include "Platform/UndefWin32Macros.h"
#pragma comment(lib, "windowscodecs.lib")
#endif

namespace we::programs::welauncher {
namespace {

#if defined(_WIN32)
template <typename T>
void SafeRelease(T*& ptr) {
    if (ptr) {
        ptr->Release();
        ptr = nullptr;
    }
}

bool LoadPngRgbaWic(
    const std::filesystem::path& path,
    std::vector<uint8_t>& outRgba,
    uint32_t& outWidth,
    uint32_t& outHeight) {
    // Initialize COM for WIC if needed, but never tear it down — the launcher
    // process keeps COM alive for the window lifetime (avoids heap/COM races).
    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initHr) && initHr != RPC_E_CHANGED_MODE && initHr != S_FALSE) {
        return false;
    }

    IWICImagingFactory* factory = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory));
    if (FAILED(hr) || !factory) {
        return false;
    }

    IWICBitmapDecoder* decoder = nullptr;
    hr = factory->CreateDecoderFromFilename(
        path.wstring().c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder);
    if (FAILED(hr) || !decoder) {
        SafeRelease(factory);
        return false;
    }

    IWICBitmapFrameDecode* frame = nullptr;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame) {
        SafeRelease(decoder);
        SafeRelease(factory);
        return false;
    }

    IWICFormatConverter* converter = nullptr;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr) || !converter) {
        SafeRelease(frame);
        SafeRelease(decoder);
        SafeRelease(factory);
        return false;
    }

    hr = converter->Initialize(
        frame,
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        SafeRelease(converter);
        SafeRelease(frame);
        SafeRelease(decoder);
        SafeRelease(factory);
        return false;
    }

    UINT width = 0;
    UINT height = 0;
    hr = converter->GetSize(&width, &height);
    if (FAILED(hr) || width == 0 || height == 0) {
        SafeRelease(converter);
        SafeRelease(frame);
        SafeRelease(decoder);
        SafeRelease(factory);
        return false;
    }

    const size_t stride = static_cast<size_t>(width) * 4u;
    const size_t bytes = stride * static_cast<size_t>(height);
    outRgba.resize(bytes);
    hr = converter->CopyPixels(nullptr, static_cast<UINT>(stride), static_cast<UINT>(bytes), outRgba.data());

    SafeRelease(converter);
    SafeRelease(frame);
    SafeRelease(decoder);
    SafeRelease(factory);

    if (FAILED(hr)) {
        outRgba.clear();
        return false;
    }

    outWidth = width;
    outHeight = height;
    return true;
}
#endif

} // namespace

std::filesystem::path ResolveLauncherLogoPath(const std::filesystem::path& engineRoot) {
    if (!engineRoot.empty()) {
        const auto candidate = engineRoot / "Assets" / "Editor" / "Logo" / "Logo_UI.png";
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    if (auto root = PathUtils::FindEngineRoot(PathUtils::GetExecutableDirectory())) {
        const auto candidate = *root / "Assets" / "Editor" / "Logo" / "Logo_UI.png";
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

we::rhi::RHIDescriptorSetHandle LoadLauncherLogoTexture(
    we::runtime::kindui::OverlayRenderer* renderer,
    const std::filesystem::path& engineRoot,
    uint32_t displaySizePx) {
    if (!renderer) {
        return we::rhi::RHIDescriptorSetHandle::Invalid;
    }

    const auto logoPath = ResolveLauncherLogoPath(engineRoot);
#if defined(_WIN32)
    if (!logoPath.empty()) {
        std::vector<uint8_t> rgba;
        uint32_t width = 0;
        uint32_t height = 0;
        if (LoadPngRgbaWic(logoPath, rgba, width, height) && !rgba.empty()) {
            auto set = renderer->UploadRgbaTexture(width, height, rgba, true);
            if (set != we::rhi::RHIDescriptorSetHandle::Invalid) {
                return set;
            }
        } else {
            HE_WARN("[WeLauncher] Failed to decode logo: " + PathUtils::ToUtf8(logoPath));
        }
    }
#else
    (void)logoPath;
#endif

    if (auto* icons = renderer->GetIconRenderer()) {
        const uint32_t size = displaySizePx > 0 ? displaySizePx : 18u;
        auto set = icons->GetIcon("Assets/Editor/Logo/Logo_UI.png", size);
        if (set == we::rhi::RHIDescriptorSetHandle::Invalid) {
            set = icons->GetIcon("Assets/Editor/WindEffects.svg", size);
        }
        return set;
    }

    return we::rhi::RHIDescriptorSetHandle::Invalid;
}

} // namespace we::programs::welauncher
