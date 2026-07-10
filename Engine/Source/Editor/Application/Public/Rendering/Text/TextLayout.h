#pragma once

#include "Application/Export.h"

#include "Rendering/Text/TextShaper.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace we::UI::Text {

/**
 * @brief Text alignment
 */
enum class TextAlignment : uint8_t {
    Left,
    Center,
    Right,
    Justify
};

/**
 * @brief Text wrapping mode
 */
enum class TextWrapMode : uint8_t {
    None,       // No wrapping
    Word,       // Wrap at word boundaries
    Character,  // Wrap at character boundaries
    Truncate    // Truncate with ellipsis
};

/**
 * @brief Text overflow behavior
 */
enum class TextOverflow : uint8_t {
    Visible,    // Show overflow
    Hidden,     // Hide overflow
    Ellipsis,   // Show ellipsis
    Clip        // Clip at boundary
};

/**
 * @brief Line of laid out text
 */
struct TextLine {
    std::vector<ShapedGlyph> glyphs;
    float startX = 0.0f;
    float startY = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float baseline = 0.0f;
    uint32_t startCharIndex = 0;
    uint32_t endCharIndex = 0;
    bool endsWithHyphen = false;
};

/**
 * @brief Layout result
 */
struct LayoutResult {
    bool success = false;
    std::string errorMessage;
    
    std::vector<TextLine> lines;
    float totalWidth = 0.0f;
    float totalHeight = 0.0f;
    uint32_t lineCount = 0;
    uint32_t charCount = 0;
    uint32_t truncatedChars = 0;
};

/**
 * @brief Text layout configuration
 */
struct TextLayoutConfig {
    float maxWidth = 0.0f;
    float maxHeight = 0.0f;
    TextAlignment alignment = TextAlignment::Left;
    TextWrapMode wrapMode = TextWrapMode::Word;
    TextOverflow overflow = TextOverflow::Visible;
    float lineHeight = 1.2f;  // Line height multiplier
    float lineSpacing = 0.0f;  // Additional spacing between lines
    bool preserveWhitespace = true;
    bool enableHyphenation = false;
};

/**
 * @brief Text layout interface
 * 
 * Provides text layout with wrapping, alignment, and multiline support.
 * Works with shaped glyphs from the text shaper.
 */
class APPLICATION_API ITextLayout {
public:
    virtual ~ITextLayout() = default;

    /**
     * @brief Initialize the text layout engine
     * @param config Layout configuration
     * @return true if successful, false otherwise
     */
    virtual bool Initialize(const TextLayoutConfig& config) = 0;

    /**
     * @brief Shutdown the text layout engine
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Layout text
     * @param shapedGlyphs Shaped glyphs from text shaper
     * @return LayoutResult
     */
    virtual LayoutResult Layout(const std::vector<ShapedGlyph>& shapedGlyphs) = 0;

    /**
     * @brief Layout text with explicit dimensions
     * @param shapedGlyphs Shaped glyphs
     * @param maxWidth Maximum width
     * @param maxHeight Maximum height
     * @return LayoutResult
     */
    virtual LayoutResult Layout(const std::vector<ShapedGlyph>& shapedGlyphs,
                                float maxWidth, float maxHeight) = 0;

    /**
     * @brief Get current configuration
     * @return TextLayoutConfig structure
     */
    virtual TextLayoutConfig GetConfig() const = 0;

    /**
     * @brief Update configuration
     * @param config New configuration
     * @return true if successful, false otherwise
     */
    virtual bool UpdateConfig(const TextLayoutConfig& config) = 0;

    /**
     * @brief Measure text without layout
     * @param shapedGlyphs Shaped glyphs
     * @param outWidth Output width
     * @param outHeight Output height
     */
    virtual void Measure(const std::vector<ShapedGlyph>& shapedGlyphs,
                         float& outWidth, float& outHeight) = 0;

    /**
     * @brief Get line count for text
     * @param shapedGlyphs Shaped glyphs
     * @return Number of lines
     */
    virtual uint32_t GetLineCount(const std::vector<ShapedGlyph>& shapedGlyphs) = 0;

    /**
     * @brief Check if layout engine is initialized
     * @return true if initialized, false otherwise
     */
    virtual bool IsInitialized() const = 0;
};

/**
 * @brief Standard text layout implementation
 */
class APPLICATION_API TextLayout : public ITextLayout {
public:
    TextLayout();
    ~TextLayout() override;

    // Disable copying
    TextLayout(const TextLayout&) = delete;
    TextLayout& operator=(const TextLayout&) = delete;

