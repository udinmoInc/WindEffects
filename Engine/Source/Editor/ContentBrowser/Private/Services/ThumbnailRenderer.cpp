#include "Services/ThumbnailRenderer.hpp"
#include "Registry/AssetTypeResolver.hpp"
#include "Core/Theme.hpp"
#include "Core/Logger.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace we::editor::contentbrowser {

namespace {

std::string ResolvePath(const std::string& path) {
    if (std::filesystem::exists(path)) return path;
    const std::string candidates[] = { "../" + path, "../../" + path, "../../../" + path };
    for (const auto& c : candidates) {
        if (std::filesystem::exists(c)) return c;
    }
    return path;
}

std::string ResolveFolderSvgPath() {
    const char* candidates[] = {
        "Assets/Icons/content-browser-folder.svg",
        "Icons/content-browser-folder.svg",
        "../Assets/Icons/content-browser-folder.svg",
        "../../Assets/Icons/content-browser-folder.svg",
        "Engine/Content/Icons/content-browser-folder.svg",
        "../Engine/Content/Icons/content-browser-folder.svg",
    };
    for (const char* path : candidates) {
        if (std::filesystem::exists(path)) return path;
    }
    return {};
}

uint32_t SnapFolderRasterHeight(uint32_t heightPx) {
    if (heightPx <= 72u) return 64u;
    if (heightPx <= 112u) return 96u;
    return 128u;
}

uint8_t BrightenChannel(uint8_t channel, float hoverBrightness) {
    const float boost = std::clamp(hoverBrightness, 0.0f, 1.0f);
    const float f = static_cast<float>(channel) / 255.0f;
    return static_cast<uint8_t>(std::min(255.0f, (f + (1.0f - f) * 0.10f * boost) * 255.0f));
}

std::array<uint8_t, 3> ThemeRgb(const we::UI::Color& color, float hoverBrightness) {
    return {
        BrightenChannel(static_cast<uint8_t>(color.r * 255.0f), hoverBrightness),
        BrightenChannel(static_cast<uint8_t>(color.g * 255.0f), hoverBrightness),
        BrightenChannel(static_cast<uint8_t>(color.b * 255.0f), hoverBrightness),
    };
}

} // namespace

BitmapRGBA ThumbnailRenderer::CreateEmpty(uint32_t size) {
    BitmapRGBA bmp;
    bmp.width = size;
    bmp.height = size;
    bmp.pixels.resize(static_cast<size_t>(size) * size * 4, 0);
    return bmp;
}

void ThumbnailRenderer::SetPixel(BitmapRGBA& bmp, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || y < 0 || x >= static_cast<int>(bmp.width) || y >= static_cast<int>(bmp.height)) return;
    const size_t idx = (static_cast<size_t>(y) * bmp.width + static_cast<size_t>(x)) * 4;
    bmp.pixels[idx] = r;
    bmp.pixels[idx + 1] = g;
    bmp.pixels[idx + 2] = b;
    bmp.pixels[idx + 3] = a;
}

void ThumbnailRenderer::FillRect(BitmapRGBA& bmp, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    for (int py = y; py < y + h; ++py) {
        for (int px = x; px < x + w; ++px) {
            SetPixel(bmp, px, py, r, g, b, a);
        }
    }
}

void ThumbnailRenderer::DrawCircle(BitmapRGBA& bmp, int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                SetPixel(bmp, cx + x, cy + y, r, g, b, a);
            }
        }
    }
}

void ThumbnailRenderer::Blit(const BitmapRGBA& src, BitmapRGBA& dst, int dstX, int dstY, int dstW, int dstH) {
    if (src.width == 0 || src.height == 0) return;
    for (int y = 0; y < dstH; ++y) {
        for (int x = 0; x < dstW; ++x) {
            const int sx = x * static_cast<int>(src.width) / dstW;
            const int sy = y * static_cast<int>(src.height) / dstH;
            const size_t sidx = (static_cast<size_t>(sy) * src.width + static_cast<size_t>(sx)) * 4;
            const uint8_t a = src.pixels[sidx + 3];
            if (a == 0) continue;
            SetPixel(dst, dstX + x, dstY + y, src.pixels[sidx], src.pixels[sidx + 1], src.pixels[sidx + 2], a);
        }
    }
}

