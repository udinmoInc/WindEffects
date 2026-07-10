#include "Rendering/Text/MsdfAtlasGenerator.h"
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
#include <fstream>
#include <filesystem>

namespace we::UI::Text {

// ============================================================================
// MsdfAtlasGenerator Implementation
// ============================================================================

MsdfAtlasGenerator::MsdfAtlasGenerator() = default;

MsdfAtlasGenerator::~MsdfAtlasGenerator() {
    Shutdown();
}

bool MsdfAtlasGenerator::Initialize(const MsdfAtlasConfig& config) {
    if (m_Initialized) {
        HE_WARN("MsdfAtlasGenerator: Already initialized");
        return true;
    }

    // Validate configuration
    std::vector<std::string> errors;
    if (!ValidateConfig(config, errors)) {
        for (const auto& error : errors) {
            HE_ERROR("MsdfAtlasGenerator: Config error - " + error);
        }
        return false;
    }

    m_Config = config;
    m_Codepoints.clear();
    m_Placements.clear();
    m_AtlasPixels.clear();
    m_AtlasWidth = 0;
    m_AtlasHeight = 0;
    m_Dirty = false;
    m_Initialized = true;

    HE_INFO("MsdfAtlasGenerator: Initialized with " + std::to_string(m_Config.initialWidth) + 
            "x" + std::to_string(m_Config.initialHeight) + " atlas");
    return true;
}

void MsdfAtlasGenerator::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    m_Codepoints.clear();
    m_Placements.clear();
    m_AtlasPixels.clear();
    m_AtlasWidth = 0;
    m_AtlasHeight = 0;
    m_Face = nullptr;
    m_Dirty = false;
    m_Initialized = false;

    HE_INFO("MsdfAtlasGenerator: Shutdown");
}

void MsdfAtlasGenerator::SetFontFace(FT_FaceRec_* face) {
    m_Face = face;
    m_Dirty = true;
}

bool MsdfAtlasGenerator::ValidateConfig(const MsdfAtlasConfig& config, 
                                          std::vector<std::string>& outErrors) const {
    outErrors.clear();

    if (config.initialWidth <= 0 || config.initialHeight <= 0) {
        outErrors.push_back("Initial atlas dimensions must be positive");
    }

    if (config.initialWidth > config.maxWidth || config.initialHeight > config.maxHeight) {
        outErrors.push_back("Initial dimensions exceed maximum dimensions");
    }

    if (config.maxWidth > 16384 || config.maxHeight > 16384) {
        outErrors.push_back("Maximum atlas dimensions too large (>16384)");
    }

    if (config.emSize <= 0.0f || config.emSize > 256.0f) {
        outErrors.push_back("EM size must be between 0 and 256");
    }

    if (config.pixelRange <= 0.0f || config.pixelRange > 64.0f) {
        outErrors.push_back("Pixel range must be between 0 and 64");
    }

    if (config.spacing < 0) {
        outErrors.push_back("Spacing must be non-negative");
    }

    return outErrors.empty();
}

MsdfAtlasResult MsdfAtlasGenerator::GenerateAtlas(const std::vector<uint32_t>& codepoints) {
    if (!m_Initialized) {
        MsdfAtlasResult result;
        result.success = false;
        result.errorMessage = "Generator not initialized";
        return result;
    }

    if (!m_Face) {
        MsdfAtlasResult result;
        result.success = false;
        result.errorMessage = "No font face set";
        return result;
    }

    MsdfAtlasResult result = GenerateMsdfAtlas(codepoints);
    
    if (result.success) {
        m_Codepoints.clear();
        for (uint32_t cp : codepoints) {
            m_Codepoints.insert(cp);
        }
        m_AtlasPixels = result.rgbaPixels;
        m_AtlasWidth = result.atlasWidth;
        m_AtlasHeight = result.atlasHeight;
        m_Dirty = false;
        
        HE_INFO("MsdfAtlasGenerator: Generated atlas " + std::to_string(m_AtlasWidth) + 
                "x" + std::to_string(m_AtlasHeight) + " with " + 
                std::to_string(result.glyphCount) + " glyphs");
    }

    return result;
}

