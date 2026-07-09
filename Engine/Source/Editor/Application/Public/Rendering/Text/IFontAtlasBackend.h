#pragma once

#include "Application/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::UI {

struct GlyphInfo;

} // namespace we::UI

namespace we::UI::Text {

struct FontAtlasBakeRequest {
    std::string primaryFontPath;
    std::string fallbackFontPath;
    float emSizePx = 16.0f;
    float msdfPixelRange = 4.0f;
    int atlasWidth = 1024;
    int atlasHeight = 1024;
};

struct FontAtlasBakeResult {
    std::vector<uint8_t> rgbaPixels;
    int atlasWidth = 0;
    int atlasHeight = 0;
    float emSizePx = 0.0f;
    float msdfPixelRange = 0.0f;
    bool success = false;
};

// Backend-independent font atlas generation (FreeType + MSDF implementation behind this API).
class APPLICATION_API IFontAtlasBackend {
public:
    virtual ~IFontAtlasBackend() = default;

    virtual bool Initialize(const FontAtlasBakeRequest& request) = 0;
    virtual void Shutdown() = 0;

    virtual bool EnsureGlyphs(const std::vector<uint32_t>& codepoints) = 0;
    virtual bool GetGlyphQuad(uint32_t codepoint, float* cursorX, float* cursorY, GlyphInfo& outQuad) const = 0;
    virtual float MeasureText(std::string_view utf8Text, float fontSize) const = 0;

    virtual const std::vector<uint8_t>& GetAtlasPixels() const = 0;
    virtual int GetAtlasWidth() const = 0;
    virtual int GetAtlasHeight() const = 0;
    virtual float GetEmSize() const = 0;
    virtual float GetMsdfPixelRange() const = 0;
    virtual int GetGlyphCount() const = 0;
    virtual bool IsDirty() const = 0;
    virtual void ClearDirty() = 0;
};

} // namespace we::UI::Text