BitmapRGBA ThumbnailRenderer::FitIntoCell(const BitmapRGBA& source, uint32_t cellW, uint32_t cellH) {
    BitmapRGBA cell = CreateEmpty(std::max(cellW, cellH));
    cell.width = cellW;
    cell.height = cellH;
    cell.pixels.assign(static_cast<size_t>(cellW) * cellH * 4, 0);

    if (source.width == 0 || source.height == 0) return cell;

    const float scale = std::min(
        static_cast<float>(cellW) / static_cast<float>(source.width),
        static_cast<float>(cellH) / static_cast<float>(source.height));
    const int drawW = std::max(1, static_cast<int>(source.width * scale));
    const int drawH = std::max(1, static_cast<int>(source.height * scale));
    const int offX = (static_cast<int>(cellW) - drawW) / 2;
    const int offY = (static_cast<int>(cellH) - drawH) / 2;
    Blit(source, cell, offX, offY, drawW, drawH);
    return cell;
}

BitmapRGBA ThumbnailRenderer::LoadImageFile(const std::string& path, uint32_t targetSize) {
    const std::string resolved = ResolvePath(path);
    const auto ext = std::filesystem::path(resolved).extension().string();
    std::string lower = ext;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == ".svg") {
        NSVGimage* image = nsvgParseFromFile(resolved.c_str(), "px", 96.0f);
        if (!image) return RenderGenericIcon(AssetType::Texture);

        NSVGrasterizer* rast = nsvgCreateRasterizer();
        if (!rast) {
            nsvgDelete(image);
            return RenderGenericIcon(AssetType::Texture);
        }

        const int rasterSize = static_cast<int>(targetSize);
        std::vector<uint8_t> raster(static_cast<size_t>(rasterSize) * rasterSize * 4, 0);
        const float scale = static_cast<float>(rasterSize) / std::max(image->width, image->height);
        nsvgRasterize(rast, image, 0, 0, scale, raster.data(), rasterSize, rasterSize, rasterSize * 4);
        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);

        BitmapRGBA bmp;
        bmp.width = targetSize;
        bmp.height = targetSize;
        bmp.pixels = std::move(raster);
        return bmp;
    }

    int w = 0, h = 0, channels = 0;
    stbi_uc* data = stbi_load(resolved.c_str(), &w, &h, &channels, 4);
    if (!data) return RenderGenericIcon(AssetType::Texture);

    BitmapRGBA src;
    src.width = static_cast<uint32_t>(w);
    src.height = static_cast<uint32_t>(h);
    src.pixels.assign(data, data + static_cast<size_t>(w) * h * 4);
    stbi_image_free(data);

    return FitIntoCell(src, targetSize, targetSize);
}

