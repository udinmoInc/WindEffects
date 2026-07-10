#include "Rendering/Text/HarfBuzzTextLayout.h"
#include "Rendering/Text/Utf8.h"
#include "Core/Logger.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#if defined(WE_HAS_HARFBUZZ) && WE_HAS_HARFBUZZ
#include <hb.h>
#include <hb-ft.h>
#endif

#include <cmath>

namespace we::UI::Text {

namespace {

uint32_t CodepointAtCluster(std::string_view text, uint32_t cluster) {
    if (cluster >= text.size()) {
        return 0;
    }
    std::vector<uint32_t> decoded;
    DecodeUtf8(text.substr(cluster), decoded);
    return decoded.empty() ? 0 : decoded[0];
}

void UpdateMetricsFromFace(FT_FaceRec_* face, TextLayoutMetrics& metrics) {
    if (!face || !face->size) {
        return;
    }
    const float scale = 1.0f / 64.0f;
    metrics.ascender = static_cast<float>(face->size->metrics.ascender) * scale;
    metrics.descender = static_cast<float>(face->size->metrics.descender) * scale;
    metrics.lineHeight = static_cast<float>(face->size->metrics.height) * scale;

    if (face->units_per_EM > 0) {
        const float emScale = static_cast<float>(face->size->metrics.y_ppem) / static_cast<float>(face->units_per_EM);
        metrics.underlinePosition = static_cast<float>(face->underline_position) * emScale;
        metrics.underlineThickness = static_cast<float>(face->underline_thickness) * emScale;
    }
}

} // namespace

struct HarfBuzzTextLayout::Impl {
#if defined(WE_HAS_HARFBUZZ) && WE_HAS_HARFBUZZ
    hb_font_t* primaryFont = nullptr;
    hb_font_t* fallbackFont = nullptr;
    hb_buffer_t* buffer = nullptr;
#endif
    FT_FaceRec_* primaryFace = nullptr;
    FT_FaceRec_* fallbackFace = nullptr;
};

HarfBuzzTextLayout::HarfBuzzTextLayout() : m_Impl(std::make_unique<Impl>()) {}

HarfBuzzTextLayout::~HarfBuzzTextLayout() {
    Shutdown();
}

bool HarfBuzzTextLayout::Initialize(FT_FaceRec_* primaryFace, FT_FaceRec_* fallbackFace) {
    Shutdown();
    if (!primaryFace) {
        return false;
    }

    m_Impl->primaryFace = primaryFace;
    m_Impl->fallbackFace = fallbackFace;
    UpdateMetricsFromFace(primaryFace, m_Metrics);

#if defined(WE_HAS_HARFBUZZ) && WE_HAS_HARFBUZZ
    m_Impl->primaryFont = hb_ft_font_create_referenced(primaryFace);
    if (!m_Impl->primaryFont) {
        HE_ERROR("HarfBuzzTextLayout: hb_ft_font_create failed for primary face");
        Shutdown();
        return false;
    }

    hb_ft_font_set_load_flags(m_Impl->primaryFont, FT_LOAD_TARGET_NORMAL);

    if (fallbackFace) {
        m_Impl->fallbackFont = hb_ft_font_create_referenced(fallbackFace);
        if (m_Impl->fallbackFont) {
            hb_ft_font_set_load_flags(m_Impl->fallbackFont, FT_LOAD_TARGET_NORMAL);
        }
    }

    m_Impl->buffer = hb_buffer_create();
    if (!m_Impl->buffer) {
        HE_ERROR("HarfBuzzTextLayout: hb_buffer_create failed");
        Shutdown();
        return false;
    }

    HE_INFO("[UI FontAudit] HarfBuzz shaping enabled ascender=" + std::to_string(m_Metrics.ascender)
            + " descender=" + std::to_string(m_Metrics.descender)
            + " lineHeight=" + std::to_string(m_Metrics.lineHeight)
            + " ppem=" + std::to_string(primaryFace->size ? primaryFace->size->metrics.y_ppem : 0));
    return true;
#else
    HE_WARN("[UI FontAudit] HarfBuzz unavailable; using per-codepoint layout fallback");
    return true;
#endif
}

void HarfBuzzTextLayout::Shutdown() {
#if defined(WE_HAS_HARFBUZZ) && WE_HAS_HARFBUZZ
    if (m_Impl->buffer) {
        hb_buffer_destroy(m_Impl->buffer);
        m_Impl->buffer = nullptr;
    }
    if (m_Impl->fallbackFont) {
        hb_font_destroy(m_Impl->fallbackFont);
        m_Impl->fallbackFont = nullptr;
    }
    if (m_Impl->primaryFont) {
        hb_font_destroy(m_Impl->primaryFont);
        m_Impl->primaryFont = nullptr;
    }
#endif
    m_Impl->primaryFace = nullptr;
    m_Impl->fallbackFace = nullptr;
    m_Metrics = {};
    m_PrimaryFontPath.clear();
}

bool HarfBuzzTextLayout::IsReady() const {
#if defined(WE_HAS_HARFBUZZ) && WE_HAS_HARFBUZZ
    return m_Impl->primaryFont != nullptr && m_Impl->buffer != nullptr;
#else
    return m_Impl->primaryFace != nullptr;
#endif
}

bool HarfBuzzTextLayout::Shape(std::string_view utf8Text, std::vector<ShapedGlyph>& outGlyphs) const {
    outGlyphs.clear();
    if (utf8Text.empty() || !IsReady()) {
        return false;
    }

#if defined(WE_HAS_HARFBUZZ) && WE_HAS_HARFBUZZ
    hb_buffer_reset(m_Impl->buffer);
    hb_buffer_add_utf8(m_Impl->buffer, utf8Text.data(), static_cast<int>(utf8Text.size()), 0, static_cast<int>(utf8Text.size()));
    hb_buffer_guess_segment_properties(m_Impl->buffer);
    hb_shape(m_Impl->primaryFont, m_Impl->buffer, nullptr, 0);

    unsigned glyphCount = 0;
    hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(m_Impl->buffer, &glyphCount);
    hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(m_Impl->buffer, &glyphCount);
    if (!infos || !positions || glyphCount == 0) {
        return false;
    }

    outGlyphs.reserve(glyphCount);
    for (unsigned i = 0; i < glyphCount; ++i) {
        const uint32_t codepoint = CodepointAtCluster(utf8Text, infos[i].cluster);
        const uint32_t glyphId = infos[i].codepoint;
        const float xOffset = positions[i].x_offset / 64.0f;
        const float yOffset = positions[i].y_offset / 64.0f;
        const float hbAdvanceX = positions[i].x_advance / 64.0f;
        outGlyphs.push_back(ShapedGlyph{codepoint, glyphId, xOffset, yOffset, hbAdvanceX});
    }
    return !outGlyphs.empty();
#else
    std::vector<uint32_t> codepoints;
    DecodeUtf8(utf8Text, codepoints);
    for (uint32_t codepoint : codepoints) {
        FT_UInt glyphIndex = FT_Get_Char_Index(m_Impl->primaryFace, codepoint);
        if (glyphIndex == 0 && m_Impl->fallbackFace) {
            glyphIndex = FT_Get_Char_Index(m_Impl->fallbackFace, codepoint);
        }

        float advance = 0.0f;
        if (glyphIndex != 0) {
            FT_Load_Glyph(m_Impl->primaryFace, glyphIndex, FT_LOAD_TARGET_NORMAL);
            advance = static_cast<float>(m_Impl->primaryFace->glyph->advance.x) / 64.0f;
        }

        outGlyphs.push_back(ShapedGlyph{codepoint, glyphIndex, 0.0f, 0.0f, advance});
    }
    return !outGlyphs.empty();
#endif
}

float HarfBuzzTextLayout::MeasureWidth(std::string_view utf8Text) const {
    std::vector<ShapedGlyph> glyphs;
    if (!Shape(utf8Text, glyphs)) {
        return 0.0f;
    }
    float width = 0.0f;
    for (const ShapedGlyph& glyph : glyphs) {
        width += glyph.hbAdvanceX;
    }
    return width;
}

} // namespace we::UI::Text
