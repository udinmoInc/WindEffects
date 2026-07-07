#include "Services/ThumbnailRenderer.h"
#include "Registry/AssetTypeResolver.h"
#include "Core/Theme.h"
#include "Core/Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define NANOSVG_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4456 4244)
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>
#pragma warning(pop)

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <vector>

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
        "Assets/Editor/Folder.svg",
        "../Assets/Editor/Folder.svg",
        "../../Assets/Editor/Folder.svg",
        "Assets/Icons/content-browser-folder.svg",
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

std::string ResolveFolderOpenSvgPath() {
    const char* candidates[] = {
        "Assets/Editor/Folder_Open.svg",
        "../Assets/Editor/Folder_Open.svg",
        "../../Assets/Editor/Folder_Open.svg",
    };
    for (const char* path : candidates) {
        if (std::filesystem::exists(path)) return path;
    }
    return {};
}

std::string ResolveBlueprintSvgPath() {
    const char* candidates[] = {
        "Assets/Editor/Visual_Graph.svg",
        "../Assets/Editor/Visual_Graph.svg",
        "../../Assets/Editor/Visual_Graph.svg",
    };
    for (const char* path : candidates) {
        if (std::filesystem::exists(path)) return path;
    }
    return {};
}

std::array<uint8_t, 3> LerpRgb(const std::array<uint8_t, 3>& a, const std::array<uint8_t, 3>& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return {
        static_cast<uint8_t>(a[0] + (b[0] - a[0]) * t),
        static_cast<uint8_t>(a[1] + (b[1] - a[1]) * t),
        static_cast<uint8_t>(a[2] + (b[2] - a[2]) * t),
    };
}