BitmapRGBA ThumbnailRenderer::RenderMaterialSphere(const AssetRecord& asset, bool isInstance) {
    auto bmp = CreateEmpty(kThumbnailSize);
    FillRect(bmp, 0, 0, static_cast<int>(kThumbnailSize), static_cast<int>(kThumbnailSize), 28, 30, 36, 255);

    uint8_t r = 140, g = 145, b = 155;
    std::ifstream in(asset.diskPath);
    if (in) {
        std::stringstream ss;
        ss << in.rdbuf();
        const std::string content = ss.str();
        const auto findColor = [&](const std::string& key) -> int {
            const auto pos = content.find(key);
            if (pos == std::string::npos) return -1;
            const auto start = content.find('[', pos);
            const auto end = content.find(']', start);
            if (start == std::string::npos || end == std::string::npos) return -1;
            return std::stoi(content.substr(start + 1, end - start - 1));
        };
        const int cr = findColor("baseColor");
        if (cr >= 0) {
            const auto start = content.find('[', content.find("baseColor"));
            const auto end = content.find(']', start);
            std::string arr = content.substr(start + 1, end - start - 1);
            int rv = 0, gv = 0, bv = 0;
            char comma;
            std::stringstream parser(arr);
            float fr = 0, fg = 0, fb = 0;
            parser >> fr >> comma >> fg >> comma >> fb;
            r = static_cast<uint8_t>(fr * 255);
            g = static_cast<uint8_t>(fg * 255);
            b = static_cast<uint8_t>(fb * 255);
        }
    }

    const int cx = static_cast<int>(kThumbnailSize) / 2;
    const int cy = static_cast<int>(kThumbnailSize) / 2;
    const int radius = static_cast<int>(kThumbnailSize) / 3;
    DrawCircle(bmp, cx, cy, radius, r, g, b, 255);

    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                const float shade = 1.0f - (static_cast<float>(x + y) / static_cast<float>(radius * 2)) * 0.35f;
                const size_t idx = (static_cast<size_t>(cy + y) * bmp.width + static_cast<size_t>(cx + x)) * 4;
                bmp.pixels[idx] = static_cast<uint8_t>(bmp.pixels[idx] * shade);
                bmp.pixels[idx + 1] = static_cast<uint8_t>(bmp.pixels[idx + 1] * shade);
                bmp.pixels[idx + 2] = static_cast<uint8_t>(bmp.pixels[idx + 2] * shade);
            }
        }
    }

    if (isInstance) {
        FillRect(bmp, static_cast<int>(kThumbnailSize) - 28, 6, 22, 22, 214, 162, 74, 220);
    }
    return bmp;
}

