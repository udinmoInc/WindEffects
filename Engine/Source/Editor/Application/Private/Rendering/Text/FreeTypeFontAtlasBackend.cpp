#include "Rendering/Text/FreeTypeFontAtlasBackend.h"
#include "Rendering/FontAtlas.h"
#include "Rendering/Text/Utf8.h"
#include "Core/Logger.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string_view>

namespace we::UI::Text {

namespace {

std::string CodepointToHex(uint32_t codepoint) {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "U+%04X", codepoint);
    return buffer;
}

std::string ResolveFontPath(const std::string& fontName) {
    if (fontName.empty()) {
        return {};
    }
    if (std::filesystem::exists(fontName)) {
        return fontName;
    }

    const std::string searchPaths[] = {
        "Assets/Fonts/" + fontName,
        "Fonts/" + fontName,
        "../Assets/Fonts/" + fontName,
        "../Fonts/" + fontName,
    };
    for (const auto& path : searchPaths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    return fontName;
}

void SeedCommonCodepoints(std::unordered_set<uint32_t>& codepoints) {
    for (uint32_t cp = 0x20; cp <= 0x7E; ++cp) {
        codepoints.insert(cp);
    }
    for (uint32_t cp = 0xA0; cp <= 0xFF; ++cp) {
        codepoints.insert(cp);
    }
    for (uint32_t cp : {0x2013, 0x2014, 0x2022, 0x2026, 0x20AC, 0xFFFD}) {
        codepoints.insert(cp);
    }
}

class ShelfPacker {
    int m_Width;
    int m_Height;
    int m_CurrentY = 0;
    int m_CurrentX = 0;
    int m_RowHeight = 0;
public:
    ShelfPacker(int w, int h) : m_Width(w), m_Height(h) {}
    bool Pack(int w, int h, int& outX, int& outY) {
        if (m_CurrentX + w > m_Width) {
            m_CurrentX = 0;
            m_CurrentY += m_RowHeight + 1;
            m_RowHeight = 0;
        }
        if (m_CurrentY + h > m_Height) return false;
        outX = m_CurrentX;
        outY = m_CurrentY;
        m_CurrentX += w + 1;
        m_RowHeight = std::max(m_RowHeight, h);
        return true;
    }
};

} // namespace

FreeTypeFontAtlasBackend::FreeTypeFontAtlasBackend() = default;

FreeTypeFontAtlasBackend::~FreeTypeFontAtlasBackend() {
    Shutdown();
}

std::unique_ptr<IFontAtlasBackend> CreateFreeTypeFontAtlasBackend() {
    return std::make_unique<FreeTypeFontAtlasBackend>();
}

bool FreeTypeFontAtlasBackend::Initialize(const FontAtlasBakeRequest& request) {
    Shutdown();

    m_PrimaryFontPath = ResolveFontPath(request.primaryFontPath);
    m_FallbackFontPath = ResolveFontPath(request.fallbackFontPath);
    m_EmSizePx = std::clamp(request.emSizePx, 10.0f, 96.0f);
    m_MsdfPixelRange = 0.0f; // Not used for FreeType
    m_AtlasWidth = std::max(request.atlasWidth, 256);
    m_AtlasHeight = std::max(request.atlasHeight, 256);

    if (!LoadFontFaces(request)) {
        return false;
    }

    SeedCommonCodepoints(m_Codepoints);
    return RebuildAtlas();
}

void FreeTypeFontAtlasBackend::Shutdown() {
    if (m_PrimaryFace) {
        FT_Done_Face(m_PrimaryFace);
        m_PrimaryFace = nullptr;
    }
    if (m_FallbackFace) {
        FT_Done_Face(m_FallbackFace);
        m_FallbackFace = nullptr;
    }
    if (m_FreeTypeLibrary) {
        FT_Done_FreeType(m_FreeTypeLibrary);
        m_FreeTypeLibrary = nullptr;
    }

    m_Codepoints.clear();
    m_PlacementsByCodepoint.clear();
    m_AtlasRgba.clear();
    m_Dirty = false;
}

bool FreeTypeFontAtlasBackend::LoadFontFaces(const FontAtlasBakeRequest& request) {
    if (FT_Init_FreeType(&m_FreeTypeLibrary) != 0) {
        HE_ERROR("FreeTypeFontAtlasBackend: FT_Init_FreeType failed");
        return false;
    }

    if (FT_New_Face(m_FreeTypeLibrary, m_PrimaryFontPath.c_str(), 0, &m_PrimaryFace) != 0) {
        HE_ERROR("FreeTypeFontAtlasBackend: failed to load primary font: " + m_PrimaryFontPath);
        return false;
    }

    if (!m_FallbackFontPath.empty() && m_FallbackFontPath != m_PrimaryFontPath) {
        if (FT_New_Face(m_FreeTypeLibrary, m_FallbackFontPath.c_str(), 0, &m_FallbackFace) != 0) {
            HE_WARN("FreeTypeFontAtlasBackend: fallback font unavailable: " + m_FallbackFontPath);
            m_FallbackFace = nullptr;
        }
    }

    if (m_PrimaryFace) {
        m_PrimaryFace->face_flags |= FT_FACE_FLAG_HORIZONTAL;
    }

    const FT_UInt pixelHeight = static_cast<FT_UInt>(std::clamp(std::lround(m_EmSizePx), 10L, 96L));
    if (FT_Set_Pixel_Sizes(m_PrimaryFace, 0, pixelHeight) != 0) {
        HE_WARN("FreeTypeFontAtlasBackend: FT_Set_Pixel_Sizes failed for primary face");
    }
    if (m_FallbackFace && FT_Set_Pixel_Sizes(m_FallbackFace, 0, pixelHeight) != 0) {
        HE_WARN("FreeTypeFontAtlasBackend: FT_Set_Pixel_Sizes failed for fallback face");
    }

    HE_INFO("FreeTypeFontAtlasBackend: primary font=" + m_PrimaryFontPath +
            (m_FallbackFace ? " fallback=" + m_FallbackFontPath : "")
            + " emSize=" + std::to_string(m_EmSizePx));
    return true;
}

bool FreeTypeFontAtlasBackend::EnsureGlyphs(const std::vector<uint32_t>& codepoints) {
    bool added = false;
    for (uint32_t codepoint : codepoints) {
        if (!m_Codepoints.contains(codepoint)) {
            m_Codepoints.insert(codepoint);
            added = true;
        }
    }
    if (!added) {
        return true;
    }
    return RebuildAtlas();
}

bool FreeTypeFontAtlasBackend::RebuildAtlas() {
    if (!m_PrimaryFace) return false;

    m_AtlasRgba.assign(static_cast<size_t>(m_AtlasWidth * m_AtlasHeight * 4), 0);
    m_PlacementsByCodepoint.clear();

    ShelfPacker packer(m_AtlasWidth, m_AtlasHeight);
    uint32_t missingGlyphs = 0;

    for (uint32_t codepoint : m_Codepoints) {
        FT_Face face = m_PrimaryFace;
        FT_UInt glyphIndex = FT_Get_Char_Index(face, codepoint);
        if (glyphIndex == 0 && m_FallbackFace) {
            face = m_FallbackFace;
            glyphIndex = FT_Get_Char_Index(face, codepoint);
        }
        
        if (glyphIndex == 0) {
            ++missingGlyphs;
            continue;
        }

        if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT) != 0) {
            ++missingGlyphs;
            continue;
        }
        
        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) != 0) {
            ++missingGlyphs;
            continue;
        }

        FT_Bitmap& bitmap = face->glyph->bitmap;
        int width = bitmap.width;
        int height = bitmap.rows;

        GlyphPlacement placement{};
        placement.isWhitespace = (width == 0 || height == 0);
        placement.advance = static_cast<float>(face->glyph->advance.x) / 64.0f;

        if (!placement.isWhitespace) {
            int px, py;
            if (!packer.Pack(width, height, px, py)) {
                HE_ERROR("FreeTypeFontAtlasBackend: atlas full!");
                return false;
            }

            // Copy to atlas
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    uint8_t alpha = bitmap.buffer[y * bitmap.pitch + x];
                    size_t idx = static_cast<size_t>((py + y) * m_AtlasWidth + (px + x)) * 4;
                    m_AtlasRgba[idx + 0] = 255;
                    m_AtlasRgba[idx + 1] = 255;
                    m_AtlasRgba[idx + 2] = 255;
                    m_AtlasRgba[idx + 3] = alpha;
                }
            }

            placement.hasDrawableQuad = true;
            
            // Plane metrics (in em units) - scale by 1.0 since we baked at emSizePx
            placement.planeLeft = static_cast<float>(face->glyph->bitmap_left);
            placement.planeRight = placement.planeLeft + static_cast<float>(width);
            placement.planeTop = static_cast<float>(face->glyph->bitmap_top);
            placement.planeBottom = placement.planeTop - static_cast<float>(height);

            // Atlas bounds
            placement.atlasLeft = static_cast<float>(px) / static_cast<float>(m_AtlasWidth);
            placement.atlasRight = static_cast<float>(px + width) / static_cast<float>(m_AtlasWidth);
            placement.atlasTop = static_cast<float>(py) / static_cast<float>(m_AtlasHeight);
            placement.atlasBottom = static_cast<float>(py + height) / static_cast<float>(m_AtlasHeight);
        }

        m_PlacementsByCodepoint[codepoint] = placement;
    }

    if (missingGlyphs > 0) {
        HE_WARN("FreeTypeFontAtlasBackend: " + std::to_string(missingGlyphs) + " codepoints missing");
    }

    m_Dirty = true;
    HE_INFO("FreeTypeFontAtlasBackend: rebuilt atlas " + std::to_string(m_AtlasWidth) + "x" +
            std::to_string(m_AtlasHeight) + " placements=" + std::to_string(m_PlacementsByCodepoint.size()));
    return true;
}

