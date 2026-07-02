#pragma once

#include "Registry/AssetTypes.hpp"
#include <array>
#include <cstdint>
#include <functional>
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
    static BitmapRGBA RenderContentBrowserFolder(uint32_t heightPx, float hoverBrightness = 0.0f, bool opened = false,
        uint32_t widthPx = 0);
    static BitmapRGBA RenderContentBrowserBlueprint(uint32_t heightPx, float hoverBrightness = 0.0f);

    static constexpr float kFolderAspectRatio = 1.138f; // width / height (Assets/Editor/Folder.svg)
    static constexpr float kFolderOpenAspectRatio = 1.169f; // width / height (Assets/Editor/Folder_Open.svg)
    static constexpr float kBlueprintAspectRatio = 0.947f; // width / height (Assets/Editor/Visual_Graph.svg)

    static BitmapRGBA FitIntoCell(const BitmapRGBA& source, uint32_t cellW, uint32_t cellH);

private:
    static void AlphaBlend(BitmapRGBA& bmp, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static float RoundedRectCoverage(float px, float py, float rx, float ry, float rw, float rh, float radius);
    static void FillRoundedRect(BitmapRGBA& bmp, float x, float y, float w, float h, float radius,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void FillRoundedRectVerticalGradient(BitmapRGBA& bmp, float x, float y, float w, float h, float radius,
        uint8_t rTop, uint8_t gTop, uint8_t bTop, uint8_t rBot, uint8_t gBot, uint8_t bBot, uint8_t a);
    static void SetPixel(BitmapRGBA& bmp, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void FillRect(BitmapRGBA& bmp, int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void DrawCircle(BitmapRGBA& bmp, int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void Blit(const BitmapRGBA& src, BitmapRGBA& dst, int dstX, int dstY, int dstW, int dstH);
    static BitmapRGBA CreateEmpty(uint32_t size);
    static BitmapRGBA RenderContentBrowserFolderProcedural(uint32_t w, uint32_t h, float hoverBrightness);
    static BitmapRGBA RenderContentBrowserBlueprintProcedural(uint32_t w, uint32_t h, float hoverBrightness);
    static BitmapRGBA RasterizeMonochromeSvg(const std::string& resolved, uint32_t w, uint32_t h, float hoverBrightness,
        const std::function<std::array<uint8_t, 3>(float verticalT)>& sampleColor);
    static BitmapRGBA TrimTransparentPadding(const BitmapRGBA& src, uint32_t targetW, uint32_t targetH);
};

} // namespace we::editor::contentbrowser
