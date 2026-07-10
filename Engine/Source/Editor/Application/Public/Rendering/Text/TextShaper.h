#pragma once

#include "Application/Export.h"

#include "Rendering/Text/GlyphManager.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

struct FT_FaceRec_;

namespace we::UI::Text {

/**
 * @brief Text direction for bidi support
 */
enum class TextDirection : uint8_t {
    LeftToRight,
    RightToLeft,
    Auto  // Detect automatically
};

/**
 * @brief Script type for shaping
 */
enum class ScriptType : uint8_t {
    Common,
    Latin,
    Arabic,
    Cyrillic,
    Greek,
    Hebrew,
    Devanagari,
    CJK,
    Unknown
};

/**
 * @brief Shaped glyph with positioning information
 */
struct ShapedGlyph {
    uint32_t codepoint = 0;
    uint32_t glyphIndex = 0;
    
    // Position (in font units)
    float x = 0.0f;
    float y = 0.0f;
    
    // Advance (in font units)
    float advanceX = 0.0f;
    float advanceY = 0.0f;
    
    // Offset from cursor (in font units)
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    
    // Glyph properties
    bool isClusterStart = true;
    bool isLigature = false;
    bool isWhitespace = false;
    
    // Ligature information
    uint32_t ligatureId = 0;
    uint32_t ligatureComponentCount = 0;
};

/**
 * @brief Shaping result
 */
struct ShapingResult {
    bool success = false;
    std::string errorMessage;
    
    std::vector<ShapedGlyph> glyphs;
    float totalAdvance = 0.0f;
    TextDirection direction = TextDirection::LeftToRight;
    ScriptType script = ScriptType::Common;
};

/**
 * @brief Text shaper configuration
 */
struct TextShaperConfig {
    bool enableKerning = true;
    bool enableLigatures = true;
    bool enableBidi = true;
    TextDirection defaultDirection = TextDirection::Auto;
    bool useHarfBuzz = true;  // Use HarfBuzz if available
    bool fallbackToSimple = true;  // Fallback to simple shaping if HarfBuzz fails
};

/**
 * @brief Text shaper interface
 * 
 * Provides text shaping with kerning, ligatures, and bidirectional text support.
 * Can use HarfBuzz for advanced shaping or fall back to simple FreeType-based shaping.
 */
class APPLICATION_API ITextShaper {
public:
    virtual ~ITextShaper() = default;

    /**
     * @brief Initialize the text shaper
     * @param config Shaping configuration
     * @return true if successful, false otherwise
     */
    virtual bool Initialize(const TextShaperConfig& config) = 0;

    /**
     * @brief Shutdown the text shaper
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Set the font face for shaping
     * @param face FreeType face
     */
    virtual void SetFontFace(FT_FaceRec_* face) = 0;

    /**
     * @brief Set font size
     * @param size Font size in pixels
     */
    virtual void SetFontSize(float size) = 0;

    /**
     * @brief Shape text
     * @param text UTF-8 text to shape
     * @param codepoints Pre-decoded codepoints (optional, will decode if null)
     * @return ShapingResult
     */
    virtual ShapingResult Shape(std::string_view text, const std::vector<uint32_t>* codepoints = nullptr) = 0;

    /**
     * @brief Shape text with explicit direction
     * @param text UTF-8 text to shape
     * @param direction Text direction
     * @return ShapingResult
     */
    virtual ShapingResult Shape(std::string_view text, TextDirection direction) = 0;

    /**
     * @brief Detect text direction
     * @param text UTF-8 text
     * @param codepoints Pre-decoded codepoints (optional)
     * @return Detected direction
     */
    virtual TextDirection DetectDirection(std::string_view text, 
                                          const std::vector<uint32_t>* codepoints = nullptr) = 0;

    /**
     * @brief Detect script type
     * @param text UTF-8 text
     * @param codepoints Pre-decoded codepoints (optional)
     * @return Detected script type
     */
    virtual ScriptType DetectScript(std::string_view text,
                                    const std::vector<uint32_t>* codepoints = nullptr) = 0;

    /**
     * @brief Get kerning between two glyphs
     * @param leftGlyph Left glyph index
     * @param rightGlyph Right glyph index
     * @return Kerning adjustment in font units
     */
    virtual float GetKerning(uint32_t leftGlyph, uint32_t rightGlyph) = 0;