bool FreeTypeFontAtlasBackend::GetGlyphQuadAt(uint32_t codepoint, float penX, float penY, GlyphInfo& outQuad) const {
    const auto it = m_PlacementsByCodepoint.find(codepoint);
    if (it == m_PlacementsByCodepoint.end()) {
        return false;
    }

    const GlyphPlacement& glyph = it->second;
    if (!glyph.hasDrawableQuad) {
        return false;
    }

    outQuad.x0 = penX + glyph.planeLeft;
    outQuad.y0 = penY - glyph.planeTop;
    outQuad.x1 = penX + glyph.planeRight;
    outQuad.y1 = penY - glyph.planeBottom;
    outQuad.u0 = glyph.atlasLeft;
    outQuad.v0 = glyph.atlasTop;
    outQuad.u1 = glyph.atlasRight;
    outQuad.v1 = glyph.atlasBottom;
    outQuad.xadvance = glyph.advance;
    return true;
}

bool FreeTypeFontAtlasBackend::GetGlyphQuad(uint32_t codepoint, float* cursorX, float* cursorY, GlyphInfo& outQuad) const {
    const auto it = m_PlacementsByCodepoint.find(codepoint);
    if (it == m_PlacementsByCodepoint.end()) {
        return false;
    }

    const GlyphPlacement& glyph = it->second;
    const float penX = *cursorX;
    const bool drawable = GetGlyphQuadAt(codepoint, penX, *cursorY, outQuad);
    *cursorX += glyph.advance;
    return drawable;
}

