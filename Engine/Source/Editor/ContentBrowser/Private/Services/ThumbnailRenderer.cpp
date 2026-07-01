#include "Services/ThumbnailRenderer.hpp"
#include "Registry/AssetTypeResolver.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

#include <algorithm>
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

bool IsInsideRoundedRect(int px, int py, int x, int y, int w, int h, int radius) {
    if (px < x || py < y || px >= x + w || py >= y + h) return false;
    radius = std::max(0, std::min(radius, std::min(w, h) / 2));
    int dx = 0, dy = 0;
    if (px < x + radius && py < y + radius) {
        dx = (x + radius) - px; dy = (y + radius) - py;
    } else if (px >= x + w - radius && py < y + radius) {
        dx = px - (x + w - radius - 1); dy = (y + radius) - py;
    } else if (px < x + radius && py >= y + h - radius) {
        dx = (x + radius) - px; dy = py - (y + h - radius - 1);
    } else if (px >= x + w - radius && py >= y + h - radius) {
        dx = px - (x + w - radius - 1); dy = py - (y + h - radius - 1);
    }
    return dx == 0 || dy == 0 || dx * dx + dy * dy <= radius * radius;
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

BitmapRGBA ThumbnailRenderer::RenderContentBrowserFolderThumbnail(uint32_t size) {
    auto bmp = CreateEmpty(size);
    const int s = static_cast<int>(size);

    // Match Content Browser panel background — folder floats on flat dark surface.
    FillRect(bmp, 0, 0, s, s, 28, 28, 28, 255);

    const float scale = static_cast<float>(size) / 128.0f;
    const auto sc = [scale](int v) { return static_cast<int>(v * scale + 0.5f); };

    // UE5 proportions: wide body, small left tab, unified thick silhouette.
    const int bodyX = sc(16);
    const int bodyY = sc(44);
    const int bodyW = sc(96);
    const int bodyH = sc(62);
    const int bodyR = sc(7);
    const int tabX = sc(20);
    const int tabY = sc(30);
    const int tabW = sc(40);
    const int tabH = sc(18);
    const int tabR = sc(5);

    const int folderTop = tabY;
    const int folderBottom = bodyY + bodyH;
    const int folderHeight = std::max(1, folderBottom - folderTop);

    auto insideFolder = [&](int lx, int ly) {
        return IsInsideRoundedRect(lx, ly, tabX, tabY, tabW, tabH, tabR)
            || IsInsideRoundedRect(lx, ly, bodyX, bodyY, bodyW, bodyH, bodyR);
    };

    // Soft drop shadow — blurred offset copies of the folder silhouette.
    for (int layer = sc(10); layer >= sc(2); --layer) {
        const int oy = layer;
        const uint8_t alpha = static_cast<uint8_t>(std::clamp(34 - layer * 2, 4, 30));
        for (int py = tabY + oy - 2; py < folderBottom + oy + sc(4); ++py) {
            for (int px = bodyX - sc(2); px < bodyX + bodyW + sc(2); ++px) {
                const int lx = px;
                const int ly = py - oy;
                if (!insideFolder(lx, ly)) continue;
                AlphaBlendPixel(bmp, px, py, 0, 0, 0, alpha);
            }
        }
    }

    // Vertical warm gold gradient across the filled folder (lighter top, darker bottom).
    constexpr uint8_t topR = 201, topG = 181, topB = 141;
    constexpr uint8_t botR = 154, botG = 134, botB = 96;

    for (int py = folderTop; py < folderBottom; ++py) {
        const float t = static_cast<float>(py - folderTop) / static_cast<float>(folderHeight - 1);
        const uint8_t r = static_cast<uint8_t>(topR + (botR - topR) * t);
        const uint8_t g = static_cast<uint8_t>(topG + (botG - topG) * t);
        const uint8_t b = static_cast<uint8_t>(topB + (botB - topB) * t);
        for (int px = bodyX - 1; px < bodyX + bodyW + 1; ++px) {
            if (!insideFolder(px, py)) continue;
            SetPixel(bmp, px, py, r, g, b, 255);
        }
    }

    // Thin top-edge highlight on body (UE5 subtle sheen).
    const int highlightY = bodyY + sc(3);
    constexpr uint8_t hiR = 228, hiG = 212, hiB = 172;
    for (int px = bodyX + sc(8); px < bodyX + bodyW - sc(8); ++px) {
        if (insideFolder(px, highlightY)) {
            SetPixel(bmp, px, highlightY, hiR, hiG, hiB, 255);
        }
    }
    for (int px = tabX + sc(5); px < tabX + tabW - sc(5); ++px) {
        const int hy = tabY + sc(2);
        if (insideFolder(px, hy)) {
            SetPixel(bmp, px, hy, hiR, hiG, hiB, 220);
        }
    }

    return bmp;
}

void ThumbnailRenderer::AlphaBlendPixel(BitmapRGBA& bmp, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || y < 0 || x >= static_cast<int>(bmp.width) || y >= static_cast<int>(bmp.height) || a == 0) return;
    const size_t idx = (static_cast<size_t>(y) * bmp.width + static_cast<size_t>(x)) * 4;
    const float srcA = a / 255.0f;
    const float dstA = bmp.pixels[idx + 3] / 255.0f;
    const float outA = srcA + dstA * (1.0f - srcA);
    if (outA <= 0.0f) return;
    bmp.pixels[idx]     = static_cast<uint8_t>((r * srcA + bmp.pixels[idx]     * dstA * (1.0f - srcA)) / outA);
    bmp.pixels[idx + 1] = static_cast<uint8_t>((g * srcA + bmp.pixels[idx + 1] * dstA * (1.0f - srcA)) / outA);
    bmp.pixels[idx + 2] = static_cast<uint8_t>((b * srcA + bmp.pixels[idx + 2] * dstA * (1.0f - srcA)) / outA);
    bmp.pixels[idx + 3] = static_cast<uint8_t>(outA * 255.0f);
}

void ThumbnailRenderer::FillRoundedRect(BitmapRGBA& bmp, int x, int y, int w, int h, int radius,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    radius = std::max(0, std::min(radius, std::min(w, h) / 2));
    for (int py = y; py < y + h; ++py) {
        for (int px = x; px < x + w; ++px) {
            int dx = 0, dy = 0;
            if (px < x + radius && py < y + radius) {
                dx = (x + radius) - px; dy = (y + radius) - py;
            } else if (px >= x + w - radius && py < y + radius) {
                dx = px - (x + w - radius - 1); dy = (y + radius) - py;
            } else if (px < x + radius && py >= y + h - radius) {
                dx = (x + radius) - px; dy = py - (y + h - radius - 1);
            } else if (px >= x + w - radius && py >= y + h - radius) {
                dx = px - (x + w - radius - 1); dy = py - (y + h - radius - 1);
            }
            if (dx > 0 && dy > 0 && dx * dx + dy * dy > radius * radius) continue;
            if (a == 255) SetPixel(bmp, px, py, r, g, b, a);
            else AlphaBlendPixel(bmp, px, py, r, g, b, a);
        }
    }
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

} // namespace we::editor::contentbrowser