    bool Initialize(const TextLayoutConfig& config) override;
    void Shutdown() override;

    LayoutResult Layout(const std::vector<ShapedGlyph>& shapedGlyphs) override;
    LayoutResult Layout(const std::vector<ShapedGlyph>& shapedGlyphs,
                        float maxWidth, float maxHeight) override;

    TextLayoutConfig GetConfig() const override { return m_Config; }
    bool UpdateConfig(const TextLayoutConfig& config) override;

    void Measure(const std::vector<ShapedGlyph>& shapedGlyphs,
                 float& outWidth, float& outHeight) override;
    uint32_t GetLineCount(const std::vector<ShapedGlyph>& shapedGlyphs) override;

    bool IsInitialized() const override { return m_Initialized; }

private:
    /**
     * @brief Layout text without wrapping
     * @param shapedGlyphs Shaped glyphs
     * @return LayoutResult
     */
    LayoutResult LayoutNoWrap(const std::vector<ShapedGlyph>& shapedGlyphs);

    /**
     * @brief Layout text with word wrapping
     * @param shapedGlyphs Shaped glyphs
     * @param maxWidth Maximum width
     * @return LayoutResult
     */
    LayoutResult LayoutWordWrap(const std::vector<ShapedGlyph>& shapedGlyphs, float maxWidth);

    /**
     * @brief Layout text with character wrapping
     * @param shapedGlyphs Shaped glyphs
     * @param maxWidth Maximum width
     * @return LayoutResult
     */
    LayoutResult LayoutCharWrap(const std::vector<ShapedGlyph>& shapedGlyphs, float maxWidth);

    /**
     * @brief Apply alignment to lines
     * @param lines Lines to align
     * @param maxWidth Maximum width
     */
    void ApplyAlignment(std::vector<TextLine>& lines, float maxWidth);

    /**
     * @brief Apply line spacing
     * @param lines Lines to space
     */
    void ApplyLineSpacing(std::vector<TextLine>& lines);

    /**
     * @brief Truncate text to fit in bounds
     * @param lines Lines to truncate
     * @param maxWidth Maximum width
     * @param maxHeight Maximum height
     * @return LayoutResult
     */
    LayoutResult TruncateLines(std::vector<TextLine>& lines, float maxWidth, float maxHeight);

    /**
     * @brief Add ellipsis to a line
     * @param line Line to add ellipsis to
     * @param maxWidth Maximum width
     */
    void AddEllipsis(TextLine& line, float maxWidth);

    /**
     * @brief Calculate line height
     * @param glyphs Glyphs in line
     * @return Line height
     */
    float CalculateLineHeight(const std::vector<ShapedGlyph>& glyphs) const;

    /**
     * @brief Find word break position
     * @param glyphs Glyphs to search
     * @param startIndex Start index
     * @param maxWidth Maximum width
     * @return Break index
     */
    size_t FindWordBreak(const std::vector<ShapedGlyph>& glyphs, size_t startIndex, float maxWidth);

    /**
     * @brief Find character break position
     * @param glyphs Glyphs to search
     * @param startIndex Start index
     * @param maxWidth Maximum width
     * @return Break index
     */
    size_t FindCharBreak(const std::vector<ShapedGlyph>& glyphs, size_t startIndex, float maxWidth);

    TextLayoutConfig m_Config;
    bool m_Initialized = false;
};

/**
 * @brief Text layout utility functions
 */
namespace TextLayoutUtils {
    /**
     * @brief Get default layout configuration
     */
    TextLayoutConfig GetDefaultConfig();

    /**
     * @brief Get multiline layout configuration
     */
    TextLayoutConfig GetMultilineConfig();

    /**
     * @brief Get single-line layout configuration
     */
    TextLayoutConfig GetSingleLineConfig();

    /**
     * @brief Calculate optimal line height for a font size
     */
    float CalculateOptimalLineHeight(float fontSize);

    /**
     * @brief Estimate text height for a given width
     */
    float EstimateHeight(const std::vector<ShapedGlyph>& glyphs, float width, float lineHeight);

    /**
     * @brief Check if a glyph is a word boundary
     */
    bool IsWordBoundary(const ShapedGlyph& glyph);

    /**
     * @brief Check if a glyph is a line break
     */
    bool IsLineBreak(const ShapedGlyph& glyph);

    /**
     * @brief Get visual bounds of laid out text
     */
    std::array<float, 4> GetVisualBounds(const LayoutResult& layout);
};

} // namespace we::UI::Text