BitmapRGBA ThumbnailRenderer::RenderMeshPreview(const AssetRecord& asset, bool skeletal) {
    (void)asset;
    auto bmp = CreateEmpty(kThumbnailSize);
    FillRect(bmp, 0, 0, static_cast<int>(kThumbnailSize), static_cast<int>(kThumbnailSize), 28, 30, 36, 255);

    const int cx = static_cast<int>(kThumbnailSize) / 2;
    const int cy = static_cast<int>(kThumbnailSize) / 2 + 8;
    const uint8_t edgeR = skeletal ? 120 : 180;
    const uint8_t edgeG = skeletal ? 200 : 190;
    const uint8_t edgeB = skeletal ? 255 : 170;

    auto drawLine = [&](int x0, int y0, int x1, int y1) {
        const int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        const int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;
        while (true) {
            SetPixel(bmp, x0, y0, edgeR, edgeG, edgeB, 255);
            if (x0 == x1 && y0 == y1) break;
            const int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    };

    const int s = skeletal ? 22 : 28;
    drawLine(cx - s, cy - s, cx + s, cy - s);
    drawLine(cx + s, cy - s, cx + s, cy + s);
    drawLine(cx + s, cy + s, cx - s, cy + s);
    drawLine(cx - s, cy + s, cx - s, cy - s);
    drawLine(cx - s, cy - s, cx, cy - s - 16);
    drawLine(cx + s, cy - s, cx, cy - s - 16);
    drawLine(cx, cy - s - 16, cx, cy - s - 28);

    if (skeletal) {
        DrawCircle(bmp, cx, cy - s - 34, 8, edgeR, edgeG, edgeB, 255);
        drawLine(cx, cy - s - 26, cx, cy);
        drawLine(cx, cy - 8, cx - 14, cy + 10);
        drawLine(cx, cy - 8, cx + 14, cy + 10);
    }
    return bmp;
}

BitmapRGBA ThumbnailRenderer::RenderBlueprintPreview(const AssetRecord&) {
    auto bmp = CreateEmpty(kThumbnailSize);
    FillRect(bmp, 0, 0, static_cast<int>(kThumbnailSize), static_cast<int>(kThumbnailSize), 20, 48, 88, 255);
    FillRect(bmp, 16, 24, 40, 24, 40, 100, 180, 255);
    FillRect(bmp, 72, 48, 40, 24, 80, 160, 220, 255);
    FillRect(bmp, 44, 72, 40, 24, 60, 120, 200, 255);
    return bmp;
}

BitmapRGBA ThumbnailRenderer::RenderAudioWaveform(const AssetRecord&) {
    auto bmp = CreateEmpty(kThumbnailSize);
    FillRect(bmp, 0, 0, static_cast<int>(kThumbnailSize), static_cast<int>(kThumbnailSize), 24, 26, 32, 255);
    const int bars = 16;
    for (int i = 0; i < bars; ++i) {
        const int h = 12 + (i * 7 % 40);
        const int x = 12 + i * 7;
        FillRect(bmp, x, static_cast<int>(kThumbnailSize) / 2 - h / 2, 4, h, 100, 180, 255, 255);
    }
    return bmp;
}

BitmapRGBA ThumbnailRenderer::RenderFontSample(const AssetRecord& asset) {
    auto bmp = CreateEmpty(kThumbnailSize);
    FillRect(bmp, 0, 0, static_cast<int>(kThumbnailSize), static_cast<int>(kThumbnailSize), 32, 34, 40, 255);
    FillRect(bmp, 20, 44, 88, 40, 220, 220, 225, 255);
    (void)asset;
    return bmp;
}

BitmapRGBA ThumbnailRenderer::RenderScriptIcon(const AssetRecord&) {
    auto bmp = CreateEmpty(kThumbnailSize);
    FillRect(bmp, 0, 0, static_cast<int>(kThumbnailSize), static_cast<int>(kThumbnailSize), 30, 32, 38, 255);
    for (int i = 0; i < 5; ++i) {
        FillRect(bmp, 24, 28 + i * 16, 80, 6, 120, 200, 140, 255);
    }
    return bmp;
}

BitmapRGBA ThumbnailRenderer::RenderScenePreview(const AssetRecord&) {
    auto bmp = CreateEmpty(kThumbnailSize);
    FillRect(bmp, 0, 0, static_cast<int>(kThumbnailSize), static_cast<int>(kThumbnailSize), 18, 20, 26, 255);
    FillRect(bmp, 0, static_cast<int>(kThumbnailSize) * 2 / 3, static_cast<int>(kThumbnailSize), static_cast<int>(kThumbnailSize) / 3, 40, 80, 50, 255);
    FillRect(bmp, 40, 50, 48, 36, 70, 75, 85, 255);
    return bmp;
}

BitmapRGBA ThumbnailRenderer::RenderGenericIcon(AssetType type) {
    auto bmp = CreateEmpty(kThumbnailSize);
    FillRect(bmp, 0, 0, static_cast<int>(kThumbnailSize), static_cast<int>(kThumbnailSize), 36, 38, 44, 255);
    uint8_t r = 120, g = 130, b = 150;
    switch (type) {
        case AssetType::Texture: r = 100; g = 180; b = 255; break;
        case AssetType::Material: r = 180; g = 150; b = 100; break;
        case AssetType::StaticMesh: r = 180; g = 190; b = 170; break;
        case AssetType::Blueprint: r = 60; g = 120; b = 200; break;
        default: break;
    }
    DrawCircle(bmp, static_cast<int>(kThumbnailSize) / 2, static_cast<int>(kThumbnailSize) / 2, 28, r, g, b, 255);
    return bmp;
}

BitmapRGBA ThumbnailRenderer::Render(const AssetRecord& asset) {
    switch (asset.type) {
        case AssetType::Texture:
            return LoadImageFile(asset.diskPath, kThumbnailSize);
        case AssetType::Material:
            return RenderMaterialSphere(asset, false);
        case AssetType::MaterialInstance:
            return RenderMaterialSphere(asset, true);
        case AssetType::StaticMesh:
            return RenderMeshPreview(asset, false);
        case AssetType::SkeletalMesh:
            return RenderMeshPreview(asset, true);
        case AssetType::Animation:
            return RenderMeshPreview(asset, true);
        case AssetType::Blueprint:
            return RenderBlueprintPreview(asset);
        case AssetType::Scene:
        case AssetType::Prefab:
            return RenderScenePreview(asset);
        case AssetType::Audio:
            return RenderAudioWaveform(asset);
        case AssetType::Font:
            return RenderFontSample(asset);
        case AssetType::Script:
            return RenderScriptIcon(asset);
        case AssetType::Video:
            return RenderGenericIcon(AssetType::Video);
        default:
            return RenderGenericIcon(asset.type);
    }
}

void ThumbnailRenderer::AlphaBlend(BitmapRGBA& bmp, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (a == 0) return;
    if (x < 0 || y < 0 || x >= static_cast<int>(bmp.width) || y >= static_cast<int>(bmp.height)) return;
    const size_t idx = (static_cast<size_t>(y) * bmp.width + static_cast<size_t>(x)) * 4;
    const float srcA = a / 255.0f;
    const float dstA = bmp.pixels[idx + 3] / 255.0f;
    const float outA = srcA + dstA * (1.0f - srcA);
    if (outA <= 0.0f) return;
    const float blend = srcA / outA;
    bmp.pixels[idx]     = static_cast<uint8_t>(r * blend + bmp.pixels[idx] * (1.0f - blend));
    bmp.pixels[idx + 1] = static_cast<uint8_t>(g * blend + bmp.pixels[idx + 1] * (1.0f - blend));
    bmp.pixels[idx + 2] = static_cast<uint8_t>(b * blend + bmp.pixels[idx + 2] * (1.0f - blend));
    bmp.pixels[idx + 3] = static_cast<uint8_t>(outA * 255.0f);
}

float ThumbnailRenderer::RoundedRectCoverage(float px, float py, float rx, float ry, float rw, float rh, float radius) {
    const float r = std::max(0.0f, std::min(radius, std::min(rw, rh) * 0.5f));
    const float cx = rx + rw * 0.5f;
    const float cy = ry + rh * 0.5f;
    const float hx = rw * 0.5f - r;
    const float hy = rh * 0.5f - r;
    const float dx = std::abs(px - cx) - hx;
    const float dy = std::abs(py - cy) - hy;
    const float ax = std::max(dx, 0.0f);
    const float ay = std::max(dy, 0.0f);
    const float outside = std::sqrt(ax * ax + ay * ay) - r;
    const float inside = std::min(std::max(dx, dy), 0.0f);
    const float sdf = outside + inside;
    return std::clamp(0.65f - sdf, 0.0f, 1.0f);
}

void ThumbnailRenderer::FillRoundedRect(BitmapRGBA& bmp, float x, float y, float w, float h, float radius,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    const int x0 = std::max(0, static_cast<int>(std::floor(x)));
    const int y0 = std::max(0, static_cast<int>(std::floor(y)));
    const int x1 = std::min(static_cast<int>(bmp.width), static_cast<int>(std::ceil(x + w)));
    const int y1 = std::min(static_cast<int>(bmp.height), static_cast<int>(std::ceil(y + h)));
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float coverage = RoundedRectCoverage(px + 0.5f, py + 0.5f, x, y, w, h, radius);
            if (coverage <= 0.0f) continue;
            AlphaBlend(bmp, px, py, r, g, b, static_cast<uint8_t>(a * coverage));
        }
    }
}