uint32_t SnapFolderRasterHeight(uint32_t heightPx) {
    // Keep small tree icons at their requested size so we don't downscale a 64px texture to ~15px.
    if (heightPx <= 48u) return std::max(16u, heightPx);
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

std::array<uint8_t, 3> SampleFolderThemeColor(float t, float hoverBrightness) {
    const we::UI::Theme& theme = we::UI::Theme::Get();
    const auto base = ThemeRgb(theme.ContentBrowserFolderPrimary, hoverBrightness);

    // Single consistent warm yellow/gold material with subtle 5-10% top-to-bottom gradient
    const float topFactor = 1.05f;
    const float botFactor = 0.95f;
    const float factor = topFactor + (botFactor - topFactor) * std::clamp(t, 0.0f, 1.0f);

    return {
        static_cast<uint8_t>(std::min(255, static_cast<int>(base[0] * factor))),
        static_cast<uint8_t>(std::min(255, static_cast<int>(base[1] * factor))),
        static_cast<uint8_t>(std::min(255, static_cast<int>(base[2] * factor)))
    };
}

void ApplyFolderEdgeOutline(BitmapRGBA& bmp, const std::array<uint8_t, 3>& edgeRgb) {
    if (bmp.width == 0 || bmp.height == 0 || bmp.pixels.empty()) return;

    const uint32_t w = bmp.width;
    const uint32_t h = bmp.height;
    std::vector<uint8_t> alpha(static_cast<size_t>(w) * h, 0);
    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            alpha[static_cast<size_t>(y) * w + x] = bmp.pixels[(static_cast<size_t>(y) * w + x) * 4 + 3];
        }
    }

    auto isEdge = [&](uint32_t x, uint32_t y) {
        if (alpha[static_cast<size_t>(y) * w + x] <= 8) return false;
        constexpr int kDirs[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
        for (const auto& dir : kDirs) {
            const int nx = static_cast<int>(x) + dir[0];
            const int ny = static_cast<int>(y) + dir[1];
            if (nx < 0 || ny < 0 || nx >= static_cast<int>(w) || ny >= static_cast<int>(h)) return true;
            if (alpha[static_cast<size_t>(ny) * w + static_cast<uint32_t>(nx)] <= 8) return true;
        }
        return false;
    };

    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            if (!isEdge(x, y)) continue;
            const size_t idx = (static_cast<size_t>(y) * w + x) * 4;
            const float blend = 0.72f;
            bmp.pixels[idx]     = static_cast<uint8_t>(edgeRgb[0] * blend + bmp.pixels[idx] * (1.0f - blend));
            bmp.pixels[idx + 1] = static_cast<uint8_t>(edgeRgb[1] * blend + bmp.pixels[idx + 1] * (1.0f - blend));
            bmp.pixels[idx + 2] = static_cast<uint8_t>(edgeRgb[2] * blend + bmp.pixels[idx + 2] * (1.0f - blend));
        }
    }
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
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

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
    const BitmapRGBA icon = RenderContentBrowserBlueprint(kThumbnailSize, 0.0f);
    if (icon.pixels.empty()) {
        return RenderGenericIcon(AssetType::Blueprint);
    }
    return FitIntoCell(icon, kThumbnailSize, kThumbnailSize);
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
    // Clean, flat, modern folder with single consistent warm yellow/gold material
    BitmapRGBA bmp;
    bmp.width = w;
    bmp.height = h;
    bmp.pixels.assign(static_cast<size_t>(w) * h * 4, 0);

    const we::UI::Theme& theme = we::UI::Theme::Get();
    const auto shadow = ThemeRgb(theme.ContentBrowserFolderShadow, hoverBrightness);
    const auto edge = ThemeRgb(theme.ContentBrowserFolderEdge, hoverBrightness);
    const auto tabTop = ThemeRgb(theme.ContentBrowserFolderHighlight, hoverBrightness);
    const auto tabBot = ThemeRgb(theme.ContentBrowserFolderTab, hoverBrightness);
    const auto bodyTop = ThemeRgb(theme.ContentBrowserFolderTab, hoverBrightness);
    const auto bodyMid = ThemeRgb(theme.ContentBrowserFolderPrimary, hoverBrightness);
    const auto bodyBot = ThemeRgb(theme.ContentBrowserFolderBody, hoverBrightness);
    const auto highlight = ThemeRgb(theme.ContentBrowserFolderHighlight, hoverBrightness);

    constexpr float kRefW = 146.0f;
    constexpr float kRefH = 100.0f;
    const float sx = static_cast<float>(w) / kRefW;
    const float sy = static_cast<float>(h) / kRefH;
    const float scale = std::min(sx, sy);
    const auto X = [sx](float v) { return v * sx; };
    const auto Y = [sy](float v) { return v * sy; };
    const auto S = [scale](float v) { return v * scale; };

    // Very subtle cast shadow
    const float castX = X(1.5f);
    const float castY = Y(1.5f);
    FillRoundedRect(bmp, X(9.0f) + castX, Y(27.0f) + castY, X(132.0f), Y(67.0f), S(4.0f),
        shadow[0], shadow[1], shadow[2], 35);
    FillRoundedRect(bmp, X(9.0f) + castX, Y(10.0f) + castY, X(35.0f), Y(18.0f), S(4.0f),
        shadow[0], shadow[1], shadow[2], 35);

    // Gradient for body
    FillRoundedRectVerticalGradient(bmp, X(9.0f), Y(27.0f), X(132.0f), Y(67.0f), S(4.0f),
        bodyTop[0], bodyTop[1], bodyTop[2], bodyBot[0], bodyBot[1], bodyBot[2], 255);

    // Gradient for tab (distinct from body)
    FillRoundedRectVerticalGradient(bmp, X(9.0f), Y(10.0f), X(35.0f), Y(18.0f), S(4.0f),
        tabTop[0], tabTop[1], tabTop[2], tabBot[0], tabBot[1], tabBot[2], 255);

    // Very subtle highlight on tab edge only
    FillRoundedRect(bmp, X(12.0f), Y(13.0f), X(28.0f), Y(1.2f), S(0.6f),
        highlight[0], highlight[1], highlight[2], 80);

    // Thin outline for separation from background
    ApplyFolderEdgeOutline(bmp, edge);
    return bmp;
}