MsdfAtlasResult MsdfAtlasGenerator::GenerateMsdfAtlas(const std::vector<uint32_t>& codepoints) {
    MsdfAtlasResult result;
    result.config = m_Config;

    // Initialize msdfgen FreeType handle
    msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
    if (!ft) {
        result.success = false;
        result.errorMessage = "Failed to initialize msdfgen FreeType";
        HE_ERROR("MsdfAtlasGenerator: " + result.errorMessage);
        return result;
    }

    // Get font path from FreeType face (if available)
    std::string fontPath;
    if (m_Face && m_Face->stream && m_Face->stream->pathname.pointer) {
        fontPath = reinterpret_cast<const char*>(m_Face->stream->pathname.pointer);
    }

    // Load font with msdfgen
    msdfgen::FontHandle* font = nullptr;
    if (!fontPath.empty() && std::filesystem::exists(fontPath)) {
        font = msdfgen::loadFont(ft, fontPath.c_str());
    }

    if (!font) {
        // Fallback: try to use FreeType face directly
        // This is a simplified approach - in production, you'd want to handle this better
        msdfgen::deinitializeFreetype(ft);
        result.success = false;
        result.errorMessage = "Failed to load font with msdfgen";
        HE_ERROR("MsdfAtlasGenerator: " + result.errorMessage);
        return result;
    }

    // Create glyph geometries
    std::vector<msdf_atlas::GlyphGeometry> glyphGeometries;
    msdf_atlas::FontGeometry fontGeometry(&glyphGeometries);

    // Build charset from codepoints
    msdf_atlas::Charset charset;
    for (uint32_t codepoint : codepoints) {
        charset.add(codepoint);
    }

    // Load glyphs with geometry scale matching EM size
    const double geometryScale = static_cast<double>(m_Config.emSize);
    int loadedGlyphs = fontGeometry.loadCharset(font, geometryScale, charset);
    
    if (loadedGlyphs <= 0) {
        msdfgen::destroyFont(font);
        msdfgen::deinitializeFreetype(ft);
        result.success = false;
        result.errorMessage = "Failed to load glyphs from font";
        HE_ERROR("MsdfAtlasGenerator: " + result.errorMessage);
        return result;
    }

    result.glyphCount = static_cast<size_t>(loadedGlyphs);
    result.missingGlyphs = codepoints.size() - static_cast<size_t>(loadedGlyphs);

    // Apply edge coloring
    for (msdf_atlas::GlyphGeometry& glyph : glyphGeometries) {
        double angleThreshold = static_cast<double>(m_Config.edgeColoringAngleThreshold);
        switch (m_Config.edgeColoring) {
            case EdgeColoring::Simple:
                glyph.edgeColoring(msdfgen::edgeColoringSimple, angleThreshold, 0);
                break;
            case EdgeColoring::InkTrap:
                glyph.edgeColoring(msdfgen::edgeColoringInkTrap, angleThreshold, 0);
                break;
            case EdgeColoring::DistanceCheck:
                glyph.edgeColoring(msdfgen::edgeColoringSimple, angleThreshold, 0);  // Fallback to simple
                break;
        }
    }

    // Pack glyphs into atlas
    msdf_atlas::TightAtlasPacker packer;
    packer.unsetDimensions();
    packer.setDimensionsConstraint(static_cast<msdf_atlas::DimensionsConstraint>(ConvertPackingConstraint(m_Config.packingConstraint)));
    packer.setSpacing(m_Config.spacing);
    packer.setScale(geometryScale);
    packer.setMiterLimit(static_cast<double>(m_Config.miterLimit));
    packer.setPixelRange(msdfgen::Range(static_cast<double>(m_Config.pixelRange)));

    if (packer.pack(glyphGeometries.data(), static_cast<int>(glyphGeometries.size())) != 0) {
        msdfgen::destroyFont(font);
        msdfgen::deinitializeFreetype(ft);
        result.success = false;
        result.errorMessage = "Atlas packer failed";
        HE_ERROR("MsdfAtlasGenerator: " + result.errorMessage);
        return result;
    }

    // Get packed dimensions
    int packedWidth = 0;
    int packedHeight = 0;
    packer.getDimensions(packedWidth, packedHeight);
    
    // Ensure minimum dimensions
    packedWidth = std::max(packedWidth, m_Config.initialWidth);
    packedHeight = std::max(packedHeight, m_Config.initialHeight);
    
    // Clamp to maximum dimensions
    packedWidth = std::min(packedWidth, m_Config.maxWidth);
    packedHeight = std::min(packedHeight, m_Config.maxHeight);

    result.atlasWidth = packedWidth;
    result.atlasHeight = packedHeight;

    // Generate MSDF atlas
    std::vector<uint8_t> tempPixels;
    bool generationSuccess = false;

    switch (m_Config.mode) {
        case MsdfMode::MSDF: {
            msdf_atlas::ImmediateAtlasGenerator<
                float, 3, msdf_atlas::msdfGenerator,
                msdf_atlas::BitmapAtlasStorage<float, 3>> generator(packedWidth, packedHeight);

            msdf_atlas::GeneratorAttributes attributes;
            attributes.config.overlapSupport = m_Config.overlapSupport;
            attributes.scanlinePass = m_Config.scanlinePass;
            generator.setAttributes(attributes);
            generator.generate(glyphGeometries.data(), static_cast<int>(glyphGeometries.size()));

            const auto& storage = generator.atlasStorage();
            msdfgen::BitmapConstRef<float, 3> bitmap = storage;

            // Convert to RGBA8
            tempPixels.resize(packedWidth * packedHeight * 4);
            for (int y = 0; y < packedHeight; ++y) {
                for (int x = 0; x < packedWidth; ++x) {
                    const float* msdf = bitmap(x, y);
                    const int dstY = m_Config.invertY ? (packedHeight - 1 - y) : y;
                    const size_t dstIndex = (dstY * packedWidth + x) * 4;
                    tempPixels[dstIndex + 0] = static_cast<uint8_t>(std::clamp(msdf[0], 0.0f, 1.0f) * 255.0f);
                    tempPixels[dstIndex + 1] = static_cast<uint8_t>(std::clamp(msdf[1], 0.0f, 1.0f) * 255.0f);
                    tempPixels[dstIndex + 2] = static_cast<uint8_t>(std::clamp(msdf[2], 0.0f, 1.0f) * 255.0f);
                    tempPixels[dstIndex + 3] = m_Config.generateAlpha ? 255 : 0;
                }
            }
            generationSuccess = true;
            break;
        }

        case MsdfMode::SDF: {
            msdf_atlas::ImmediateAtlasGenerator<
                float, 1, msdf_atlas::sdfGenerator,
                msdf_atlas::BitmapAtlasStorage<float, 1>> generator(packedWidth, packedHeight);

            msdf_atlas::GeneratorAttributes attributes;
            attributes.config.overlapSupport = m_Config.overlapSupport;
            attributes.scanlinePass = m_Config.scanlinePass;
            generator.setAttributes(attributes);
            generator.generate(glyphGeometries.data(), static_cast<int>(glyphGeometries.size()));

            const auto& storage = generator.atlasStorage();
            msdfgen::BitmapConstRef<float, 1> bitmap = storage;

            // Convert to RGBA8 (grayscale to RGB)
            tempPixels.resize(packedWidth * packedHeight * 4);
            for (int y = 0; y < packedHeight; ++y) {
                for (int x = 0; x < packedWidth; ++x) {
                    float sdf = *(bitmap.operator()(x, y));
                    const int dstY = m_Config.invertY ? (packedHeight - 1 - y) : y;
                    const size_t dstIndex = (dstY * packedWidth + x) * 4;
                    uint8_t value = static_cast<uint8_t>(std::clamp(sdf, 0.0f, 1.0f) * 255.0f);
                    tempPixels[dstIndex + 0] = value;
                    tempPixels[dstIndex + 1] = value;
                    tempPixels[dstIndex + 2] = value;
                    tempPixels[dstIndex + 3] = m_Config.generateAlpha ? 255 : 0;
                }
            }
            generationSuccess = true;
            break;
        }

        case MsdfMode::PSDF: {
            msdf_atlas::ImmediateAtlasGenerator<
                float, 1, msdf_atlas::psdfGenerator,
                msdf_atlas::BitmapAtlasStorage<float, 1>> generator(packedWidth, packedHeight);

            msdf_atlas::GeneratorAttributes attributes;
            attributes.config.overlapSupport = m_Config.overlapSupport;
            attributes.scanlinePass = m_Config.scanlinePass;
            generator.setAttributes(attributes);
            generator.generate(glyphGeometries.data(), static_cast<int>(glyphGeometries.size()));

            const auto& storage = generator.atlasStorage();
            msdfgen::BitmapConstRef<float, 1> bitmap = storage;

            // Convert to RGBA8
            tempPixels.resize(packedWidth * packedHeight * 4);
            for (int y = 0; y < packedHeight; ++y) {
                for (int x = 0; x < packedWidth; ++x) {
                    float psdf = *(bitmap.operator()(x, y));
                    const int dstY = m_Config.invertY ? (packedHeight - 1 - y) : y;
                    const size_t dstIndex = (dstY * packedWidth + x) * 4;
                    uint8_t value = static_cast<uint8_t>(std::clamp(psdf, 0.0f, 1.0f) * 255.0f);
                    tempPixels[dstIndex + 0] = value;
                    tempPixels[dstIndex + 1] = value;
                    tempPixels[dstIndex + 2] = value;
                    tempPixels[dstIndex + 3] = m_Config.generateAlpha ? 255 : 0;
                }
            }
            generationSuccess = true;
            break;
        }

        case MsdfMode::MTSDF: {
            msdf_atlas::ImmediateAtlasGenerator<
                float, 4, msdf_atlas::mtsdfGenerator,
                msdf_atlas::BitmapAtlasStorage<float, 4>> generator(packedWidth, packedHeight);

            msdf_atlas::GeneratorAttributes attributes;
            attributes.config.overlapSupport = m_Config.overlapSupport;
            attributes.scanlinePass = m_Config.scanlinePass;
            generator.setAttributes(attributes);
            generator.generate(glyphGeometries.data(), static_cast<int>(glyphGeometries.size()));

            const auto& storage = generator.atlasStorage();
            msdfgen::BitmapConstRef<float, 4> bitmap = storage;

            // Convert to RGBA8
            tempPixels.resize(packedWidth * packedHeight * 4);
            for (int y = 0; y < packedHeight; ++y) {
                for (int x = 0; x < packedWidth; ++x) {
                    const float* mtsdf = bitmap(x, y);
                    const int dstY = m_Config.invertY ? (packedHeight - 1 - y) : y;
                    const size_t dstIndex = (dstY * packedWidth + x) * 4;
                    tempPixels[dstIndex + 0] = static_cast<uint8_t>(std::clamp(mtsdf[0], 0.0f, 1.0f) * 255.0f);
                    tempPixels[dstIndex + 1] = static_cast<uint8_t>(std::clamp(mtsdf[1], 0.0f, 1.0f) * 255.0f);
                    tempPixels[dstIndex + 2] = static_cast<uint8_t>(std::clamp(mtsdf[2], 0.0f, 1.0f) * 255.0f);
                    tempPixels[dstIndex + 3] = static_cast<uint8_t>(std::clamp(mtsdf[3], 0.0f, 1.0f) * 255.0f);
                }
            }
            generationSuccess = true;
            break;
        }
    }

    msdfgen::destroyFont(font);
    msdfgen::deinitializeFreetype(ft);

    if (!generationSuccess) {
        result.success = false;
        result.errorMessage = "MSDF generation failed";
        HE_ERROR("MsdfAtlasGenerator: " + result.errorMessage);
        return result;
    }

    // Build glyph placements
    m_Placements.clear();
    for (uint32_t codepoint : codepoints) {
        const msdf_atlas::GlyphGeometry* glyph = fontGeometry.getGlyph(codepoint);
        if (glyph) {
            AtlasGlyphPlacement placement = BuildPlacement(*glyph, packedWidth, packedHeight);
            placement.codepoint = codepoint;
            m_Placements[codepoint] = placement;
        }
    }

    result.rgbaPixels = std::move(tempPixels);
    result.success = true;

    // Calculate packing efficiency
    if (result.glyphCount > 0) {
        double totalGlyphArea = 0.0;
        for (const auto& [cp, placement] : m_Placements) {
            totalGlyphArea += placement.atlasWidth * placement.atlasHeight;
        }
        double atlasArea = static_cast<double>(packedWidth * packedHeight);
        result.packingEfficiency = static_cast<float>(totalGlyphArea / atlasArea);
    }

    return result;
}