void ThumbnailRenderer::FillRoundedRectVerticalGradient(BitmapRGBA& bmp, float x, float y, float w, float h, float radius,
    uint8_t rTop, uint8_t gTop, uint8_t bTop, uint8_t rBot, uint8_t gBot, uint8_t bBot, uint8_t a)
{
    const int x0 = std::max(0, static_cast<int>(std::floor(x)));
    const int y0 = std::max(0, static_cast<int>(std::floor(y)));
    const int x1 = std::min(static_cast<int>(bmp.width), static_cast<int>(std::ceil(x + w)));
    const int y1 = std::min(static_cast<int>(bmp.height), static_cast<int>(std::ceil(y + h)));
    for (int py = y0; py < y1; ++py) {
        const float t = h > 0.0f ? (py + 0.5f - y) / h : 0.0f;
        const uint8_t r = static_cast<uint8_t>(rTop + (rBot - rTop) * t);
        const uint8_t g = static_cast<uint8_t>(gTop + (gBot - gTop) * t);
        const uint8_t b = static_cast<uint8_t>(bTop + (bBot - bTop) * t);
        for (int px = x0; px < x1; ++px) {
            const float coverage = RoundedRectCoverage(px + 0.5f, py + 0.5f, x, y, w, h, radius);
            if (coverage <= 0.0f) continue;
            AlphaBlend(bmp, px, py, r, g, b, static_cast<uint8_t>(a * coverage));
        }
    }
}

