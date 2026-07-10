#pragma once

#include "Text/Core/Types.h"
#include "Text/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::text {

enum class AtlasFormat : uint8_t {
    Msdf = 0,
    Mtsdf = 1,
    Sdf = 2,
    Bitmap = 3,
};

enum class GraphicsApi : uint8_t {
    Vulkan = 0,
    DirectX12 = 1,
    Metal = 2,
    OpenGL = 3,
    Unknown = 255,
};

struct TEXT_API FontMetrics {
    float ascender = 0.0f;
    float descender = 0.0f;
    float lineHeight = 0.0f;
    float underlinePosition = 0.0f;
    float underlineThickness = 0.0f;
    float bakeSizePx = 0.0f;
    float msdfPixelRange = 4.0f;
    std::string familyName;
    std::string styleName;
    uint16_t weight = 400;
    bool italic = false;
};

struct TEXT_API GlyphMetrics {
    Codepoint codepoint = 0;
    Rect bounds{};
    Vec2 bearing{};
    float advance = 0.0f;
    RectUv atlasUv{};
    uint16_t atlasPage = 0;
    bool isWhitespace = false;
    bool hasDrawableQuad = false;
};

struct TEXT_API KerningPair {
    Codepoint left = 0;
    Codepoint right = 0;
    float advance = 0.0f;
};

struct TEXT_API UnicodeRange {
    Codepoint start = 0;
    Codepoint end = 0;
};

struct TEXT_API AtlasPage {
    uint32_t width = 0;
    uint32_t height = 0;
    AtlasFormat format = AtlasFormat::Msdf;
    std::vector<uint8_t> rgba;
};

} // namespace we::runtime::text
