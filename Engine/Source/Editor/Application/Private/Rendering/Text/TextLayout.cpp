#include "Rendering/Text/TextLayout.h"
#include "Core/Logger.h"

#include <algorithm>
#include <array>

namespace we::UI::Text {

// ============================================================================
// TextLayout Implementation
// ============================================================================

TextLayout::TextLayout() = default;

TextLayout::~TextLayout() {
    Shutdown();
}

bool TextLayout::Initialize(const TextLayoutConfig& config) {
    if (m_Initialized) {
        HE_WARN("TextLayout: Already initialized");
        return true;
    }

    m_Config = config;
    m_Initialized = true;

    HE_INFO("TextLayout: Initialized");
    return true;
}

void TextLayout::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    m_Initialized = false;
    HE_INFO("TextLayout: Shutdown");
}

LayoutResult TextLayout::Layout(const std::vector<ShapedGlyph>& shapedGlyphs) {
    if (!m_Initialized) {
        LayoutResult result;
        result.success = false;
        result.errorMessage = "Layout engine not initialized";
        return result;
    }

    return Layout(shapedGlyphs, m_Config.maxWidth, m_Config.maxHeight);
}

LayoutResult TextLayout::Layout(const std::vector<ShapedGlyph>& shapedGlyphs,
                                 float maxWidth, float maxHeight) {
    LayoutResult result;

    if (shapedGlyphs.empty()) {
        result.success = true;
        return result;
    }

    // Determine wrapping mode
    if (m_Config.wrapMode == TextWrapMode::None || maxWidth <= 0.0f) {
        result = LayoutNoWrap(shapedGlyphs);
    } else if (m_Config.wrapMode == TextWrapMode::Word) {
        result = LayoutWordWrap(shapedGlyphs, maxWidth);
    } else if (m_Config.wrapMode == TextWrapMode::Character) {
        result = LayoutCharWrap(shapedGlyphs, maxWidth);
    } else {
        result = LayoutNoWrap(shapedGlyphs);
    }

    // Apply alignment
    if (result.success && maxWidth > 0.0f) {
        ApplyAlignment(result.lines, maxWidth);
    }

    // Apply line spacing
    ApplyLineSpacing(result.lines);

    // Handle overflow
    if (maxHeight > 0.0f && m_Config.overflow != TextOverflow::Visible) {
        result = TruncateLines(result.lines, maxWidth, maxHeight);
    }

    // Calculate total dimensions
    result.totalWidth = maxWidth > 0.0f ? maxWidth : result.totalWidth;
    result.totalHeight = 0.0f;
    for (const auto& line : result.lines) {
        result.totalHeight += line.height;
    }

    result.lineCount = static_cast<uint32_t>(result.lines.size());
    result.charCount = static_cast<uint32_t>(shapedGlyphs.size());

    return result;
}

LayoutResult TextLayout::LayoutNoWrap(const std::vector<ShapedGlyph>& shapedGlyphs) {
    LayoutResult result;
    result.success = true;

    if (shapedGlyphs.empty()) {
        return result;
    }

    TextLine line;
    line.glyphs = shapedGlyphs;
    line.startCharIndex = 0;
    line.endCharIndex = static_cast<uint32_t>(shapedGlyphs.size());

    // Calculate line dimensions
    float minX = FLT_MAX;
    float maxX = FLT_MIN;
    float minY = FLT_MAX;
    float maxY = FLT_MIN;

    for (const auto& glyph : shapedGlyphs) {
        minX = std::min(minX, glyph.x);
        maxX = std::max(maxX, glyph.x + glyph.advanceX);
        minY = std::min(minY, glyph.y);
        maxY = std::max(maxY, glyph.y + glyph.advanceY);
    }

    line.width = maxX - minX;
    line.height = maxY - minY;
    line.startX = minX;
    line.startY = minY;
    line.baseline = 0.0f;  // Would be set from font metrics

    result.lines.push_back(line);
    result.totalWidth = line.width;
    result.totalHeight = line.height;

    return result;
}

LayoutResult TextLayout::LayoutWordWrap(const std::vector<ShapedGlyph>& shapedGlyphs, float maxWidth) {
    LayoutResult result;
    result.success = true;

    if (shapedGlyphs.empty()) {
        return result;
    }

    size_t startIndex = 0;
    uint32_t charIndex = 0;

    while (startIndex < shapedGlyphs.size()) {
        size_t breakIndex = FindWordBreak(shapedGlyphs, startIndex, maxWidth);

        if (breakIndex == startIndex) {
            // Single character exceeds max width, force break
            breakIndex = startIndex + 1;
        }

        TextLine line;
        line.glyphs.assign(shapedGlyphs.begin() + startIndex, shapedGlyphs.begin() + breakIndex);
        line.startCharIndex = charIndex;
        line.endCharIndex = charIndex + static_cast<uint32_t>(breakIndex - startIndex);

        // Calculate line dimensions
        float minX = FLT_MAX;
        float maxX = FLT_MIN;
        float minY = FLT_MAX;
        float maxY = FLT_MIN;

        for (const auto& glyph : line.glyphs) {
            minX = std::min(minX, glyph.x);
            maxX = std::max(maxX, glyph.x + glyph.advanceX);
            minY = std::min(minY, glyph.y);
            maxY = std::max(maxY, glyph.y + glyph.advanceY);
        }

        line.width = maxX - minX;
        line.height = CalculateLineHeight(line.glyphs);
        line.startX = minX;
        line.startY = minY;
        line.baseline = 0.0f;

        result.lines.push_back(line);
        result.totalWidth = std::max(result.totalWidth, line.width);

        startIndex = breakIndex;
        charIndex = line.endCharIndex;
    }

    return result;
}