BitmapRGBA ThumbnailRenderer::RenderContentBrowserFolderProcedural(uint32_t w, uint32_t h, float hoverBrightness) {
    // Procedural fallback mirrors the SVG path layout (nanosvg-compatible flat paths).
    BitmapRGBA bmp;
    bmp.width = w;
    bmp.height = h;
    bmp.pixels.assign(static_cast<size_t>(w) * h * 4, 0);

    const we::UI::Theme& theme = we::UI::Theme::Get();
    const auto shadow = ThemeRgb(theme.ContentBrowserFolderShadow, hoverBrightness);
    const auto tabTop = ThemeRgb(theme.ContentBrowserFolderHighlight, hoverBrightness);
    const auto tabBot = ThemeRgb(theme.ContentBrowserFolderTab, hoverBrightness);
    const auto bodyTop = ThemeRgb(theme.ContentBrowserFolderTab, hoverBrightness);
    const auto bodyMid = ThemeRgb(theme.ContentBrowserFolderBody, hoverBrightness);
    const auto bodyBot = ThemeRgb({ theme.ContentBrowserFolderBody.r * 0.92f,
        theme.ContentBrowserFolderBody.g * 0.92f, theme.ContentBrowserFolderBody.b * 0.92f, 1.0f }, hoverBrightness);
    const auto highlight = ThemeRgb(theme.ContentBrowserFolderHighlight, hoverBrightness);

    constexpr float kRefW = 146.0f;
    constexpr float kRefH = 100.0f;
    const float sx = static_cast<float>(w) / kRefW;
    const float sy = static_cast<float>(h) / kRefH;
    const float scale = std::min(sx, sy);
    const auto X = [sx](float v) { return v * sx; };
    const auto Y = [sy](float v) { return v * sy; };
    const auto S = [scale](float v) { return v * scale; };

    const float castX = X(2.8f);
    const float castY = Y(3.2f);
    FillRoundedRect(bmp, X(9.0f) + castX, Y(27.0f) + castY, X(132.0f), Y(67.0f), S(4.0f),
        shadow[0], shadow[1], shadow[2], 77);
    FillRoundedRect(bmp, X(9.0f) + castX, Y(10.0f) + castY, X(35.0f), Y(18.0f), S(4.0f),
        shadow[0], shadow[1], shadow[2], 77);

    FillRoundedRectVerticalGradient(bmp, X(9.0f), Y(27.0f), X(132.0f), Y(67.0f), S(4.0f),
        bodyTop[0], bodyTop[1], bodyTop[2], bodyBot[0], bodyBot[1], bodyBot[2], 255);
    FillRoundedRectVerticalGradient(bmp, X(9.0f), Y(27.0f), X(132.0f), Y(67.0f), S(4.0f),
        bodyMid[0], bodyMid[1], bodyMid[2], bodyBot[0], bodyBot[1], bodyBot[2], 255);

    FillRoundedRectVerticalGradient(bmp, X(9.0f), Y(10.0f), X(35.0f), Y(18.0f), S(4.0f),
        tabTop[0], tabTop[1], tabTop[2], tabBot[0], tabBot[1], tabBot[2], 255);

    FillRoundedRectVerticalGradient(bmp, X(9.0f), Y(52.0f), X(132.0f), Y(42.0f), S(4.0f),
        shadow[0], shadow[1], shadow[2], shadow[0], shadow[1], shadow[2], 56);

    FillRoundedRect(bmp, X(12.0f), Y(13.0f), X(28.0f), Y(1.4f), S(0.7f),
        highlight[0], highlight[1], highlight[2], 158);
    FillRoundedRect(bmp, X(50.0f), Y(28.5f), X(84.0f), Y(1.2f), S(0.6f),
        highlight[0], highlight[1], highlight[2], 102);

    return bmp;
}

