#pragma once

#include "Application/Export.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <system_error>

struct FT_LibraryRec_;
struct FT_FaceRec_;

namespace we::UI::Text {

/**
 * @brief Font file format
 */
enum class FontFormat : uint8_t {
    Unknown,
    TrueType,    // .ttf
    OpenType,    // .otf
    WOFF,        // .woff
    WOFF2,       // .woff2
    Type1,       // .pfb, .pfa
    CID,         // CID-keyed fonts
    Bitmap       // .bdf, .pcf
};

/**
 * @brief Font style and weight information
 */
enum class FontWeight : uint16_t {
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    Regular = 400,
    Medium = 500,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Black = 900
};

enum class FontStyle : uint8_t {
    Normal,
    Italic,
    Oblique
};

enum class FontStretch : uint8_t {
    UltraCondensed,
    ExtraCondensed,
    Condensed,
    SemiCondensed,
    Normal,
    SemiExpanded,
    Expanded,
    ExtraExpanded,
    UltraExpanded
};

/**
 * @brief Font metrics extracted from font file
 */
struct FontMetrics {
    float unitsPerEm = 0.0f;
    float ascender = 0.0f;
    float descender = 0.0f;
    float lineHeight = 0.0f;
    float underlinePosition = 0.0f;
    float underlineThickness = 0.0f;
    float strikeoutPosition = 0.0f;
    float strikeoutThickness = 0.0f;
    float capHeight = 0.0f;
    float xHeight = 0.0f;
    
    // Bounding box
    float bboxMinX = 0.0f;
    float bboxMinY = 0.0f;
    float bboxMaxX = 0.0f;
    float bboxMaxY = 0.0f;
};

/**
 * @brief Font information
 */
struct FontInfo {
    std::string familyName;
    std::string styleName;
    std::string copyright;
    std::string version;
    std::string trademark;
    std::string manufacturer;
    std::string designer;
    std::string license;
    std::string licenseUrl;
    
    FontWeight weight = FontWeight::Regular;
    FontStyle style = FontStyle::Normal;
    FontStretch stretch = FontStretch::Normal;
    
    bool isFixedPitch = false;
    bool hasVerticalMetrics = false;
    bool isVariableFont = false;
    
    uint32_t glyphCount = 0;
    uint16_t faceIndex = 0;
    uint16_t faceCount = 1;
    
    FontMetrics metrics;
};

/**
 * @brief Result of font loading operation
 */
struct FontLoadResult {
    bool success = false;
    std::string errorMessage;
    FontInfo info;
    FT_FaceRec_* face = nullptr;
};

/**
 * @brief Font loader configuration
 */
struct FontLoaderConfig {
    bool loadKerning = true;
    bool loadBitmaps = false;
    bool forceAutoHint = false;
    uint32_t loadFlags = 0; // FreeType load flags
    uint32_t renderFlags = 0; // FreeType render flags
};

/**
 * @brief Font loader interface
 * 
 * Provides font file loading using FreeType library.
 * Supports TTF, OTF, and other font formats.
 * Extracts font metrics and information.
 */
class APPLICATION_API IFontLoader {
public:
    virtual ~IFontLoader() = default;

    /**
     * @brief Initialize the font loader
     * @return true if successful, false otherwise
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Shutdown the font loader
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Load a font from file
     * @param fontPath Path to font file
     * @param faceIndex Face index for font collections (0 for single-face fonts)
     * @param config Loading configuration
     * @return FontLoadResult with font information and FreeType face
     */
    virtual FontLoadResult LoadFont(const std::string& fontPath, uint32_t faceIndex = 0, 
                                    const FontLoaderConfig& config = {}) = 0;

    /**
     * @brief Load a font from memory
     * @param data Font data in memory
     * @param dataSize Size of font data in bytes
     * @param faceIndex Face index for font collections
     * @param config Loading configuration
     * @return FontLoadResult with font information and FreeType face
     */
    virtual FontLoadResult LoadFontFromMemory(const void* data, size_t dataSize, uint32_t faceIndex = 0,
                                              const FontLoaderConfig& config = {}) = 0;

    /**
     * @brief Unload a font face
     * @param face FreeType face to unload
     */
    virtual void UnloadFont(FT_FaceRec_* face) = 0;

    /**
     * @brief Set font size in pixels
     * @param face FreeType face
     * @param width Width in pixels (0 for same as height)
     * @param height Height in pixels
     * @return true if successful, false otherwise
     */
    virtual bool SetFontSize(FT_FaceRec_* face, uint32_t width, uint32_t height) = 0;

    /**
     * @brief Set font size in points
     * @param face FreeType face
     * @param width Width in points (0 for same as height)
     * @param height Height in points
     * @param horizontalDPI Horizontal DPI
     * @param verticalDPI Vertical DPI
     * @return true if successful, false otherwise
     */
    virtual bool SetFontSizePoints(FT_FaceRec_* face, uint32_t width, uint32_t height,
                                   uint32_t horizontalDPI, uint32_t verticalDPI) = 0;

    /**
     * @brief Get font information for a loaded face
     * @param face FreeType face
     * @return FontInfo structure
     */
    virtual FontInfo GetFontInfo(FT_FaceRec_* face) const = 0;

    /**
     * @brief Get font metrics for a loaded face
     * @param face FreeType face
     * @return FontMetrics structure
     */
    virtual FontMetrics GetFontMetrics(FT_FaceRec_* face) const = 0;

    /**
     * @brief Detect font format from file extension or content
     * @param fontPath Path to font file
     * @return Detected font format
     */
    virtual FontFormat DetectFormat(const std::string& fontPath) const = 0;