LayoutResult TextLayout::LayoutCharWrap(const std::vector<ShapedGlyph>& shapedGlyphs, float maxWidth) {
    LayoutResult result;
    result.success = true;

    if (shapedGlyphs.empty()) {
        return result;
    }

    size_t startIndex = 0;
    uint32_t charIndex = 0;

    while (startIndex < shapedGlyphs.size()) {
        size_t breakIndex = FindCharBreak(shapedGlyphs, startIndex, maxWidth);

        if (breakIndex == startIndex) {
            breakIndex = startIndex + 1;
        }

        TextLine line;
        line.glyphs.assign(shapedGlyphs.begin() + startIndex, shapedGlyphs.begin() + breakIndex);
        line.startCharIndex = charIndex;
        line.endCharIndex = charIndex + static_cast<uint32_t>(breakIndex - startIndex);

        float minX = FLT_MAX;
        float maxX = FLT_MIN;
        float minY = FLT_MAX;
        float maxY = FLT_MIN;

        for (const auto& glyph : line.glyphs) {
            minX = std::min(minX, glyph.x);
            maxX = std::max(maxX, glyph.x + glyph.advanceX);
            minY = std::min(minY, glyph.y);
            maxY = std::max(maxY, glyph.y + glyph.advanceY);
        }

        line.width = maxX - minX;
        line.height = CalculateLineHeight(line.glyphs);
        line.startX = minX;
        line.startY = minY;
        line.baseline = 0.0f;

        result.lines.push_back(line);
        result.totalWidth = std::max(result.totalWidth, line.width);

        startIndex = breakIndex;
        charIndex = line.endCharIndex;
    }

    return result;
}

void TextLayout::ApplyAlignment(std::vector<TextLine>& lines, float maxWidth) {
    for (auto& line : lines) {
        switch (m_Config.alignment) {
            case TextAlignment::Left:
                line.startX = 0.0f;
                break;
            case TextAlignment::Center:
                line.startX = (maxWidth - line.width) * 0.5f;
                break;
            case TextAlignment::Right:
                line.startX = maxWidth - line.width;
                break;
            case TextAlignment::Justify:
                // Simple justification - distribute space evenly
                if (line.glyphs.size() > 1) {
                    float extraSpace = maxWidth - line.width;
                    float spacePerGlyph = extraSpace / static_cast<float>(line.glyphs.size() - 1);
                    for (size_t i = 0; i < line.glyphs.size(); ++i) {
                        line.glyphs[i].x += spacePerGlyph * static_cast<float>(i);
                    }
                    line.width = maxWidth;
                }
                line.startX = 0.0f;
                break;
        }
    }
}

void TextLayout::ApplyLineSpacing(std::vector<TextLine>& lines) {
    if (lines.empty()) {
        return;
    }

    float currentY = 0.0f;
    for (size_t i = 0; i < lines.size(); ++i) {
        lines[i].startY = currentY;
        currentY += lines[i].height + m_Config.lineSpacing;
    }
}

LayoutResult TextLayout::TruncateLines(std::vector<TextLine>& lines, float maxWidth, float maxHeight) {
    LayoutResult result;
    result.success = true;
    result.lines = lines;

    float currentHeight = 0.0f;
    size_t linesToKeep = lines.size();

    for (size_t i = 0; i < lines.size(); ++i) {
        if (currentHeight + lines[i].height > maxHeight) {
            linesToKeep = i;
            break;
        }
        currentHeight += lines[i].height;
    }

    if (linesToKeep < lines.size()) {
        // Truncate last visible line
        if (linesToKeep > 0) {
            if (m_Config.overflow == TextOverflow::Ellipsis) {
                AddEllipsis(lines[linesToKeep - 1], maxWidth);
            }
            result.truncatedChars = static_cast<uint32_t>(lines.size() - linesToKeep);
        }
        lines.resize(linesToKeep);
    }

    return result;
}

void TextLayout::AddEllipsis(TextLine& line, float maxWidth) {
    // Simplified ellipsis - just mark as truncated
    // In a real implementation, you'd add actual ellipsis glyphs
    line.endsWithHyphen = true;
}

