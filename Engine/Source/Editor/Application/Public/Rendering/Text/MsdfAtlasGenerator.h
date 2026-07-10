#pragma once

#include "Application/Export.h"

#include "Rendering/Text/GlyphManager.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <functional>

struct FT_FaceRec_;

namespace msdf_atlas {
class GlyphGeometry;
class FontGeometry;
}

namespace we::UI::Text {

/**
 * @brief MSDF generation mode
 */
enum class MsdfMode : uint8_t {
    MSDF,       // Multi-channel signed distance field (default)
    SDF,        // Single-channel signed distance field
    PSDF,       // Pseudo-distance field (faster, lower quality)
    MTSDF       // Multi-channel true-distance field (highest quality)
};

/**
 * @brief Atlas packing constraint
 */
enum class PackingConstraint : uint8_t {
    PowerOfTwoSquare,    // Power-of-two square texture (default)
    PowerOfTwoRectangle, // Power-of-two rectangle
    Square,              // Square texture
    Rectangle            // Rectangle texture
};

/**
 * @brief Edge coloring mode for MSDF
 */
enum class EdgeColoring : uint8_t {
    Simple,      // Simple edge coloring (default)
    InkTrap,     // Ink trap edge coloring
    DistanceCheck // Distance-based edge coloring
};

/**
 * @brief MSDF atlas configuration
 */
struct MsdfAtlasConfig {
    // Atlas dimensions
    int initialWidth = 1024;
    int initialHeight = 1024;
    int maxWidth = 4096;
    int maxHeight = 4096;
    
    // MSDF generation parameters
    float emSize = 16.0f;
    float pixelRange = 4.0f;
    float miterLimit = 1.0f;
    MsdfMode mode = MsdfMode::MSDF;
    EdgeColoring edgeColoring = EdgeColoring::Simple;
    float edgeColoringAngleThreshold = 3.0f;
    
    // Packing parameters
    PackingConstraint packingConstraint = PackingConstraint::PowerOfTwoSquare;
    int spacing = 2;
    
    // Generation options
    bool overlapSupport = true;
    bool scanlinePass = true;
    bool preprocessGeometry = true;
    
    // Texture format
    bool generateAlpha = true;  // Generate alpha channel from distance
    bool invertY = true;        // Flip Y axis for Vulkan/DirectX
};

/**
 * @brief Atlas generation result
 */
struct MsdfAtlasResult {
    bool success = false;
    std::string errorMessage;
    
    // Atlas data
    std::vector<uint8_t> rgbaPixels;
    int atlasWidth = 0;
    int atlasHeight = 0;
    
    // Generation parameters used
    MsdfAtlasConfig config;
    
    // Statistics
    size_t glyphCount = 0;
    size_t missingGlyphs = 0;
    float packingEfficiency = 0.0f;
};

/**
 * @brief Glyph placement in atlas
 */
struct AtlasGlyphPlacement {
    uint32_t codepoint = 0;
    
    // Plane bounds (in font units)
    float planeLeft = 0.0f;
    float planeBottom = 0.0f;
    float planeRight = 0.0f;
    float planeTop = 0.0f;
    
    // Atlas bounds (normalized UV coordinates)
    float atlasLeft = 0.0f;
    float atlasBottom = 0.0f;
    float atlasRight = 0.0f;
    float atlasTop = 0.0f;
    
    // Atlas position (in pixels)
    int atlasX = 0;
    int atlasY = 0;
    int atlasWidth = 0;
    int atlasHeight = 0;
    
    // Glyph properties
    float advance = 0.0f;
    bool isWhitespace = false;
    bool hasDrawableQuad = false;
};

/**
 * @brief MSDF atlas generator interface
 * 
 * Generates MSDF font atlases using msdf-atlas-gen library.
 * Supports TTF, OTF, and pre-generated MSDF assets.
 * Produces RGBA8 UNORM atlas textures for GPU rendering.
 */
class APPLICATION_API IMsdfAtlasGenerator {
public:
    virtual ~IMsdfAtlasGenerator() = default;

    /**
     * @brief Initialize the atlas generator
     * @param config Generation configuration
     * @return true if successful, false otherwise
     */
    virtual bool Initialize(const MsdfAtlasConfig& config) = 0;

    /**
     * @brief Shutdown the atlas generator
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Set the font face for atlas generation
     * @param face FreeType face
     */
    virtual void SetFontFace(FT_FaceRec_* face) = 0;

    /**
     * @brief Generate atlas for a set of codepoints
     * @param codepoints Vector of Unicode codepoints
     * @return MsdfAtlasResult with atlas data and placements
     */
    virtual MsdfAtlasResult GenerateAtlas(const std::vector<uint32_t>& codepoints) = 0;

