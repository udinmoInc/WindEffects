#include "Text/Shaping/TextShaper.h"

#include "Text/Assets/FontAssetManager.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#if defined(WE_HAS_HARFBUZZ) && WE_HAS_HARFBUZZ
#include <hb.h>
#include <hb-ft.h>
#endif

#include <algorithm>
#include <cmath>
#include <string>

namespace we::runtime::text::shaping {

namespace {

#if defined(WE_HAS_HARFBUZZ) && WE_HAS_HARFBUZZ
class HarfBuzzTextShaper final : public ITextShaper {
public:
    explicit HarfBuzzTextShaper(assets::IFontAssetManager& assetManager)
        : m_AssetManager(assetManager)
    {
    }

    TextResult<std::vector<ShapedRun>> Shape(
        const std::span<const Codepoint> codepoints,
        const FontHandle primaryFont,
        const ShapeOptions& options) const override
    {
        std::vector<ShapedRun> runs;
        if (codepoints.empty()) {
            return TextResult<std::vector<ShapedRun>>::Success(std::move(runs));
        }

        const auto asset = m_AssetManager.GetAsset(primaryFont);
        if (!asset || asset->sourcePath.empty()) {
            return TextResult<std::vector<ShapedRun>>::Failure("Primary font asset unavailable");
        }

        FT_Library library = nullptr;
        FT_Face face = nullptr;
        if (FT_Init_FreeType(&library) != 0
            || FT_New_Face(library, asset->sourcePath.string().c_str(), 0, &face) != 0) {
            if (library) {
                FT_Done_FreeType(library);
            }
            return TextResult<std::vector<ShapedRun>>::Failure("Failed to open font for shaping");
        }

        const FT_UInt pixelHeight = static_cast<FT_UInt>(std::clamp(std::lround(options.fontSize), 10L, 96L));
        FT_Set_Pixel_Sizes(face, 0, pixelHeight);

        hb_font_t* hbFont = hb_ft_font_create_referenced(face);
        hb_buffer_t* buffer = hb_buffer_create();

        std::string utf8;
        utf8.reserve(codepoints.size() * 4);
        for (const Codepoint codepoint : codepoints) {
            if (codepoint <= 0x7F) {
                utf8.push_back(static_cast<char>(codepoint));
            } else if (codepoint <= 0x7FF) {
                utf8.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
                utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            } else if (codepoint <= 0xFFFF) {
                utf8.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
                utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            } else {
                utf8.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
                utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
                utf8.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                utf8.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
        }

        hb_buffer_add_utf8(buffer, utf8.data(), static_cast<int>(utf8.size()), 0, static_cast<int>(utf8.size()));
        hb_buffer_guess_segment_properties(buffer);
        hb_buffer_set_direction(
            buffer,
            options.direction == TextDirection::RightToLeft ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
        hb_shape(hbFont, buffer, nullptr, 0);

        unsigned glyphCount = 0;
        hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &glyphCount);
        hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, &glyphCount);

        ShapedRun run;
        run.direction = options.direction;
        run.script = DetectScript(codepoints.front());
        run.glyphs.reserve(glyphCount);

        for (unsigned i = 0; i < glyphCount; ++i) {
            const size_t cluster = infos[i].cluster < codepoints.size() ? infos[i].cluster : 0;
            ShapedGlyph glyph;
            glyph.codepoint = codepoints[cluster];
            glyph.glyphIndex = infos[i].codepoint;
            glyph.fontHandle = primaryFont;
            glyph.xOffset = positions[i].x_offset / 64.0f;
            glyph.yOffset = positions[i].y_offset / 64.0f;
            glyph.xAdvance = positions[i].x_advance / 64.0f;
            glyph.yAdvance = positions[i].y_advance / 64.0f;
            glyph.cluster = static_cast<uint32_t>(cluster);
            run.glyphs.push_back(glyph);
        }

        runs.push_back(std::move(run));

        hb_buffer_destroy(buffer);
        hb_font_destroy(hbFont);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return TextResult<std::vector<ShapedRun>>::Success(std::move(runs));
    }

private:
    assets::IFontAssetManager& m_AssetManager;
};
#endif

class BasicTextShaper final : public ITextShaper {
public:
    explicit BasicTextShaper(assets::IFontAssetManager& assetManager)
        : m_AssetManager(assetManager)
    {
    }

    TextResult<std::vector<ShapedRun>> Shape(
        const std::span<const Codepoint> codepoints,
        const FontHandle primaryFont,
        const ShapeOptions& options) const override
    {
        ShapedRun run;
        run.direction = options.direction;
        run.script = codepoints.empty() ? Script::Unknown : DetectScript(codepoints.front());
        run.glyphs.reserve(codepoints.size());

        const auto asset = m_AssetManager.GetAsset(primaryFont);
        for (const Codepoint codepoint : codepoints) {
            ShapedGlyph glyph;
            glyph.codepoint = codepoint;
            glyph.fontHandle = primaryFont;
            if (asset) {
                if (const GlyphMetrics* metrics = asset->FindGlyph(codepoint)) {
                    glyph.xAdvance = metrics->advance;
                }
            }
            run.glyphs.push_back(glyph);
        }

        std::vector<ShapedRun> runs;
        runs.push_back(std::move(run));
        return TextResult<std::vector<ShapedRun>>::Success(std::move(runs));
    }

private:
    assets::IFontAssetManager& m_AssetManager;
};

} // namespace

Script DetectScript(const Codepoint codepoint)
{
    if (codepoint >= 0x1F300 && codepoint <= 0x1FAFF) {
        return Script::Emoji;
    }
    if (codepoint >= 0x0600 && codepoint <= 0x06FF) {
        return Script::Arabic;
    }
    if (codepoint >= 0x0900 && codepoint <= 0x097F) {
        return Script::Devanagari;
    }
    if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) {
        return Script::Han;
    }
    if (codepoint >= 0x3040 && codepoint <= 0x309F) {
        return Script::Hiragana;
    }
    if (codepoint >= 0x30A0 && codepoint <= 0x30FF) {
        return Script::Katakana;
    }
    if (codepoint >= 0xAC00 && codepoint <= 0xD7AF) {
        return Script::Hangul;
    }
    if (codepoint >= 0x0370 && codepoint <= 0x03FF) {
        return Script::Greek;
    }
    if (codepoint >= 0x0400 && codepoint <= 0x04FF) {
        return Script::Cyrillic;
    }
    if ((codepoint >= 'A' && codepoint <= 'Z') || (codepoint >= 'a' && codepoint <= 'z')) {
        return Script::Latin;
    }
    return Script::Common;
}

std::unique_ptr<ITextShaper> CreateTextShaper(assets::IFontAssetManager& assetManager)
{
#if defined(WE_HAS_HARFBUZZ) && WE_HAS_HARFBUZZ
    return std::make_unique<HarfBuzzTextShaper>(assetManager);
#else
    return std::make_unique<BasicTextShaper>(assetManager);
#endif
}

} // namespace we::runtime::text::shaping
