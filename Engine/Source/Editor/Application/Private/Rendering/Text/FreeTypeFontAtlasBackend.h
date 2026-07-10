#pragma once

#include "Rendering/Text/IFontAtlasBackend.h"

#include <unordered_map>
#include <unordered_set>
#include <memory>

struct FT_LibraryRec_;
struct FT_FaceRec_;

namespace we::UI::Text {

class FreeTypeFontAtlasBackend final : public IFontAtlasBackend {
public:
    FreeTypeFontAtlasBackend();
    ~FreeTypeFontAtlasBackend() override;

    bool Initialize(const FontAtlasBakeRequest& request) override;
    void Shutdown() override;

    bool EnsureGlyphs(const std::vector<uint32_t>& codepoints) override;
    bool GetGlyphQuad(uint32_t codepoint, float* cursorX, float* cursorY, GlyphInfo& outQuad) const override;
    bool GetGlyphQuadAt(uint32_t codepoint, float penX, float penY, GlyphInfo& outQuad) const override;
    float GetGlyphAdvance(uint32_t codepoint) const override;
    bool GetFontFaces(FontFaceHandles& outFaces) const override;
    float MeasureText(std::string_view utf8Text, float fontSize) const override;

    const std::vector<uint8_t>& GetAtlasPixels() const override { return m_AtlasRgba; }
    int GetAtlasWidth() const override { return m_AtlasWidth; }
    int GetAtlasHeight() const override { return m_AtlasHeight; }
    float GetEmSize() const override { return m_EmSizePx; }
    float GetMsdfPixelRange() const override { return m_MsdfPixelRange; }
    int GetGlyphCount() const override { return static_cast<int>(m_Codepoints.size()); }
    bool IsDirty() const override { return m_Dirty; }
    void ClearDirty() override { m_Dirty = false; }

private:
    bool LoadFontFaces(const FontAtlasBakeRequest& request);
    bool RebuildAtlas();

    struct GlyphPlacement {
        bool isWhitespace = false;
        bool hasDrawableQuad = false;
        float advance = 0.0f;
        float planeLeft = 0.0f;
        float planeBottom = 0.0f;
        float planeRight = 0.0f;
        float planeTop = 0.0f;
        float atlasLeft = 0.0f;
        float atlasBottom = 0.0f;
        float atlasRight = 0.0f;
        float atlasTop = 0.0f;
    };

    std::string m_PrimaryFontPath;
    std::string m_FallbackFontPath;
    float m_EmSizePx = 16.0f;
    float m_MsdfPixelRange = 0.0f;
    int m_AtlasWidth = 1024;
    int m_AtlasHeight = 1024;

    FT_LibraryRec_* m_FreeTypeLibrary = nullptr;
    FT_FaceRec_* m_PrimaryFace = nullptr;
    FT_FaceRec_* m_FallbackFace = nullptr;

    std::unordered_set<uint32_t> m_Codepoints;
    std::unordered_map<uint32_t, GlyphPlacement> m_PlacementsByCodepoint;
    std::vector<uint8_t> m_AtlasRgba;
    bool m_Dirty = false;
};

std::unique_ptr<IFontAtlasBackend> CreateFreeTypeFontAtlasBackend();

} // namespace we::UI::Text