    /**
     * @brief Validate font file
     * @param fontPath Path to font file
     * @return true if valid, false otherwise
     */
    virtual bool ValidateFontFile(const std::string& fontPath) const = 0;

    /**
     * @brief Get number of faces in a font collection
     * @param fontPath Path to font file
     * @return Number of faces, or 0 if error
     */
    virtual uint32_t GetFaceCount(const std::string& fontPath) const = 0;

    /**
     * @brief Get FreeType library handle (for advanced use)
     * @return FreeType library handle
     */
    virtual FT_LibraryRec_* GetFreeTypeLibrary() const = 0;

    /**
     * @brief Check if font loader is initialized
     * @return true if initialized, false otherwise
     */
    virtual bool IsInitialized() const = 0;
};

/**
 * @brief Standard font loader implementation using FreeType
 */
class APPLICATION_API FontLoader : public IFontLoader {
public:
    FontLoader();
    ~FontLoader() override;

    // Disable copying
    FontLoader(const FontLoader&) = delete;
    FontLoader& operator=(const FontLoader&) = delete;

    bool Initialize() override;
    void Shutdown() override;

    FontLoadResult LoadFont(const std::string& fontPath, uint32_t faceIndex = 0,
                            const FontLoaderConfig& config = {}) override;
    FontLoadResult LoadFontFromMemory(const void* data, size_t dataSize, uint32_t faceIndex = 0,
                                      const FontLoaderConfig& config = {}) override;
    void UnloadFont(FT_FaceRec_* face) override;

    bool SetFontSize(FT_FaceRec_* face, uint32_t width, uint32_t height) override;
    bool SetFontSizePoints(FT_FaceRec_* face, uint32_t width, uint32_t height,
                           uint32_t horizontalDPI, uint32_t verticalDPI) override;

    FontInfo GetFontInfo(FT_FaceRec_* face) const override;
    FontMetrics GetFontMetrics(FT_FaceRec_* face) const override;

    FontFormat DetectFormat(const std::string& fontPath) const override;
    bool ValidateFontFile(const std::string& fontPath) const override;
    uint32_t GetFaceCount(const std::string& fontPath) const override;

    FT_LibraryRec_* GetFreeTypeLibrary() const override { return m_FreeTypeLibrary; }
    bool IsInitialized() const override { return m_FreeTypeLibrary != nullptr; }

private:
    /**
     * @brief Extract font information from FreeType face
     * @param face FreeType face
     * @return FontInfo structure
     */
    FontInfo ExtractFontInfo(FT_FaceRec_* face) const;

    /**
     * @brief Extract font metrics from FreeType face
     * @param face FreeType face
     * @return FontMetrics structure
     */
    FontMetrics ExtractFontMetrics(FT_FaceRec_* face) const;

    /**
     * @brief Resolve font path with search directories
     * @param fontPath Input font path
     * @return Resolved absolute path
     */
    std::string ResolveFontPath(const std::string& fontPath) const;

    /**
     * @brief Apply loading configuration to face
     * @param face FreeType face
     * @param config Loading configuration
     */
    void ApplyConfig(FT_FaceRec_* face, const FontLoaderConfig& config) const;

    FT_LibraryRec_* m_FreeTypeLibrary = nullptr;
};

/**
 * @brief Font file validator for startup validation
 */
class APPLICATION_API FontValidator {
public:
    /**
     * @brief Validate font file with detailed error reporting
     * @param fontPath Path to font file
     * @param outErrors Output error messages
     * @return true if valid, false otherwise
     */
    static bool Validate(const std::string& fontPath, std::vector<std::string>& outErrors);

    /**
     * @brief Quick validation check
     * @param fontPath Path to font file
     * @return true if valid, false otherwise
     */
    static bool QuickValidate(const std::string& fontPath);

    /**
     * @brief Get font file information without loading
     * @param fontPath Path to font file
     * @param outInfo Output font information
     * @return true if successful, false otherwise
     */
    static bool GetFileInfo(const std::string& fontPath, FontInfo& outInfo);
};

/**
 * @brief Font loader utility functions
 */
namespace FontLoaderUtils {
    /**
     * @brief Convert weight enum to numeric value
     */
    inline uint16_t WeightToValue(FontWeight weight) {
        return static_cast<uint16_t>(weight);
    }

    /**
     * @brief Convert numeric value to weight enum
     */
    FontWeight ValueToWeight(uint16_t value);

    /**
     * @brief Get default font search paths
     */
    std::vector<std::string> GetDefaultSearchPaths();

    /**
     * @brief Find font file by family name in system directories
     * @param familyName Font family name (e.g., "Arial", "Inter")
     * @param style Optional style name (e.g., "Bold", "Italic")
     * @return Path to font file, or empty if not found
     */
    std::string FindSystemFont(const std::string& familyName, const std::string& style = "");

    /**
     * @brief List all available fonts in a directory
     * @param directory Directory to search
     * @return List of font file paths
     */
    std::vector<std::string> ListFontsInDirectory(const std::string& directory);

    /**
     * @brief Check if font supports a specific script
     * @param face FreeType face
     * @param script Script tag (e.g., "latn", "arab", "cyrl")
     * @return true if supported, false otherwise
     */
    bool SupportsScript(FT_FaceRec_* face, const std::string& script);

    /**
     * @brief Get supported Unicode ranges for a font
     * @param face FreeType face
     * @return Vector of (start, end) codepoint ranges
     */
    std::vector<std::pair<uint32_t, uint32_t>> GetUnicodeRanges(FT_FaceRec_* face);
};

} // namespace we::UI::Text