AtlasGlyphPlacement MsdfAtlasGenerator::BuildPlacement(const msdf_atlas::GlyphGeometry& glyph,
                                                         int atlasWidth, int atlasHeight) const {
    AtlasGlyphPlacement placement;

    placement.isWhitespace = glyph.isWhitespace();
    placement.advance = static_cast<float>(glyph.getAdvance());

    // Get plane bounds (in font units)
    double l = 0.0, b = 0.0, r = 0.0, t = 0.0;
    glyph.getQuadPlaneBounds(l, b, r, t);
    placement.planeLeft = static_cast<float>(l);
    placement.planeBottom = static_cast<float>(std::min(b, t));
    placement.planeRight = static_cast<float>(r);
    placement.planeTop = static_cast<float>(std::max(b, t));

    // Get atlas bounds (in pixels)
    double al = 0.0, ab = 0.0, ar = 0.0, at = 0.0;
    glyph.getQuadAtlasBounds(al, ab, ar, at);
    
    placement.atlasLeft = static_cast<float>(al);
    placement.atlasRight = static_cast<float>(ar);
    placement.atlasBottom = static_cast<float>(ab);
    placement.atlasTop = static_cast<float>(at);

    // Convert to normalized UV coordinates
    placement.atlasLeft /= static_cast<float>(atlasWidth);
    placement.atlasRight /= static_cast<float>(atlasWidth);
    
    if (m_Config.invertY) {
        placement.atlasTop = 1.0f - (static_cast<float>(ab) / static_cast<float>(atlasHeight));
        placement.atlasBottom = 1.0f - (static_cast<float>(at) / static_cast<float>(atlasHeight));
    } else {
        placement.atlasTop = static_cast<float>(at) / static_cast<float>(atlasHeight);
        placement.atlasBottom = static_cast<float>(ab) / static_cast<float>(atlasHeight);
    }

    // Calculate pixel dimensions
    placement.atlasX = static_cast<int>(al);
    placement.atlasY = static_cast<int>(ab);
    placement.atlasWidth = static_cast<int>(ar - al);
    placement.atlasHeight = static_cast<int>(at - ab);

    // Check if drawable
    const float planeW = std::fabs(placement.planeRight - placement.planeLeft);
    const float planeH = placement.planeTop - placement.planeBottom;
    const float atlasW = placement.atlasRight - placement.atlasLeft;
    const float atlasH = placement.atlasBottom - placement.atlasTop;
    
    placement.hasDrawableQuad = !placement.isWhitespace
        && planeW > 0.001f
        && planeH > 0.001f
        && atlasW > 0.0f
        && atlasH > 0.0f;

    return placement;
}

