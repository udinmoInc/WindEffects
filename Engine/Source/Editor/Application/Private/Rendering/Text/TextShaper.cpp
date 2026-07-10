#include "Rendering/Text/TextShaper.h"
#include "Core/Logger.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <unordered_set>

namespace we::UI::Text {

// ============================================================================
// TextShaper Implementation
// ============================================================================

TextShaper::TextShaper() = default;

TextShaper::~TextShaper() {
    Shutdown();
}

bool TextShaper::Initialize(const TextShaperConfig& config) {
    if (m_Initialized) {
        HE_WARN("TextShaper: Already initialized");
        return true;
    }

    m_Config = config;
    m_HasHarfBuzz = CheckHarfBuzzAvailability();
    
    if (m_Config.useHarfBuzz && !m_HasHarfBuzz) {
        HE_WARN("TextShaper: HarfBuzz requested but not available, will use simple shaping");
    }

    m_Initialized = true;
    HE_INFO("TextShaper: Initialized (HarfBuzz: " + std::string(m_HasHarfBuzz ? "yes" : "no") + ")");
    return true;
}

void TextShaper::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    m_Face = nullptr;
    m_FontSize = 0.0f;
    m_Initialized = false;

    HE_INFO("TextShaper: Shutdown");
}

void TextShaper::SetFontFace(FT_FaceRec_* face) {
    m_Face = face;
}

void TextShaper::SetFontSize(float size) {
    m_FontSize = size;
}

bool TextShaper::CheckHarfBuzzAvailability() {
    // This is a placeholder for actual HarfBuzz availability check
    // In a real implementation, you would check if HarfBuzz library is linked
    // and available at runtime
    return false;  // Placeholder - HarfBuzz not available
}

ShapingResult TextShaper::Shape(std::string_view text, const std::vector<uint32_t>* codepoints) {
    if (!m_Initialized || !m_Face) {
        ShapingResult result;
        result.success = false;
        result.errorMessage = "Shaper not initialized or no font face";
        return result;
    }

    // Detect direction if auto
    TextDirection direction = m_Config.defaultDirection;
    if (direction == TextDirection::Auto && m_Config.enableBidi) {
        direction = DetectDirection(text, codepoints);
    }

    // Use HarfBuzz if available and enabled
    if (m_Config.useHarfBuzz && m_HasHarfBuzz) {
        ShapingResult result = ShapeWithHarfBuzz(text, codepoints, direction);
        if (result.success) {
            return result;
        }
        
        if (m_Config.fallbackToSimple) {
            HE_WARN("TextShaper: HarfBuzz shaping failed, falling back to simple shaping");
        } else {
            return result;
        }
    }

    // Simple shaping
    std::vector<uint32_t> localCodepoints;
    if (!codepoints) {
        // Decode UTF-8 (simplified - in real implementation use UnicodeDecoder)
        localCodepoints.reserve(text.size());
        for (char c : text) {
            localCodepoints.push_back(static_cast<uint8_t>(c));
        }
        codepoints = &localCodepoints;
    }

    return ShapeSimple(*codepoints, direction);
}

ShapingResult TextShaper::Shape(std::string_view text, TextDirection direction) {
    return Shape(text, nullptr);
}

ShapingResult TextShaper::ShapeWithHarfBuzz(std::string_view text, const std::vector<uint32_t>* codepoints,
                                              TextDirection direction) {
    // Placeholder for HarfBuzz shaping
    // In a real implementation, this would use HarfBuzz library
    ShapingResult result;
    result.success = false;
    result.errorMessage = "HarfBuzz shaping not implemented";
    return result;
}