float TextLayout::CalculateLineHeight(const std::vector<ShapedGlyph>& glyphs) const {
    if (glyphs.empty()) {
        return 0.0f;
    }

    float minY = FLT_MAX;
    float maxY = FLT_MIN;

    for (const auto& glyph : glyphs) {
        minY = std::min(minY, glyph.y);
        maxY = std::max(maxY, glyph.y + glyph.advanceY);
    }

    float height = maxY - minY;
    return height * m_Config.lineHeight;
}

size_t TextLayout::FindWordBreak(const std::vector<ShapedGlyph>& glyphs, size_t startIndex, float maxWidth) {
    float currentWidth = 0.0f;
    size_t lastWordBreak = startIndex;

    for (size_t i = startIndex; i < glyphs.size(); ++i) {
        if (TextLayoutUtils::IsWordBoundary(glyphs[i])) {
            lastWordBreak = i;
        }

        currentWidth += glyphs[i].advanceX;

        if (currentWidth > maxWidth) {
            return lastWordBreak > startIndex ? lastWordBreak : i;
        }
    }

    return glyphs.size();
}

size_t TextLayout::FindCharBreak(const std::vector<ShapedGlyph>& glyphs, size_t startIndex, float maxWidth) {
    float currentWidth = 0.0f;

    for (size_t i = startIndex; i < glyphs.size(); ++i) {
        currentWidth += glyphs[i].advanceX;

        if (currentWidth > maxWidth) {
            return i;
        }
    }

    return glyphs.size();
}

bool TextLayout::UpdateConfig(const TextLayoutConfig& config) {
    m_Config = config;
    return true;
}

void TextLayout::Measure(const std::vector<ShapedGlyph>& shapedGlyphs,
                         float& outWidth, float& outHeight) {
    LayoutResult result = Layout(shapedGlyphs);
    outWidth = result.totalWidth;
    outHeight = result.totalHeight;
}

uint32_t TextLayout::GetLineCount(const std::vector<ShapedGlyph>& shapedGlyphs) {
    LayoutResult result = Layout(shapedGlyphs);
    return static_cast<uint32_t>(result.lines.size());
}

// ============================================================================
// TextLayoutUtils Implementation
// ============================================================================

namespace TextLayoutUtils {

TextLayoutConfig GetDefaultConfig() {
    TextLayoutConfig config;
    config.maxWidth = 0.0f;
    config.maxHeight = 0.0f;
    config.alignment = TextAlignment::Left;
    config.wrapMode = TextWrapMode::Word;
    config.overflow = TextOverflow::Visible;
    config.lineHeight = 1.2f;
    config.lineSpacing = 0.0f;
    config.preserveWhitespace = true;
    config.enableHyphenation = false;
    return config;
}

TextLayoutConfig GetMultilineConfig() {
    TextLayoutConfig config = GetDefaultConfig();
    config.maxWidth = 800.0f;
    config.maxHeight = 0.0f;
    config.wrapMode = TextWrapMode::Word;
    config.lineHeight = 1.4f;
    config.lineSpacing = 4.0f;
    return config;
}

TextLayoutConfig GetSingleLineConfig() {
    TextLayoutConfig config = GetDefaultConfig();
    config.maxWidth = 0.0f;
    config.maxHeight = 0.0f;
    config.wrapMode = TextWrapMode::None;
    config.lineHeight = 1.0f;
    config.lineSpacing = 0.0f;
    return config;
}

float CalculateOptimalLineHeight(float fontSize) {
    return fontSize * 1.2f;
}

float EstimateHeight(const std::vector<ShapedGlyph>& glyphs, float width, float lineHeight) {
    if (glyphs.empty()) {
        return 0.0f;
    }

    float currentLineWidth = 0.0f;
    int lineCount = 1;

    for (const auto& glyph : glyphs) {
        if (currentLineWidth + glyph.advanceX > width) {
            lineCount++;
            currentLineWidth = glyph.advanceX;
        } else {
            currentLineWidth += glyph.advanceX;
        }
    }

    return static_cast<float>(lineCount) * lineHeight;
}

bool IsWordBoundary(const ShapedGlyph& glyph) {
    // Check if glyph is whitespace (word boundary)
    return glyph.isWhitespace;
}

bool IsLineBreak(const ShapedGlyph& glyph) {
    // Check if glyph represents a line break
    return glyph.codepoint == '\n' || glyph.codepoint == '\r';
}

std::array<float, 4> GetVisualBounds(const LayoutResult& layout) {
    if (layout.lines.empty()) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }

    float minX = FLT_MAX;
    float minY = FLT_MAX;
    float maxX = FLT_MIN;
    float maxY = FLT_MIN;

    for (const auto& line : layout.lines) {
        minX = std::min(minX, line.startX);
        maxX = std::max(maxX, line.startX + line.width);
        minY = std::min(minY, line.startY);
        maxY = std::max(maxY, line.startY + line.height);
    }

    return {minX, minY, maxX, maxY};
}

} // namespace TextLayoutUtils

} // namespace we::UI::Text