int MsdfAtlasGenerator::ConvertPackingConstraint(PackingConstraint constraint) const {
    switch (constraint) {
        case PackingConstraint::PowerOfTwoSquare:
            return static_cast<int>(msdf_atlas::DimensionsConstraint::POWER_OF_TWO_SQUARE);
        case PackingConstraint::PowerOfTwoRectangle:
            return static_cast<int>(msdf_atlas::DimensionsConstraint::POWER_OF_TWO_RECTANGLE);
        case PackingConstraint::Square:
            return static_cast<int>(msdf_atlas::DimensionsConstraint::SQUARE);
        case PackingConstraint::Rectangle:
            return static_cast<int>(msdf_atlas::DimensionsConstraint::SQUARE);
        default:
            return static_cast<int>(msdf_atlas::DimensionsConstraint::POWER_OF_TWO_SQUARE);
    }
}

int MsdfAtlasGenerator::GetMsdfChannels() const {
    switch (m_Config.mode) {
        case MsdfMode::SDF:
        case MsdfMode::PSDF:
            return 1;
        case MsdfMode::MSDF:
            return 3;
        case MsdfMode::MTSDF:
            return 4;
        default:
            return 3;
    }
}

MsdfAtlasResult MsdfAtlasGenerator::RegenerateAtlas() {
    std::vector<uint32_t> codepoints = GetCodepoints();
    return GenerateAtlas(codepoints);
}

