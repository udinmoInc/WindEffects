#include "Text/Import/FontImporter.h"

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
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace we::runtime::text::importing {

namespace {

std::unordered_set<Codepoint> SeedBasicCharset()
{
    std::unordered_set<Codepoint> codepoints;
    for (Codepoint cp = 0x20; cp <= 0x7E; ++cp) {
        codepoints.insert(cp);
    }
    for (Codepoint cp = 0xA0; cp <= 0xFF; ++cp) {
        codepoints.insert(cp);
    }
    for (Codepoint cp : {0x2013, 0x2014, 0x2022, 0x2026, 0x20AC, 0xFFFD}) {
        codepoints.insert(cp);
    }
    return codepoints;
}

std::unordered_set<Codepoint> SeedFullCharset()
{
    auto codepoints = SeedBasicCharset();
    for (Codepoint cp = 0x100; cp <= 0x17F; ++cp) {
        codepoints.insert(cp);
    }
    for (Codepoint cp = 0x400; cp <= 0x4FF; ++cp) {
        codepoints.insert(cp);
    }
    for (Codepoint cp = 0x0370; cp <= 0x03FF; ++cp) {
        codepoints.insert(cp);
    }
    for (Codepoint cp = 0x0600; cp <= 0x06FF; ++cp) {
        codepoints.insert(cp);
    }
    for (Codepoint cp = 0x0900; cp <= 0x097F; ++cp) {
        codepoints.insert(cp);
    }
    for (Codepoint cp = 0x4E00; cp <= 0x4FFF; ++cp) {
        codepoints.insert(cp);
    }
    for (Codepoint cp = 0x3040; cp <= 0x30FF; ++cp) {
        codepoints.insert(cp);
    }
    for (Codepoint cp = 0xAC00; cp <= 0xACFF; ++cp) {
        codepoints.insert(cp);
    }
    for (Codepoint cp = 0x1F300; cp <= 0x1F5FF; ++cp) {
        codepoints.insert(cp);
    }
    return codepoints;
}

GlyphMetrics BuildGlyphMetrics(
    const msdf_atlas::GlyphGeometry& glyph,
    const Codepoint codepoint,
    const int atlasWidth,
    const int atlasHeight)
{
    GlyphMetrics metrics{};
    metrics.codepoint = codepoint;
    metrics.advance = static_cast<float>(glyph.getAdvance());
    metrics.isWhitespace = glyph.isWhitespace();

    double left = 0.0;
    double bottom = 0.0;
    double right = 0.0;
    double top = 0.0;
    glyph.getQuadPlaneBounds(left, bottom, right, top);
    metrics.bounds.x = static_cast<float>(left);
    metrics.bounds.y = static_cast<float>(std::min(bottom, top));
    metrics.bounds.width = static_cast<float>(right - left);
    metrics.bounds.height = static_cast<float>(std::max(bottom, top) - std::min(bottom, top));
    metrics.bearing.x = metrics.bounds.x;
    metrics.bearing.y = metrics.bounds.y;

    double atlasLeft = 0.0;
    double atlasBottom = 0.0;
    double atlasRight = 0.0;
    double atlasTop = 0.0;
    glyph.getQuadAtlasBounds(atlasLeft, atlasBottom, atlasRight, atlasTop);
    const float atlasYTop = static_cast<float>(std::min(atlasBottom, atlasTop));
    const float atlasYBottom = static_cast<float>(std::max(atlasBottom, atlasTop));
    metrics.atlasUv.u0 = static_cast<float>(atlasLeft / atlasWidth);
    metrics.atlasUv.u1 = static_cast<float>(atlasRight / atlasWidth);
    metrics.atlasUv.v0 = 1.0f - (atlasYBottom / static_cast<float>(atlasHeight));
    metrics.atlasUv.v1 = 1.0f - (atlasYTop / static_cast<float>(atlasHeight));
    metrics.atlasPage = 0;

    const float planeW = std::fabs(static_cast<float>(right - left));
    const float planeH = static_cast<float>(std::max(bottom, top) - std::min(bottom, top));
    const float atlasW = metrics.atlasUv.u1 - metrics.atlasUv.u0;
    const float atlasH = metrics.atlasUv.v1 - metrics.atlasUv.v0;
    metrics.hasDrawableQuad = !metrics.isWhitespace
        && planeW > 0.001f
        && planeH > 0.001f
        && atlasW > 0.0f
        && atlasH > 0.0f;
    return metrics;
}

bool ValidateAtlasDimensions(const int width, const int height, std::string& error)
{
    if (width <= 0 || height <= 0) {
        error = "Invalid atlas dimensions: " + std::to_string(width) + "x" + std::to_string(height);
        return false;
    }
    if (width > 8192 || height > 8192) {
        error = "Atlas dimensions too large: " + std::to_string(width) + "x" + std::to_string(height);
        return false;
    }
    const size_t bytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    if (bytes > 64ull * 1024 * 1024) {
        error = "Atlas allocation too large";
        return false;
    }
    return true;
}

msdfgen::FontHandle* LoadFontFromPath(msdfgen::FreetypeHandle* ft, const std::filesystem::path& inputPath)
{
    msdfgen::FontHandle* font = msdfgen::loadFont(ft, inputPath.string().c_str());
    if (font) {
        return font;
    }

    std::ifstream fontFile(inputPath, std::ios::binary | std::ios::ate);
    if (!fontFile) {
        return nullptr;
    }
    const std::streamsize fileSize = fontFile.tellg();
    if (fileSize <= 0) {
        return nullptr;
    }
    fontFile.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer(static_cast<size_t>(fileSize));
    if (!fontFile.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        return nullptr;
    }
    return msdfgen::loadFontData(ft, buffer.data(), static_cast<int>(buffer.size()));
}

TextResult<FontAsset> ImportMsdfFont(
    const std::filesystem::path& inputPath,
    const ImportOptions& options,
    const uint32_t faceIndex)
{
    try {
    if (!std::filesystem::exists(inputPath)) {
        return TextResult<FontAsset>::Failure("Font file not found", inputPath.string());
    }

    const float bakeSizePx = std::clamp(options.bakeSizePx, 10.0f, 96.0f);
    const float msdfPixelRange = std::max(options.msdfPixelRange, bakeSizePx * 0.2f);
    const auto codepoints = BuildCharset(options.charset);

    msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
    if (!ft) {
        return TextResult<FontAsset>::Failure("msdfgen::initializeFreetype failed");
    }

    msdfgen::FontHandle* primaryFont = LoadFontFromPath(ft, inputPath);
    if (!primaryFont) {
        msdfgen::deinitializeFreetype(ft);
        return TextResult<FontAsset>::Failure("Failed to load font", inputPath.string());
    }

    msdfgen::FontHandle* fallbackFont = nullptr;
    if (!options.fallbackFontPath.empty()
        && options.fallbackFontPath != inputPath
        && std::filesystem::exists(options.fallbackFontPath)) {
        fallbackFont = LoadFontFromPath(ft, options.fallbackFontPath);
    }

    std::vector<msdf_atlas::GlyphGeometry> glyphGeometries;
    msdf_atlas::FontGeometry fontGeometry(&glyphGeometries);

    msdf_atlas::Charset charset;
    for (const Codepoint codepoint : codepoints) {
        charset.add(static_cast<uint32_t>(codepoint));
    }

    const double geometryLoadScale = 1.0;
    const double packerScale = static_cast<double>(bakeSizePx);
    if (fontGeometry.loadCharset(primaryFont, geometryLoadScale, charset) <= 0) {
        msdfgen::destroyFont(primaryFont);
        if (fallbackFont) {
            msdfgen::destroyFont(fallbackFont);
        }
        msdfgen::deinitializeFreetype(ft);
        return TextResult<FontAsset>::Failure("loadCharset failed for primary font");
    }

    if (fallbackFont) {
        for (const Codepoint codepoint : codepoints) {
            if (fontGeometry.getGlyph(static_cast<uint32_t>(codepoint)) != nullptr) {
                continue;
            }
            msdf_atlas::Charset single;
            single.add(static_cast<uint32_t>(codepoint));
            fontGeometry.loadCharset(fallbackFont, geometryLoadScale, single);
        }
    }

    for (msdf_atlas::GlyphGeometry& glyph : glyphGeometries) {
        glyph.edgeColoring(msdfgen::edgeColoringSimple, 3.0, 0);
    }

    msdf_atlas::TightAtlasPacker packer;
    packer.unsetDimensions();
    packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::POWER_OF_TWO_SQUARE);
    packer.setSpacing(2);
    packer.setScale(packerScale);
    packer.setMiterLimit(1.0);
    packer.setPixelRange(msdfgen::Range(static_cast<double>(msdfPixelRange)));
    if (packer.pack(glyphGeometries.data(), static_cast<int>(glyphGeometries.size())) != 0) {
        msdfgen::destroyFont(primaryFont);
        if (fallbackFont) {
            msdfgen::destroyFont(fallbackFont);
        }
        msdfgen::deinitializeFreetype(ft);
        return TextResult<FontAsset>::Failure("Atlas packer failed");
    }

    int width = 0;
    int height = 0;
    packer.getDimensions(width, height);
    std::string dimensionError;
    if (!ValidateAtlasDimensions(width, height, dimensionError)) {
        msdfgen::destroyFont(primaryFont);
        if (fallbackFont) {
            msdfgen::destroyFont(fallbackFont);
        }
        msdfgen::deinitializeFreetype(ft);
        return TextResult<FontAsset>::Failure(dimensionError);
    }
    if (options.atlasWidth > 0) {
        width = std::max(width, options.atlasWidth);
    }
    if (options.atlasHeight > 0) {
        height = std::max(height, options.atlasHeight);
    }
    if (!ValidateAtlasDimensions(width, height, dimensionError)) {
        msdfgen::destroyFont(primaryFont);
        if (fallbackFont) {
            msdfgen::destroyFont(fallbackFont);
        }
        msdfgen::deinitializeFreetype(ft);
        return TextResult<FontAsset>::Failure(dimensionError);
    }

    msdf_atlas::ImmediateAtlasGenerator<
        float,
        3,
        msdf_atlas::msdfGenerator,
        msdf_atlas::BitmapAtlasStorage<float, 3>> generator(width, height);

    msdf_atlas::GeneratorAttributes generatorAttributes;
    generatorAttributes.config.overlapSupport = false;
    generatorAttributes.scanlinePass = true;
    generator.setAttributes(generatorAttributes);
    generator.generate(glyphGeometries.data(), static_cast<int>(glyphGeometries.size()));

    const auto& storage = generator.atlasStorage();
    const msdfgen::BitmapConstRef<float, 3> bitmap = storage;
    if (!ValidateAtlasDimensions(bitmap.width, bitmap.height, dimensionError)) {
        msdfgen::destroyFont(primaryFont);
        if (fallbackFont) {
            msdfgen::destroyFont(fallbackFont);
        }
        msdfgen::deinitializeFreetype(ft);
        return TextResult<FontAsset>::Failure(dimensionError);
    }

    FontAsset asset;
    asset.atlasFormat = AtlasFormat::Msdf;
    asset.sourcePath = inputPath;

    AtlasPage page;
    page.width = static_cast<uint32_t>(bitmap.width);
    page.height = static_cast<uint32_t>(bitmap.height);
    page.format = AtlasFormat::Msdf;
    const size_t rgbaBytes = static_cast<size_t>(page.width) * static_cast<size_t>(page.height) * 4;
    page.rgba.resize(rgbaBytes);

    for (int y = 0; y < bitmap.height; ++y) {
        for (int x = 0; x < bitmap.width; ++x) {
            const float* msdf = bitmap(x, y);
            const int dstY = bitmap.height - 1 - y;
            const size_t dstIndex = static_cast<size_t>(dstY * bitmap.width + x) * 4;
            page.rgba[dstIndex + 0] = static_cast<uint8_t>(std::clamp(msdf[0], 0.0f, 1.0f) * 255.0f);
            page.rgba[dstIndex + 1] = static_cast<uint8_t>(std::clamp(msdf[1], 0.0f, 1.0f) * 255.0f);
            page.rgba[dstIndex + 2] = static_cast<uint8_t>(std::clamp(msdf[2], 0.0f, 1.0f) * 255.0f);
            page.rgba[dstIndex + 3] = 255;
        }
    }
    asset.atlasPages.push_back(std::move(page));

    asset.metrics.bakeSizePx = bakeSizePx;
    asset.metrics.msdfPixelRange = msdfPixelRange;
    // Plane bounds / advances from msdf-atlas-gen are in bake-pixel units when scale=bakeSizePx.
    asset.metrics.geometryScale = bakeSizePx;

    FT_Library freeTypeLibrary = nullptr;
    FT_Face face = nullptr;
    if (FT_Init_FreeType(&freeTypeLibrary) == 0) {
        if (FT_New_Face(freeTypeLibrary, inputPath.string().c_str(), static_cast<FT_Long>(faceIndex), &face) == 0) {
            asset.metrics.familyName = face->family_name ? face->family_name : "";
            asset.metrics.styleName = face->style_name ? face->style_name : "";
            // Request metrics at the same pixel size used for MSDF bake so layout scales consistently.
            FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(std::lround(bakeSizePx)));
            asset.metrics.ascender = static_cast<float>(face->size->metrics.ascender) / 64.0f;
            asset.metrics.descender = static_cast<float>(face->size->metrics.descender) / 64.0f;
            asset.metrics.lineHeight = static_cast<float>(face->size->metrics.height) / 64.0f;
            const float em = static_cast<float>(std::max<FT_Long>(face->units_per_EM, 1));
            asset.metrics.underlinePosition =
                static_cast<float>(face->underline_position) / em * bakeSizePx;
            asset.metrics.underlineThickness =
                static_cast<float>(face->underline_thickness) / em * bakeSizePx;
            if (face->style_flags & FT_STYLE_FLAG_BOLD) {
                asset.metrics.weight = 700;
            } else {
                asset.metrics.weight = 400;
            }
            asset.metrics.italic = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
        }
        if (face) {
            FT_Done_Face(face);
        }
        FT_Done_FreeType(freeTypeLibrary);
    }

    asset.glyphs.reserve(codepoints.size());
    for (const Codepoint codepoint : codepoints) {
        const msdf_atlas::GlyphGeometry* glyph = fontGeometry.getGlyph(static_cast<uint32_t>(codepoint));
        if (!glyph) {
            continue;
        }
        asset.glyphs.push_back(BuildGlyphMetrics(*glyph, codepoint, bitmap.width, bitmap.height));
    }

    std::sort(asset.glyphs.begin(), asset.glyphs.end(), [](const GlyphMetrics& a, const GlyphMetrics& b) {
        return a.codepoint < b.codepoint;
    });

    if (!asset.glyphs.empty()) {
        UnicodeRange range{};
        range.start = asset.glyphs.front().codepoint;
        range.end = asset.glyphs.front().codepoint;
        for (size_t i = 1; i < asset.glyphs.size(); ++i) {
            if (asset.glyphs[i].codepoint == range.end + 1) {
                range.end = asset.glyphs[i].codepoint;
            } else {
                asset.coverage.push_back(range);
                range.start = asset.glyphs[i].codepoint;
                range.end = asset.glyphs[i].codepoint;
            }
        }
        asset.coverage.push_back(range);
    }

    msdfgen::destroyFont(primaryFont);
    if (fallbackFont) {
        msdfgen::destroyFont(fallbackFont);
    }
    msdfgen::deinitializeFreetype(ft);

    return TextResult<FontAsset>::Success(std::move(asset));
    } catch (const std::exception& ex) {
        return TextResult<FontAsset>::Failure(std::string("MSDF import exception: ") + ex.what());
    } catch (...) {
        return TextResult<FontAsset>::Failure("MSDF import failed with unknown exception");
    }
}

} // namespace

std::unordered_set<Codepoint> BuildCharset(const std::string& charsetName)
{
    if (charsetName == "full") {
        return SeedFullCharset();
    }
    return SeedBasicCharset();
}

TextResult<FontAsset> TtfOtfImporter::Import(
    const std::filesystem::path& inputPath,
    const ImportOptions& options) const
{
    return ImportMsdfFont(inputPath, options, 0);
}

TextResult<FontAsset> TtcImporter::Import(
    const std::filesystem::path& inputPath,
    const ImportOptions& options) const
{
    return ImportMsdfFont(inputPath, options, options.ttcFaceIndex);
}

std::unique_ptr<IFontImporter> CreateFontImporter(const std::filesystem::path& inputPath)
{
    const auto extension = inputPath.extension().string();
    if (extension == ".ttc" || extension == ".TTC") {
        return std::make_unique<TtcImporter>();
    }
    return std::make_unique<TtfOtfImporter>();
}

} // namespace we::runtime::text::importing