    /**
     * @brief Get current configuration
     * @return TextShaperConfig structure
     */
    virtual TextShaperConfig GetConfig() const = 0;

    /**
     * @brief Update configuration
     * @param config New configuration
     * @return true if successful, false otherwise
     */
    virtual bool UpdateConfig(const TextShaperConfig& config) = 0;

    /**
     * @brief Check if HarfBuzz is available
     * @return true if available, false otherwise
     */
    virtual bool HasHarfBuzz() const = 0;

    /**
     * @brief Check if shaper is initialized
     * @return true if initialized, false otherwise
     */
    virtual bool IsInitialized() const = 0;
};

/**
 * @brief Standard text shaper implementation
 */
class APPLICATION_API TextShaper : public ITextShaper {
public:
    TextShaper();
    ~TextShaper() override;

    // Disable copying
    TextShaper(const TextShaper&) = delete;
    TextShaper& operator=(const TextShaper&) = delete;

    bool Initialize(const TextShaperConfig& config) override;
    void Shutdown() override;

    void SetFontFace(FT_FaceRec_* face) override;
    void SetFontSize(float size) override;

    ShapingResult Shape(std::string_view text, const std::vector<uint32_t>* codepoints = nullptr) override;
    ShapingResult Shape(std::string_view text, TextDirection direction) override;

    TextDirection DetectDirection(std::string_view text,
                                  const std::vector<uint32_t>* codepoints = nullptr) override;
    ScriptType DetectScript(std::string_view text,
                            const std::vector<uint32_t>* codepoints = nullptr) override;

    float GetKerning(uint32_t leftGlyph, uint32_t rightGlyph) override;

    TextShaperConfig GetConfig() const override { return m_Config; }
    bool UpdateConfig(const TextShaperConfig& config) override;

    bool HasHarfBuzz() const override { return m_HasHarfBuzz; }
    bool IsInitialized() const override { return m_Initialized; }

private:
    /**
     * @brief Shape using HarfBuzz
     * @param text UTF-8 text
     * @param codepoints Pre-decoded codepoints
     * @param direction Text direction
     * @return ShapingResult
     */
    ShapingResult ShapeWithHarfBuzz(std::string_view text, const std::vector<uint32_t>* codepoints,
                                     TextDirection direction);

    /**
     * @brief Shape using simple FreeType-based shaping
     * @param codepoints Codepoints to shape
     * @param direction Text direction
     * @return ShapingResult
     */
    ShapingResult ShapeSimple(const std::vector<uint32_t>& codepoints, TextDirection direction);

    /**
     * @brief Apply kerning to shaped glyphs
     * @param glyphs Shaped glyphs
     */
    void ApplyKerning(std::vector<ShapedGlyph>& glyphs);

    /**
     * @brief Reorder glyphs for bidirectional text
     * @param glyphs Shaped glyphs
     * @param direction Text direction
     */
    void ReorderBidi(std::vector<ShapedGlyph>& glyphs, TextDirection direction);

    /**
     * @brief Check if HarfBuzz is available
     * @return true if available, false otherwise
     */
    bool CheckHarfBuzzAvailability();

    FT_FaceRec_* m_Face = nullptr;
    float m_FontSize = 0.0f;
    TextShaperConfig m_Config;
    
    bool m_HasHarfBuzz = false;
    bool m_Initialized = false;
};

/**
 * @brief Text shaper utility functions
 */
namespace TextShaperUtils {
    /**
     * @brief Check if a codepoint is RTL (right-to-left)
     */
    bool IsRTL(uint32_t codepoint);

    /**
     * @brief Check if a codepoint is a ligature component
     */
    bool IsLigatureComponent(uint32_t codepoint);

    /**
     * @brief Get script type for a codepoint
     */
    ScriptType GetScriptForCodepoint(uint32_t codepoint);

    /**
     * @brief Detect dominant script from codepoints
     */
    ScriptType DetectDominantScript(const std::vector<uint32_t>& codepoints);

    /**
     * @brief Check if text contains bidirectional characters
     */
    bool ContainsBidi(const std::vector<uint32_t>& codepoints);

    /**
     * @brief Get default shaper configuration
     */
    TextShaperConfig GetDefaultConfig();

    /**
     * @brief Get high-quality shaper configuration
     */
    TextShaperConfig GetHighQualityConfig();

    /**
     * @brief Get performance-oriented shaper configuration
     */
    TextShaperConfig GetPerformanceConfig();
};

} // namespace we::UI::Text