BitmapRGBA ThumbnailRenderer::RasterizeMonochromeSvg(const std::string& resolved, uint32_t w, uint32_t h, float hoverBrightness,
    const std::function<std::array<uint8_t, 3>(float verticalT)>& sampleColor) {
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

    std::vector<uint8_t> alphaMask(static_cast<size_t>(w) * h, 0);
    uint32_t minOpaqueY = h;
    uint32_t maxOpaqueY = 0;
    bool hasOpaque = false;
    bool useThemeTint = true;
    uint32_t colorEnergy = 0;
    uint32_t opaqueCount = 0;

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
            if (sumA == 0) continue;

            const uint8_t outA = static_cast<uint8_t>(sumA / (kSSAA * kSSAA));
            const size_t idx = static_cast<size_t>(y) * w + x;
            alphaMask[idx] = outA;
            minOpaqueY = std::min(minOpaqueY, y);
            maxOpaqueY = std::max(maxOpaqueY, y);
            hasOpaque = true;

            const uint8_t r = static_cast<uint8_t>(sumR / sumA);
            const uint8_t g = static_cast<uint8_t>(sumG / sumA);
            const uint8_t b = static_cast<uint8_t>(sumB / sumA);
            colorEnergy += static_cast<uint32_t>(r) + g + b;
            ++opaqueCount;
            if (r > 48 || g > 48 || b > 48) {
                useThemeTint = false;
            }
        }
    }

    if (!hasOpaque) return bmp;
    if (opaqueCount > 0 && colorEnergy / opaqueCount > 90) {
        useThemeTint = false;
    }

    const float opaqueSpan = std::max(1.0f, static_cast<float>(maxOpaqueY - minOpaqueY));

    for (uint32_t y = 0; y < h; ++y) {
        for (uint32_t x = 0; x < w; ++x) {
            const size_t idx = static_cast<size_t>(y) * w + x;
            const uint8_t outA = alphaMask[idx];
            if (outA == 0) continue;

            const size_t dstIdx = idx * 4;
            bmp.pixels[dstIdx + 3] = outA;

            if (useThemeTint) {
                const float t = (static_cast<float>(y) - static_cast<float>(minOpaqueY)) / opaqueSpan;
                const auto rgb = sampleColor(t);
                bmp.pixels[dstIdx] = rgb[0];
                bmp.pixels[dstIdx + 1] = rgb[1];
                bmp.pixels[dstIdx + 2] = rgb[2];
            } else {
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
                bmp.pixels[dstIdx]     = BrightenChannel(static_cast<uint8_t>(sumR / sumA), hoverBrightness);
                bmp.pixels[dstIdx + 1] = BrightenChannel(static_cast<uint8_t>(sumG / sumA), hoverBrightness);
                bmp.pixels[dstIdx + 2] = BrightenChannel(static_cast<uint8_t>(sumB / sumA), hoverBrightness);
            }
        }
    }

    return bmp;
}

BitmapRGBA ThumbnailRenderer::TrimTransparentPadding(const BitmapRGBA& src, uint32_t targetW, uint32_t targetH) {
    if (src.width == 0 || src.height == 0 || src.pixels.empty()) return src;

    uint32_t minX = src.width;
    uint32_t minY = src.height;
    uint32_t maxX = 0;
    uint32_t maxY = 0;
    bool hasOpaque = false;

    for (uint32_t y = 0; y < src.height; ++y) {
        for (uint32_t x = 0; x < src.width; ++x) {
            const size_t idx = (static_cast<size_t>(y) * src.width + x) * 4 + 3;
            if (src.pixels[idx] <= 8) continue;
            hasOpaque = true;
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
        }
    }

    if (!hasOpaque) return src;

    const uint32_t pad = 1;
    minX = minX > pad ? minX - pad : 0;
    minY = minY > pad ? minY - pad : 0;
    maxX = std::min(src.width - 1, maxX + pad);
    maxY = std::min(src.height - 1, maxY + pad);

    const uint32_t cropW = maxX - minX + 1;
    const uint32_t cropH = maxY - minY + 1;

    BitmapRGBA crop;
    crop.width = cropW;
    crop.height = cropH;
    crop.pixels.resize(static_cast<size_t>(cropW) * cropH * 4);
    for (uint32_t y = 0; y < cropH; ++y) {
        for (uint32_t x = 0; x < cropW; ++x) {
            const size_t srcIdx = (static_cast<size_t>(minY + y) * src.width + static_cast<size_t>(minX + x)) * 4;
            const size_t dstIdx = (static_cast<size_t>(y) * cropW + x) * 4;
            crop.pixels[dstIdx] = src.pixels[srcIdx];
            crop.pixels[dstIdx + 1] = src.pixels[srcIdx + 1];
            crop.pixels[dstIdx + 2] = src.pixels[srcIdx + 2];
            crop.pixels[dstIdx + 3] = src.pixels[srcIdx + 3];
        }
    }

    if (cropW <= targetW && cropH <= targetH) {
        BitmapRGBA result;
        result.width = targetW;
        result.height = targetH;
        result.pixels.assign(static_cast<size_t>(targetW) * targetH * 4, 0);
        const int offX = static_cast<int>((targetW - cropW) / 2);
        const int offY = static_cast<int>((targetH - cropH) / 2);
        for (uint32_t y = 0; y < cropH; ++y) {
            for (uint32_t x = 0; x < cropW; ++x) {
                const size_t srcIdx = (static_cast<size_t>(y) * cropW + x) * 4;
                const size_t dstIdx = (static_cast<size_t>(offY + static_cast<int>(y)) * targetW
                    + static_cast<size_t>(offX + static_cast<int>(x))) * 4;
                result.pixels[dstIdx] = crop.pixels[srcIdx];
                result.pixels[dstIdx + 1] = crop.pixels[srcIdx + 1];
                result.pixels[dstIdx + 2] = crop.pixels[srcIdx + 2];
                result.pixels[dstIdx + 3] = crop.pixels[srcIdx + 3];
            }
        }
        return result;
    }

    return FitIntoCell(crop, targetW, targetH);
}