ShapingResult TextShaper::ShapeSimple(const std::vector<uint32_t>& codepoints, TextDirection direction) {
    ShapingResult result;
    result.success = true;
    result.direction = direction;
    result.script = TextShaperUtils::DetectDominantScript(codepoints);

    if (!m_Face) {
        result.success = false;
        result.errorMessage = "No font face";
        return result;
    }

    const float scale = 1.0f / 64.0f;  // Convert from 26.6 fixed-point to float
    float penX = 0.0f;
    float penY = 0.0f;

    result.glyphs.reserve(codepoints.size());

    for (uint32_t codepoint : codepoints) {
        ShapedGlyph glyph;
        glyph.codepoint = codepoint;
        glyph.isClusterStart = true;
        glyph.isWhitespace = TextShaperUtils::IsRTL(codepoint);  // Simplified check

        FT_UInt glyphIndex = FT_Get_Char_Index(m_Face, codepoint);
        glyph.glyphIndex = glyphIndex;

        if (glyphIndex == 0) {
            // Missing glyph
            glyph.advanceX = m_FontSize * 0.5f;
            glyph.advanceY = 0.0f;
            result.glyphs.push_back(glyph);
            penX += glyph.advanceX;
            continue;
        }

        // Load glyph
        if (FT_Load_Glyph(m_Face, glyphIndex, FT_LOAD_DEFAULT) != 0) {
            glyph.advanceX = m_FontSize * 0.5f;
            glyph.advanceY = 0.0f;
            result.glyphs.push_back(glyph);
            penX += glyph.advanceX;
            continue;
        }

        // Get advance
        glyph.advanceX = static_cast<float>(m_Face->glyph->metrics.horiAdvance) * scale;
        glyph.advanceY = static_cast<float>(m_Face->glyph->metrics.vertAdvance) * scale;

        // Get bearing
        glyph.offsetX = static_cast<float>(m_Face->glyph->metrics.horiBearingX) * scale;
        glyph.offsetY = static_cast<float>(m_Face->glyph->metrics.horiBearingY) * scale;

        glyph.x = penX + glyph.offsetX;
        glyph.y = penY + glyph.offsetY;

        result.glyphs.push_back(glyph);
        penX += glyph.advanceX;
    }

    // Apply kerning if enabled
    if (m_Config.enableKerning) {
        ApplyKerning(result.glyphs);
    }

    // Reorder for bidi if enabled
    if (m_Config.enableBidi && direction == TextDirection::RightToLeft) {
        ReorderBidi(result.glyphs, direction);
    }

    result.totalAdvance = penX;
    return result;
}

void TextShaper::ApplyKerning(std::vector<ShapedGlyph>& glyphs) {
    if (!m_Face || !FT_HAS_KERNING(m_Face)) {
        return;
    }

    if (glyphs.size() < 2) {
        return;
    }

    float penX = 0.0f;

    for (size_t i = 0; i < glyphs.size(); ++i) {
        if (i > 0) {
            FT_UInt leftIndex = glyphs[i - 1].glyphIndex;
            FT_UInt rightIndex = glyphs[i].glyphIndex;

            FT_Vector kerning;
            if (FT_Get_Kerning(m_Face, leftIndex, rightIndex, FT_KERNING_DEFAULT, &kerning) == 0) {
                float kerningAdjust = static_cast<float>(kerning.x) / 64.0f;
                penX += kerningAdjust;
            }
        }

        glyphs[i].x = penX + glyphs[i].offsetX;
        penX += glyphs[i].advanceX;
    }
}

void TextShaper::ReorderBidi(std::vector<ShapedGlyph>& glyphs, TextDirection direction) {
    if (direction == TextDirection::RightToLeft) {
        // Reverse the glyphs for RTL
        std::reverse(glyphs.begin(), glyphs.end());

        // Recalculate positions
        float penX = 0.0f;
        for (auto& glyph : glyphs) {
            glyph.x = penX + glyph.offsetX;
            penX += glyph.advanceX;
        }
    }
}

TextDirection TextShaper::DetectDirection(std::string_view text,
                                          const std::vector<uint32_t>* codepoints) {
    std::vector<uint32_t> localCodepoints;
    if (!codepoints) {
        // Simplified decoding
        localCodepoints.reserve(text.size());
        for (char c : text) {
            localCodepoints.push_back(static_cast<uint8_t>(c));
        }
        codepoints = &localCodepoints;
    }

    // Count RTL vs LTR characters
    size_t rtlCount = 0;
    size_t ltrCount = 0;

    for (uint32_t cp : *codepoints) {
        if (TextShaperUtils::IsRTL(cp)) {
            rtlCount++;
        } else {
            ltrCount++;
        }
    }

    if (rtlCount > ltrCount) {
        return TextDirection::RightToLeft;
    }
    return TextDirection::LeftToRight;
}

ScriptType TextShaper::DetectScript(std::string_view text,
                                     const std::vector<uint32_t>* codepoints) {
    std::vector<uint32_t> localCodepoints;
    if (!codepoints) {
        localCodepoints.reserve(text.size());
        for (char c : text) {
            localCodepoints.push_back(static_cast<uint8_t>(c));
        }
        codepoints = &localCodepoints;
    }

    return TextShaperUtils::DetectDominantScript(*codepoints);
}