float FreeTypeFontAtlasBackend::GetGlyphAdvance(uint32_t codepoint) const {
    const auto it = m_PlacementsByCodepoint.find(codepoint);
    if (it != m_PlacementsByCodepoint.end()) {
        return it->second.advance;
    }
    
    if (!m_PrimaryFace) {
        return 0.0f;
    }

    FT_Face face = m_PrimaryFace;
    FT_UInt glyphIndex = FT_Get_Char_Index(face, codepoint);
    if (glyphIndex == 0 && m_FallbackFace) {
        face = m_FallbackFace;
        glyphIndex = FT_Get_Char_Index(face, codepoint);
    }
    if (glyphIndex == 0) {
        return 0.0f;
    }

    if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT) != 0) {
        return 0.0f;
    }
    return (static_cast<float>(face->glyph->advance.x) / 64.0f);
}

bool FreeTypeFontAtlasBackend::GetFontFaces(FontFaceHandles& outFaces) const {
    if (!m_PrimaryFace) {
        return false;
    }
    outFaces.primary = m_PrimaryFace;
    outFaces.fallback = m_FallbackFace;
    return true;
}

float FreeTypeFontAtlasBackend::MeasureText(std::string_view utf8Text, float fontSize) const {
    std::vector<uint32_t> codepoints;
    DecodeUtf8(utf8Text, codepoints);

    const float scale = m_EmSizePx > 0.0f ? (fontSize / m_EmSizePx) : 1.0f;
    float penX = 0.0f;
    float penY = 0.0f;
    for (uint32_t codepoint : codepoints) {
        GlyphInfo quad{};
        if (!GetGlyphQuad(codepoint, &penX, &penY, quad)) {
            continue;
        }
    }
    return penX * scale;
}

} // namespace we::UI::Text
