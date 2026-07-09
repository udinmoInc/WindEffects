#include "Rendering/Text/MsdfFontAtlasBackend.h"
#include "Rendering/FontAtlas.h"
#include "Rendering/Text/Utf8.h"
#include "Core/Logger.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4458 4505)
#endif
#include <msdf-atlas-gen/msdf-atlas-gen.h>
#include <msdfgen.h>
#include <msdfgen-ext.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <string_view>

namespace we::UI::Text {

namespace {

constexpr double kGlyphScale = 1.0;

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

} // namespace

MsdfFontAtlasBackend::MsdfFontAtlasBackend() = default;

MsdfFontAtlasBackend::~MsdfFontAtlasBackend() {
    Shutdown();
}

std::unique_ptr<IFontAtlasBackend> CreateMsdfFontAtlasBackend() {
    return std::make_unique<MsdfFontAtlasBackend>();
}

bool MsdfFontAtlasBackend::Initialize(const FontAtlasBakeRequest& request) {
    Shutdown();

    m_PrimaryFontPath = ResolveFontPath(request.primaryFontPath);
    m_FallbackFontPath = ResolveFontPath(request.fallbackFontPath);
    m_EmSizePx = std::clamp(request.emSizePx, 10.0f, 96.0f);
    m_MsdfPixelRange = std::max(request.msdfPixelRange, 2.0f);
    m_AtlasWidth = std::max(request.atlasWidth, 256);
    m_AtlasHeight = std::max(request.atlasHeight, 256);

    if (!LoadFontFaces(request)) {
        return false;
    }

    SeedCommonCodepoints(m_Codepoints);
    return RebuildAtlas();
}

void MsdfFontAtlasBackend::Shutdown() {
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
    m_CodepointToGlyphIndex.clear();
    m_Glyphs.clear();
    m_AtlasRgba.clear();
    m_Dirty = false;
}

bool MsdfFontAtlasBackend::LoadFontFaces(const FontAtlasBakeRequest& request) {
    if (FT_Init_FreeType(&m_FreeTypeLibrary) != 0) {
        HE_ERROR("MsdfFontAtlasBackend: FT_Init_FreeType failed");
        return false;
    }

    if (FT_New_Face(m_FreeTypeLibrary, m_PrimaryFontPath.c_str(), 0, &m_PrimaryFace) != 0) {
        HE_ERROR("MsdfFontAtlasBackend: failed to load primary font: " + m_PrimaryFontPath);
        return false;
    }

    if (!m_FallbackFontPath.empty() && m_FallbackFontPath != m_PrimaryFontPath) {
        if (FT_New_Face(m_FreeTypeLibrary, m_FallbackFontPath.c_str(), 0, &m_FallbackFace) != 0) {
            HE_WARN("MsdfFontAtlasBackend: fallback font unavailable: " + m_FallbackFontPath);
            m_FallbackFace = nullptr;
        }
    }

    HE_INFO("MsdfFontAtlasBackend: primary font=" + m_PrimaryFontPath +
            (m_FallbackFace ? " fallback=" + m_FallbackFontPath : ""));
    return true;
}

bool MsdfFontAtlasBackend::EnsureGlyphs(const std::vector<uint32_t>& codepoints) {
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

bool MsdfFontAtlasBackend::RebuildAtlas() {
    msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
    if (!ft) {
        HE_ERROR("MsdfFontAtlasBackend: msdfgen::initializeFreetype failed");
        return false;
    }

    msdfgen::FontHandle* primaryFont = msdfgen::loadFont(ft, m_PrimaryFontPath.c_str());
    if (!primaryFont) {
        HE_ERROR("MsdfFontAtlasBackend: msdfgen failed to load primary font");
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    msdfgen::FontHandle* fallbackFont = nullptr;
    if (!m_FallbackFontPath.empty() && m_FallbackFontPath != m_PrimaryFontPath) {
        fallbackFont = msdfgen::loadFont(ft, m_FallbackFontPath.c_str());
    }

    std::vector<msdf_atlas::GlyphGeometry> glyphGeometries;
    msdf_atlas::FontGeometry fontGeometry(&glyphGeometries);

    msdf_atlas::Charset charset;
    for (uint32_t codepoint : m_Codepoints) {
        charset.add(codepoint);
    }

    if (fontGeometry.loadCharset(primaryFont, kGlyphScale, charset) <= 0) {
        HE_ERROR("MsdfFontAtlasBackend: loadCharset failed for primary font");
        msdfgen::destroyFont(primaryFont);
        if (fallbackFont) {
            msdfgen::destroyFont(fallbackFont);
        }
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    if (fallbackFont) {
        for (uint32_t codepoint : m_Codepoints) {
            if (fontGeometry.getGlyph(codepoint) != nullptr) {
                continue;
            }
            msdf_atlas::Charset single;
            single.add(codepoint);
            fontGeometry.loadCharset(fallbackFont, kGlyphScale, single);
        }
    }

    for (msdf_atlas::GlyphGeometry& glyph : glyphGeometries) {
        glyph.edgeColoring(msdfgen::edgeColoringSimple, 3.0, 0);
    }

    msdf_atlas::TightAtlasPacker packer;
    packer.unsetDimensions();
    packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::POWER_OF_TWO_SQUARE);
    packer.setSpacing(2);
    packer.setScale(static_cast<double>(m_EmSizePx));
    packer.setPixelRange(msdfgen::Range(static_cast<double>(m_MsdfPixelRange)));
    if (packer.pack(glyphGeometries.data(), static_cast<int>(glyphGeometries.size())) != 0) {
        HE_ERROR("MsdfFontAtlasBackend: atlas packer failed");
        msdfgen::destroyFont(primaryFont);
        if (fallbackFont) {
            msdfgen::destroyFont(fallbackFont);
        }
        msdfgen::deinitializeFreetype(ft);
        return false;
    }

    int width = 0;
    int height = 0;
    packer.getDimensions(width, height);
    width = std::max(width, m_AtlasWidth);
    height = std::max(height, m_AtlasHeight);

    msdf_atlas::ImmediateAtlasGenerator<
        float,
        3,
        msdf_atlas::msdfGenerator,
        msdf_atlas::BitmapAtlasStorage<float, 3>> generator(width, height);

    msdf_atlas::GeneratorAttributes generatorAttributes;
    generatorAttributes.config.overlapSupport = true;
    generatorAttributes.scanlinePass = true;
    generator.setAttributes(generatorAttributes);
    generator.generate(glyphGeometries.data(), static_cast<int>(glyphGeometries.size()));

    const auto& storage = generator.atlasStorage();
    msdfgen::BitmapConstRef<float, 3> bitmap = storage;
    m_AtlasWidth = bitmap.width;
    m_AtlasHeight = bitmap.height;
    m_AtlasRgba.resize(static_cast<size_t>(m_AtlasWidth * m_AtlasHeight * 4));

    for (int y = 0; y < m_AtlasHeight; ++y) {
        for (int x = 0; x < m_AtlasWidth; ++x) {
            const float* msdf = bitmap(x, y);
            const size_t dstIndex = static_cast<size_t>(y * m_AtlasWidth + x) * 4;
            m_AtlasRgba[dstIndex + 0] = static_cast<uint8_t>(std::clamp(msdf[0], 0.0f, 1.0f) * 255.0f);
            m_AtlasRgba[dstIndex + 1] = static_cast<uint8_t>(std::clamp(msdf[1], 0.0f, 1.0f) * 255.0f);
            m_AtlasRgba[dstIndex + 2] = static_cast<uint8_t>(std::clamp(msdf[2], 0.0f, 1.0f) * 255.0f);
            m_AtlasRgba[dstIndex + 3] = 255;
        }
    }

    m_Glyphs.resize(glyphGeometries.size());
    m_CodepointToGlyphIndex.clear();
    for (size_t i = 0; i < glyphGeometries.size(); ++i) {
        const msdf_atlas::unicode_t codepoint = glyphGeometries[i].getCodepoint();
        if (codepoint != 0) {
            m_CodepointToGlyphIndex[static_cast<uint32_t>(codepoint)] = static_cast<int>(i);
        }

        const msdf_atlas::GlyphGeometry& glyph = glyphGeometries[i];
        GlyphPlacement placement{};

        double l = 0.0;
        double b = 0.0;
        double r = 0.0;
        double t = 0.0;
        glyph.getQuadPlaneBounds(l, b, r, t);
        placement.planeLeft = static_cast<float>(l);
        placement.planeBottom = static_cast<float>(b);
        placement.planeRight = static_cast<float>(r);
        placement.planeTop = static_cast<float>(t);
        placement.advance = static_cast<float>(glyph.getAdvance());

        double al = 0.0;
        double ab = 0.0;
        double ar = 0.0;
        double at = 0.0;
        glyph.getQuadAtlasBounds(al, ab, ar, at);
        placement.atlasLeft = static_cast<float>(al / m_AtlasWidth);
        placement.atlasBottom = static_cast<float>(ab / m_AtlasHeight);
        placement.atlasRight = static_cast<float>(ar / m_AtlasWidth);
        placement.atlasTop = static_cast<float>(at / m_AtlasHeight);

        m_Glyphs[i] = placement;
    }

    msdfgen::destroyFont(primaryFont);
    if (fallbackFont) {
        msdfgen::destroyFont(fallbackFont);
    }
    msdfgen::deinitializeFreetype(ft);

    m_Dirty = true;
    HE_INFO("MsdfFontAtlasBackend: rebuilt atlas " + std::to_string(m_AtlasWidth) + "x" +
            std::to_string(m_AtlasHeight) + " glyphs=" + std::to_string(m_Glyphs.size()));
    return true;
}

int MsdfFontAtlasBackend::FindGlyphIndex(uint32_t codepoint) const {
    const auto it = m_CodepointToGlyphIndex.find(codepoint);
    if (it != m_CodepointToGlyphIndex.end()) {
        return it->second;
    }
    return -1;
}

bool MsdfFontAtlasBackend::GetGlyphQuad(uint32_t codepoint, float* cursorX, float* cursorY, GlyphInfo& outQuad) const {
    const int glyphIndex = FindGlyphIndex(codepoint);
    if (glyphIndex < 0 || glyphIndex >= static_cast<int>(m_Glyphs.size())) {
        return false;
    }

    const GlyphPlacement& glyph = m_Glyphs[static_cast<size_t>(glyphIndex)];
    outQuad.x0 = *cursorX + glyph.planeLeft;
    outQuad.y0 = *cursorY + glyph.planeBottom;
    outQuad.x1 = *cursorX + glyph.planeRight;
    outQuad.y1 = *cursorY + glyph.planeTop;
    outQuad.u0 = glyph.atlasLeft;
    outQuad.v0 = glyph.atlasBottom;
    outQuad.u1 = glyph.atlasRight;
    outQuad.v1 = glyph.atlasTop;
    outQuad.xadvance = glyph.advance;

    *cursorX += glyph.advance;
    return true;
}

float MsdfFontAtlasBackend::MeasureText(std::string_view utf8Text, float fontSize) const {
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

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace we::UI::Text