BitmapRGBA ThumbnailRenderer::RenderContentBrowserBlueprintProcedural(uint32_t w, uint32_t h, float hoverBrightness) {
    BitmapRGBA bmp;
    bmp.width = w;
    bmp.height = h;
    bmp.pixels.assign(static_cast<size_t>(w) * h * 4, 0);

    const we::UI::Theme& theme = we::UI::Theme::Get();
    const auto tint = ThemeRgb(theme.IconMuted, hoverBrightness);
    const float cx = static_cast<float>(w) * 0.5f;
    const float cy = static_cast<float>(h) * 0.5f;
    const float boxW = static_cast<float>(w) * 0.72f;
    const float boxH = static_cast<float>(h) * 0.76f;
    FillRoundedRect(bmp, cx - boxW * 0.5f, cy - boxH * 0.5f, boxW, boxH, 6.0f, tint[0], tint[1], tint[2], 255);
    return bmp;
}

BitmapRGBA ThumbnailRenderer::RenderContentBrowserFolder(
    uint32_t heightPx, float hoverBrightness, bool opened, uint32_t widthPx) {
    const uint32_t h = std::max(16u, SnapFolderRasterHeight(heightPx));
    const float aspectRatio = opened ? kFolderOpenAspectRatio : kFolderAspectRatio;
    const uint32_t w = widthPx > 0
        ? std::max(16u, widthPx)
        : std::max(16u, static_cast<uint32_t>(std::round(static_cast<float>(h) * aspectRatio)));

    const we::UI::Theme& theme = we::UI::Theme::Get();
    const auto edge = ThemeRgb(theme.ContentBrowserFolderEdge, hoverBrightness);

    const std::string svgPath = opened ? ResolveFolderOpenSvgPath() : ResolveFolderSvgPath();
    if (!svgPath.empty()) {
        BitmapRGBA svgBmp = RasterizeMonochromeSvg(ResolvePath(svgPath), w, h, hoverBrightness,
            [&](float t) { return SampleFolderThemeColor(t, hoverBrightness); });
        if (!svgBmp.pixels.empty()) {
            svgBmp = TrimTransparentPadding(svgBmp, w, h);
            ApplyFolderEdgeOutline(svgBmp, edge);
            return svgBmp;
        }
        HE_WARN("[ContentBrowser] Failed to rasterize folder SVG; using procedural artwork.");
    } else {
        static bool s_ReportedMissingSvg = false;
        if (!s_ReportedMissingSvg) {
            HE_WARN(std::string("[ContentBrowser] ") + (opened ? "Open" : "Closed")
                + " folder SVG not found; using procedural artwork.");
            s_ReportedMissingSvg = true;
        }
    }

    return RenderContentBrowserFolderProcedural(w, h, hoverBrightness);
}

BitmapRGBA ThumbnailRenderer::RenderContentBrowserBlueprint(uint32_t heightPx, float hoverBrightness) {
    const uint32_t h = std::max(16u, SnapFolderRasterHeight(heightPx));
    const uint32_t w = std::max(16u, static_cast<uint32_t>(std::round(static_cast<float>(h) * kBlueprintAspectRatio)));

    const std::string svgPath = ResolveBlueprintSvgPath();
    if (!svgPath.empty()) {
        const we::UI::Theme& theme = we::UI::Theme::Get();
        const auto flatTint = ThemeRgb(theme.IconMuted, hoverBrightness);
        const BitmapRGBA svgBmp = RasterizeMonochromeSvg(ResolvePath(svgPath), w, h, hoverBrightness,
            [&](float) { return flatTint; });
        if (!svgBmp.pixels.empty()) {
            return TrimTransparentPadding(svgBmp, w, h);
        }
        HE_WARN("[ContentBrowser] Failed to rasterize blueprint SVG; using procedural artwork.");
    } else {
        static bool s_ReportedMissingSvg = false;
        if (!s_ReportedMissingSvg) {
            HE_WARN("[ContentBrowser] Blueprint SVG not found (Assets/Editor/Visual_Graph.svg); using procedural artwork.");
            s_ReportedMissingSvg = true;
        }
    }

    return RenderContentBrowserBlueprintProcedural(w, h, hoverBrightness);
}

} // namespace we::editor::contentbrowser