    /**
     * @brief Regenerate atlas with current codepoints
     * @return MsdfAtlasResult with atlas data and placements
     */
    virtual MsdfAtlasResult RegenerateAtlas() = 0;

    /**
     * @brief Add codepoints to the atlas
     * @param codepoints Vector of Unicode codepoints to add
     * @return true if atlas needs regeneration, false otherwise
     */
    virtual bool AddCodepoints(const std::vector<uint32_t>& codepoints) = 0;

    /**
     * @brief Remove codepoints from the atlas
     * @param codepoints Vector of Unicode codepoints to remove
     * @return true if atlas needs regeneration, false otherwise
     */
    virtual bool RemoveCodepoints(const std::vector<uint32_t>& codepoints) = 0;

    /**
     * @brief Get glyph placement for a codepoint
     * @param codepoint Unicode codepoint
     * @param outPlacement Output placement data
     * @return true if found, false otherwise
     */
    virtual bool GetGlyphPlacement(uint32_t codepoint, AtlasGlyphPlacement& outPlacement) const = 0;

    /**
     * @brief Get all glyph placements
     * @return Vector of glyph placements
     */
    virtual std::vector<AtlasGlyphPlacement> GetAllPlacements() const = 0;

    /**
     * @brief Get atlas pixels
     * @return Vector of RGBA8 pixels
     */
    virtual const std::vector<uint8_t>& GetAtlasPixels() const = 0;

    /**
     * @brief Get atlas dimensions
     * @return Pair of (width, height)
     */
    virtual std::pair<int, int> GetAtlasDimensions() const = 0;

    /**
     * @brief Get current configuration
     * @return MsdfAtlasConfig structure
     */
    virtual MsdfAtlasConfig GetConfig() const = 0;

    /**
     * @brief Update configuration
     * @param config New configuration
     * @return true if successful, false otherwise
     */
    virtual bool UpdateConfig(const MsdfAtlasConfig& config) = 0;

    /**
     * @brief Check if atlas is dirty (needs regeneration)
     * @return true if dirty, false otherwise
     */
    virtual bool IsDirty() const = 0;

    /**
     * @brief Clear dirty flag
     */
    virtual void ClearDirty() = 0;

    /**
     * @brief Get number of codepoints in atlas
     * @return Number of codepoints
     */
    virtual size_t GetCodepointCount() const = 0;

    /**
     * @brief Get all codepoints in atlas
     * @return Vector of codepoints
     */
    virtual std::vector<uint32_t> GetCodepoints() const = 0;

    /**
     * @brief Load pre-generated MSDF atlas from file
     * @param atlasPath Path to atlas image file
     * @param metadataPath Path to metadata file (JSON)
     * @return true if successful, false otherwise
     */
    virtual bool LoadPreGenerated(const std::string& atlasPath, const std::string& metadataPath) = 0;

    /**
     * @brief Save generated atlas to file
     * @param atlasPath Path to save atlas image
     * @param metadataPath Path to save metadata (JSON)
     * @return true if successful, false otherwise
     */
    virtual bool SaveToFile(const std::string& atlasPath, const std::string& metadataPath) const = 0;
};

/**
 * @brief Standard MSDF atlas generator implementation
 */
class APPLICATION_API MsdfAtlasGenerator : public IMsdfAtlasGenerator {
public:
    MsdfAtlasGenerator();
    ~MsdfAtlasGenerator() override;

    // Disable copying
    MsdfAtlasGenerator(const MsdfAtlasGenerator&) = delete;
    MsdfAtlasGenerator& operator=(const MsdfAtlasGenerator&) = delete;

    bool Initialize(const MsdfAtlasConfig& config) override;
    void Shutdown() override;

    void SetFontFace(FT_FaceRec_* face) override;

    MsdfAtlasResult GenerateAtlas(const std::vector<uint32_t>& codepoints) override;
    MsdfAtlasResult RegenerateAtlas() override;

    bool AddCodepoints(const std::vector<uint32_t>& codepoints) override;
    bool RemoveCodepoints(const std::vector<uint32_t>& codepoints) override;

    bool GetGlyphPlacement(uint32_t codepoint, AtlasGlyphPlacement& outPlacement) const override;
    std::vector<AtlasGlyphPlacement> GetAllPlacements() const override;

    const std::vector<uint8_t>& GetAtlasPixels() const override { return m_AtlasPixels; }
    std::pair<int, int> GetAtlasDimensions() const override { return {m_AtlasWidth, m_AtlasHeight}; }

    MsdfAtlasConfig GetConfig() const override { return m_Config; }
    bool UpdateConfig(const MsdfAtlasConfig& config) override;