bool MsdfAtlasGenerator::AddCodepoints(const std::vector<uint32_t>& codepoints) {
    bool added = false;
    for (uint32_t codepoint : codepoints) {
        if (m_Codepoints.find(codepoint) == m_Codepoints.end()) {
            m_Codepoints.insert(codepoint);
            added = true;
        }
    }
    if (added) {
        m_Dirty = true;
    }
    return added;
}

bool MsdfAtlasGenerator::RemoveCodepoints(const std::vector<uint32_t>& codepoints) {
    bool removed = false;
    for (uint32_t codepoint : codepoints) {
        if (m_Codepoints.erase(codepoint) > 0) {
            removed = true;
        }
    }
    if (removed) {
        m_Dirty = true;
    }
    return removed;
}

bool MsdfAtlasGenerator::GetGlyphPlacement(uint32_t codepoint, AtlasGlyphPlacement& outPlacement) const {
    auto it = m_Placements.find(codepoint);
    if (it != m_Placements.end()) {
        outPlacement = it->second;
        return true;
    }
    return false;
}

std::vector<AtlasGlyphPlacement> MsdfAtlasGenerator::GetAllPlacements() const {
    std::vector<AtlasGlyphPlacement> placements;
    placements.reserve(m_Placements.size());
    
    for (const auto& [codepoint, placement] : m_Placements) {
        placements.push_back(placement);
    }
    
    return placements;
}

