#pragma once

#include "Registry/AssetTypes.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace we::editor::contentbrowser {

struct BitmapRGBA {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> pixels;
};

class ThumbnailRenderer {
public:
    static constexpr uint32_t kThumbnailSize = 128;

    static BitmapRGBA Render(const AssetRecord& asset);
    static BitmapRGBA LoadImageFile(const std::string& path, uint32_t targetSize);
    static BitmapRGBA RenderMaterialSphere(const AssetRecord& asset, bool isInstance);
    static BitmapRGBA RenderMeshPreview(const AssetRecord& asset, bool skeletal);
    static BitmapRGBA RenderBlueprintPreview(const AssetRecord& asset);
    static BitmapRGBA RenderAudioWaveform(const AssetRecord& asset);
    static BitmapRGBA RenderFontSample(const AssetRecord& asset);
    static BitmapRGBA RenderScriptIcon(const AssetRecord& asset);
    static BitmapRGBA RenderScenePreview(const AssetRecord& asset);
    static BitmapRGBA RenderGenericIcon(AssetType type);

    // Dedicated Content Browser folder tile (filled, warm gray/gold, no embedded previews).
    static BitmapRGBA RenderContentBrowserFolderThumbnail(uint32_t size = kThumbnailSize);

    static BitmapRGBA FitIntoCell(const BitmapRGBA& source, uint32_t cellW, uint32_t cellH);

private:
    static void SetPixel(BitmapRGBA& bmp, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void AlphaBlendPixel(BitmapRGBA& bmp, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void FillRect(BitmapRGBA& bmp, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void FillRoundedRect(BitmapRGBA& bmp, int x, int y, int w, int h, int radius,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void DrawCircle(BitmapRGBA& bmp, int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void Blit(const BitmapRGBA& src, BitmapRGBA& dst, int dstX, int dstY, int dstW, int dstH);
    static BitmapRGBA CreateEmpty(uint32_t size);
};

} // namespace we::editor::contentbrowser