    bool IsDirty() const override { return m_Dirty; }
    void ClearDirty() override { m_Dirty = false; }

    size_t GetCodepointCount() const override { return m_Codepoints.size(); }
    std::vector<uint32_t> GetCodepoints() const override;

    bool LoadPreGenerated(const std::string& atlasPath, const std::string& metadataPath) override;
    bool SaveToFile(const std::string& atlasPath, const std::string& metadataPath) const override;

private:
    /**
     * @brief Generate MSDF atlas using msdf-atlas-gen
     * @param codepoints Vector of codepoints
     * @return MsdfAtlasResult
     */
    MsdfAtlasResult GenerateMsdfAtlas(const std::vector<uint32_t>& codepoints);

    /**
     * @brief Build glyph placement from msdf-atlas-gen geometry
     * @param glyph Glyph geometry
     * @param atlasWidth Atlas width
     * @param atlasHeight Atlas height
     * @return AtlasGlyphPlacement
     */
    AtlasGlyphPlacement BuildPlacement(const msdf_atlas::GlyphGeometry& glyph, 
                                       int atlasWidth, int atlasHeight) const;

    /**
     * @brief Convert packing constraint to msdf-atlas-gen enum
     */
    int ConvertPackingConstraint(PackingConstraint constraint) const;

    /**
     * @brief Convert MSDF mode to template parameter
     */
    int GetMsdfChannels() const;

    /**
     * @brief Validate configuration
     * @param config Configuration to validate
     * @param outErrors Output error messages
     * @return true if valid, false otherwise
     */
    bool ValidateConfig(const MsdfAtlasConfig& config, std::vector<std::string>& outErrors) const;

    FT_FaceRec_* m_Face = nullptr;
    MsdfAtlasConfig m_Config;
    
    std::unordered_set<uint32_t> m_Codepoints;
    std::unordered_map<uint32_t, AtlasGlyphPlacement> m_Placements;
    
    std::vector<uint8_t> m_AtlasPixels;
    int m_AtlasWidth = 0;
    int m_AtlasHeight = 0;
    
    bool m_Dirty = false;
    bool m_Initialized = false;
};

/**
 * @brief MSDF atlas generator utility functions
 */
namespace MsdfAtlasUtils {
    /**
     * @brief Calculate optimal pixel range for a font size
     * @param emSize Font size in pixels
     * @param targetScale Target scale factor
     * @return Recommended pixel range
     */
    float CalculateOptimalPixelRange(float emSize, float targetScale = 1.0f);

    /**
     * @brief Estimate atlas size needed for a set of codepoints
     * @param codepoints Vector of codepoints
     * @param emSize Font size
     * @param spacing Glyph spacing
     * @return Estimated size in pixels
     */
    std::pair<int, int> EstimateAtlasSize(const std::vector<uint32_t>& codepoints, 
                                          float emSize, int spacing);

    /**
     * @brief Validate MSDF atlas data
     * @param pixels Atlas pixels
     * @param width Atlas width
     * @param height Atlas height
     * @param outErrors Output error messages
     * @return true if valid, false otherwise
     */
    bool ValidateAtlasData(const std::vector<uint8_t>& pixels, int width, int height,
                           std::vector<std::string>& outErrors);

    /**
     * @brief Convert atlas pixels to different format
     * @param inputPixels Input RGBA8 pixels
     * @param outputPixels Output pixels
     * @param width Width
     * @param height Height
     * @param targetFormat Target format (e.g., "RGB8", "R8")
     * @return true if successful, false otherwise
     */
    bool ConvertAtlasFormat(const std::vector<uint8_t>& inputPixels,
                            std::vector<uint8_t>& outputPixels,
                            int width, int height,
                            const std::string& targetFormat);

    /**
     * @brief Generate atlas preview image (for debugging)
     * @param pixels Atlas pixels
     * @param width Width
     * @param height Height
     * @param outputPath Path to save preview
     * @return true if successful, false otherwise
     */
    bool GeneratePreview(const std::vector<uint8_t>& pixels, int width, int height,
                         const std::string& outputPath);

    /**
     * @brief Get default MSDF configuration
     * @return MsdfAtlasConfig with sensible defaults
     */
    MsdfAtlasConfig GetDefaultConfig();

    /**
     * @brief Get high-quality MSDF configuration
     * @return MsdfAtlasConfig for high-quality rendering
     */
    MsdfAtlasConfig GetHighQualityConfig();

    /**
     * @brief Get performance-oriented MSDF configuration
     * @return MsdfAtlasConfig for faster generation
     */
    MsdfAtlasConfig GetPerformanceConfig();
};

} // namespace we::UI::Text