bool MsdfAtlasGenerator::UpdateConfig(const MsdfAtlasConfig& config) {
    std::vector<std::string> errors;
    if (!ValidateConfig(config, errors)) {
        return false;
    }
    
    m_Config = config;
    m_Dirty = true;
    return true;
}

std::vector<uint32_t> MsdfAtlasGenerator::GetCodepoints() const {
    std::vector<uint32_t> codepoints;
    codepoints.reserve(m_Codepoints.size());
    
    for (uint32_t codepoint : m_Codepoints) {
        codepoints.push_back(codepoint);
    }
    
    return codepoints;
}

bool MsdfAtlasGenerator::LoadPreGenerated(const std::string& atlasPath, const std::string& metadataPath) {
    // This would be implemented to load pre-generated MSDF atlases
    // For now, return false as this requires additional implementation
    HE_WARN("MsdfAtlasGenerator: LoadPreGenerated not yet implemented");
    return false;
}

bool MsdfAtlasGenerator::SaveToFile(const std::string& atlasPath, const std::string& metadataPath) const {
    // Save atlas image
    std::ofstream atlasFile(atlasPath, std::ios::binary);
    if (!atlasFile) {
        HE_ERROR("MsdfAtlasGenerator: Failed to open atlas file for writing: " + atlasPath);
        return false;
    }
    
    atlasFile.write(reinterpret_cast<const char*>(m_AtlasPixels.data()), m_AtlasPixels.size());
    atlasFile.close();

    // Save metadata as JSON
    std::ofstream metaFile(metadataPath);
    if (!metaFile) {
        HE_ERROR("MsdfAtlasGenerator: Failed to open metadata file for writing: " + metadataPath);
        return false;
    }

    metaFile << "{\n";
    metaFile << "  \"version\": 1,\n";
    metaFile << "  \"atlasWidth\": " << m_AtlasWidth << ",\n";
    metaFile << "  \"atlasHeight\": " << m_AtlasHeight << ",\n";
    metaFile << "  \"emSize\": " << m_Config.emSize << ",\n";
    metaFile << "  \"pixelRange\": " << m_Config.pixelRange << ",\n";
    metaFile << "  \"glyphs\": {\n";

    bool first = true;
    for (const auto& [codepoint, placement] : m_Placements) {
        if (!first) {
            metaFile << ",\n";
        }
        first = false;

        metaFile << "    \"" << codepoint << "\": {\n";
        metaFile << "      \"planeLeft\": " << placement.planeLeft << ",\n";
        metaFile << "      \"planeBottom\": " << placement.planeBottom << ",\n";
        metaFile << "      \"planeRight\": " << placement.planeRight << ",\n";
        metaFile << "      \"planeTop\": " << placement.planeTop << ",\n";
        metaFile << "      \"atlasLeft\": " << placement.atlasLeft << ",\n";
        metaFile << "      \"atlasBottom\": " << placement.atlasBottom << ",\n";
        metaFile << "      \"atlasRight\": " << placement.atlasRight << ",\n";
        metaFile << "      \"atlasTop\": " << placement.atlasTop << ",\n";
        metaFile << "      \"advance\": " << placement.advance << ",\n";
        metaFile << "      \"isWhitespace\": " << (placement.isWhitespace ? "true" : "false") << "\n";
        metaFile << "    }";
    }

    metaFile << "\n  }\n";
    metaFile << "}\n";
    metaFile.close();

    HE_INFO("MsdfAtlasGenerator: Saved atlas to " + atlasPath + " and metadata to " + metadataPath);
    return true;
}