BitmapRGBA ThumbnailRenderer::RasterizeFolderSvg(const std::string& resolved, uint32_t w, uint32_t h, float hoverBrightness) {
    NSVGimage* image = nsvgParseFromFile(resolved.c_str(), "px", 96.0f);
    if (!image) return {};

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(image);
        return {};
    }

    constexpr int kSSAA = 4;
    const int rasterW = static_cast<int>(w) * kSSAA;
    const int rasterH = static_cast<int>(h) * kSSAA;
    const float scale = std::min(
        static_cast<float>(rasterW) / static_cast<float>(image->width),
        static_cast<float>(rasterH) / static_cast<float>(image->height));
    const float offsetX = (static_cast<float>(rasterW) - image->width * scale) * 0.5f;
    const float offsetY = (static_cast<float>(rasterH) - image->height * scale) * 0.5f;

    std::vector<uint8_t> rasterData(static_cast<size_t>(rasterW) * static_cast<size_t>(rasterH) * 4, 0);
    nsvgRasterize(rast, image, offsetX, offsetY, scale, rasterData.data(), rasterW, rasterH, rasterW * 4);
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    BitmapRGBA bmp;
    bmp.width = w;
    bmp.height = h;
    bmp.pixels.assign(static_cast<size_t>(w) * h * 4, 0);

    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            uint32_t sumR = 0, sumG = 0, sumB = 0, sumA = 0;
            for (int dy = 0; dy < kSSAA; ++dy) {
                for (int dx = 0; dx < kSSAA; ++dx) {
                    const int sx = static_cast<int>(x) * kSSAA + dx;
                    const int sy = static_cast<int>(y) * kSSAA + dy;
                    const size_t srcIdx = (static_cast<size_t>(sy) * static_cast<size_t>(rasterW) + static_cast<size_t>(sx)) * 4;
                    const uint8_t a = rasterData[srcIdx + 3];
                    sumR += rasterData[srcIdx] * a;
                    sumG += rasterData[srcIdx + 1] * a;
                    sumB += rasterData[srcIdx + 2] * a;
                    sumA += a;
                }
            }
            const size_t dstIdx = (static_cast<size_t>(y) * w + x) * 4;
            if (sumA == 0) continue;
            const uint8_t outA = static_cast<uint8_t>(sumA / (kSSAA * kSSAA));
            bmp.pixels[dstIdx]     = BrightenChannel(static_cast<uint8_t>(sumR / sumA), hoverBrightness);
            bmp.pixels[dstIdx + 1] = BrightenChannel(static_cast<uint8_t>(sumG / sumA), hoverBrightness);
            bmp.pixels[dstIdx + 2] = BrightenChannel(static_cast<uint8_t>(sumB / sumA), hoverBrightness);
            bmp.pixels[dstIdx + 3] = outA;
        }
    }

    return bmp;
}

BitmapRGBA ThumbnailRenderer::RenderContentBrowserFolder(uint32_t heightPx, float hoverBrightness) {
    const uint32_t h = std::max(16u, SnapFolderRasterHeight(heightPx));
    const uint32_t w = std::max(16u, static_cast<uint32_t>(std::round(static_cast<float>(h) * kFolderAspectRatio)));

    const std::string svgPath = ResolveFolderSvgPath();
    if (!svgPath.empty()) {
        const BitmapRGBA svgBmp = RasterizeFolderSvg(ResolvePath(svgPath), w, h, hoverBrightness);
        if (!svgBmp.pixels.empty()) {
            return svgBmp;
        }
        HE_WARN("[ContentBrowser] Failed to rasterize folder SVG; using procedural artwork.");
    } else {
        static bool s_ReportedMissingSvg = false;
        if (!s_ReportedMissingSvg) {
            HE_WARN("[ContentBrowser] Folder SVG not found (content-browser-folder.svg); using procedural artwork.");
            s_ReportedMissingSvg = true;
        }
    }

    return RenderContentBrowserFolderProcedural(w, h, hoverBrightness);
}

} // namespace we::editor::contentbrowser