float TextShaper::GetKerning(uint32_t leftGlyph, uint32_t rightGlyph) {
    if (!m_Face || !FT_HAS_KERNING(m_Face)) {
        return 0.0f;
    }

    FT_Vector kerning;
    if (FT_Get_Kerning(m_Face, leftGlyph, rightGlyph, FT_KERNING_DEFAULT, &kerning) == 0) {
        return static_cast<float>(kerning.x) / 64.0f;
    }

    return 0.0f;
}

bool TextShaper::UpdateConfig(const TextShaperConfig& config) {
    m_Config = config;
    return true;
}

// ============================================================================
// TextShaperUtils Implementation
// ============================================================================

namespace TextShaperUtils {

bool IsRTL(uint32_t codepoint) {
    // RTL ranges (simplified)
    if ((codepoint >= 0x0590 && codepoint <= 0x05FF) ||  // Hebrew
        (codepoint >= 0x0600 && codepoint <= 0x06FF) ||  // Arabic
        (codepoint >= 0x0750 && codepoint <= 0x077F) ||  // Arabic Supplement
        (codepoint >= 0xFB50 && codepoint <= 0xFDFF) ||  // Arabic Presentation Forms
        (codepoint >= 0xFE70 && codepoint <= 0xFEFF)) {  // Arabic Presentation Forms-B
        return true;
    }
    return false;
}

bool IsLigatureComponent(uint32_t codepoint) {
    // This is a simplified check
    // In a real implementation, you'd check against Unicode ligature ranges
    return false;
}

ScriptType GetScriptForCodepoint(uint32_t codepoint) {
    // Basic script detection based on Unicode ranges
    if (codepoint >= 0x0041 && codepoint <= 0x00FF) {
        return ScriptType::Latin;
    } else if (codepoint >= 0x0370 && codepoint <= 0x03FF) {
        return ScriptType::Greek;
    } else if (codepoint >= 0x0400 && codepoint <= 0x04FF) {
        return ScriptType::Cyrillic;
    } else if (codepoint >= 0x0590 && codepoint <= 0x05FF) {
        return ScriptType::Hebrew;
    } else if (codepoint >= 0x0600 && codepoint <= 0x06FF) {
        return ScriptType::Arabic;
    } else if (codepoint >= 0x0900 && codepoint <= 0x097F) {
        return ScriptType::Devanagari;
    } else if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) {
        return ScriptType::CJK;
    }
    return ScriptType::Common;
}

ScriptType DetectDominantScript(const std::vector<uint32_t>& codepoints) {
    std::unordered_map<ScriptType, size_t> scriptCounts;

    for (uint32_t cp : codepoints) {
        ScriptType script = GetScriptForCodepoint(cp);
        scriptCounts[script]++;
    }

    ScriptType dominant = ScriptType::Common;
    size_t maxCount = 0;

    for (const auto& [script, count] : scriptCounts) {
        if (count > maxCount) {
            maxCount = count;
            dominant = script;
        }
    }

    return dominant;
}

bool ContainsBidi(const std::vector<uint32_t>& codepoints) {
    for (uint32_t cp : codepoints) {
        if (IsRTL(cp)) {
            return true;
        }
    }
    return false;
}

TextShaperConfig GetDefaultConfig() {
    TextShaperConfig config;
    config.enableKerning = true;
    config.enableLigatures = true;
    config.enableBidi = true;
    config.defaultDirection = TextDirection::Auto;
    config.useHarfBuzz = true;
    config.fallbackToSimple = true;
    return config;
}

TextShaperConfig GetHighQualityConfig() {
    TextShaperConfig config = GetDefaultConfig();
    config.enableKerning = true;
    config.enableLigatures = true;
    config.enableBidi = true;
    config.useHarfBuzz = true;
    config.fallbackToSimple = false;
    return config;
}

TextShaperConfig GetPerformanceConfig() {
    TextShaperConfig config = GetDefaultConfig();
    config.enableKerning = false;
    config.enableLigatures = false;
    config.enableBidi = false;
    config.useHarfBuzz = false;
    config.fallbackToSimple = true;
    return config;
}

} // namespace TextShaperUtils

} // namespace we::UI::Text