// ============================================================================
// MsdfAtlasUtils Implementation
// ============================================================================

namespace MsdfAtlasUtils {

float CalculateOptimalPixelRange(float emSize, float targetScale) {
    // Recommended pixel range is approximately 2-4x the font size
    // For high-quality rendering at various scales
    return std::clamp(emSize * 0.25f * targetScale, 2.0f, 16.0f);
}

std::pair<int, int> EstimateAtlasSize(const std::vector<uint32_t>& codepoints,
                                       float emSize, int spacing) {
    // This is a rough estimation
    // In practice, you'd want to measure actual glyph sizes
    
    const float avgGlyphWidth = emSize * 0.6f;  // Average glyph width estimate
    const float avgGlyphHeight = emSize;        // Average glyph height estimate
    
    const int glyphsPerRow = static_cast<int>(std::sqrt(codepoints.size()));
    const int rows = (static_cast<int>(codepoints.size()) + glyphsPerRow - 1) / glyphsPerRow;
    
    const int estimatedWidth = static_cast<int>((glyphsPerRow * avgGlyphWidth) + spacing);
    const int estimatedHeight = static_cast<int>((rows * avgGlyphHeight) + spacing);
    
    // Round up to power of two
    int width = 1;
    while (width < estimatedWidth) width *= 2;
    
    int height = 1;
    while (height < estimatedHeight) height *= 2;
    
    return {width, height};
}

bool ValidateAtlasData(const std::vector<uint8_t>& pixels, int width, int height,
                       std::vector<std::string>& outErrors) {
    outErrors.clear();

    if (width <= 0 || height <= 0) {
        outErrors.push_back("Invalid atlas dimensions");
        return false;
    }

    const size_t expectedSize = static_cast<size_t>(width * height * 4);
    if (pixels.size() != expectedSize) {
        outErrors.push_back("Atlas pixel size mismatch: expected " + 
                           std::to_string(expectedSize) + ", got " + 
                           std::to_string(pixels.size()));
        return false;
    }

    // Check for obviously invalid values
    for (size_t i = 0; i < pixels.size(); ++i) {
        // All values should be valid (0-255)
        // No additional validation needed for RGBA8
    }

    return true;
}

bool ConvertAtlasFormat(const std::vector<uint8_t>& inputPixels,
                        std::vector<uint8_t>& outputPixels,
                        int width, int height,
                        const std::string& targetFormat) {
    if (targetFormat == "RGB8") {
        outputPixels.resize(width * height * 3);
        for (size_t i = 0; i < inputPixels.size() / 4; ++i) {
            outputPixels[i * 3 + 0] = inputPixels[i * 4 + 0];
            outputPixels[i * 3 + 1] = inputPixels[i * 4 + 1];
            outputPixels[i * 3 + 2] = inputPixels[i * 4 + 2];
        }
        return true;
    } else if (targetFormat == "R8") {
        outputPixels.resize(width * height);
        for (size_t i = 0; i < inputPixels.size() / 4; ++i) {
            // Use the median of RGB for single-channel
            outputPixels[i] = inputPixels[i * 4 + 1]; // Use green channel
        }
        return true;
    }

    return false;
}

bool GeneratePreview(const std::vector<uint8_t>& pixels, int width, int height,
                     const std::string& outputPath) {
    // This would save the atlas as a PNG/BMP for debugging
    // For now, just log the intent
    HE_INFO("MsdfAtlasUtils: Preview generation requested for " + outputPath + 
            " (not yet implemented)");
    return false;
}

MsdfAtlasConfig GetDefaultConfig() {
    MsdfAtlasConfig config;
    config.initialWidth = 1024;
    config.initialHeight = 1024;
    config.maxWidth = 4096;
    config.maxHeight = 4096;
    config.emSize = 16.0f;
    config.pixelRange = 4.0f;
    config.miterLimit = 1.0f;
    config.mode = MsdfMode::MSDF;
    config.edgeColoring = EdgeColoring::Simple;
    config.edgeColoringAngleThreshold = 3.0f;
    config.packingConstraint = PackingConstraint::PowerOfTwoSquare;
    config.spacing = 2;
    config.overlapSupport = true;
    config.scanlinePass = true;
    config.preprocessGeometry = true;
    config.generateAlpha = true;
    config.invertY = true;
    return config;
}

MsdfAtlasConfig GetHighQualityConfig() {
    MsdfAtlasConfig config = GetDefaultConfig();
    config.pixelRange = 8.0f;
    config.mode = MsdfMode::MTSDF;
    config.edgeColoring = EdgeColoring::DistanceCheck;
    config.edgeColoringAngleThreshold = 1.0f;
    config.miterLimit = 2.0f;
    config.spacing = 4;
    return config;
}

MsdfAtlasConfig GetPerformanceConfig() {
    MsdfAtlasConfig config = GetDefaultConfig();
    config.pixelRange = 2.0f;
    config.mode = MsdfMode::PSDF;
    config.edgeColoring = EdgeColoring::Simple;
    config.edgeColoringAngleThreshold = 5.0f;
    config.miterLimit = 1.0f;
    config.spacing = 1;
    config.overlapSupport = false;
    config.scanlinePass = false;
    return config;
}

} // namespace MsdfAtlasUtils

} // namespace we::UI::Text
