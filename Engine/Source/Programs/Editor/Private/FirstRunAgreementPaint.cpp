#include "Platform/Platform.h"
#include "FirstRunAgreementPopup.h"
#include "FirstRunAgreementInternal.h"
#include "Core/EditorConfigPaths.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/DPIContext.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>

namespace we::programs::editor {
void FirstRunAgreementPopup::LayoutContent(we::runtime::kindui::PaintContext& context) {
    const float contentWidth = m_ContentRect.width;
    
    // Skip layout if width hasn't changed
    if (std::abs(contentWidth - m_CachedLayoutWidth) < 1.0f && !m_BlockYPositions.empty()) {
        return;
    }
    
    m_CachedLayoutWidth = contentWidth;
    m_BlockYPositions.clear();
    m_BlockYPositions.reserve(m_Blocks.size());
    m_BlockHeights.clear();
    m_BlockHeights.reserve(m_Blocks.size());
    
    float y = 0.0f;
    for (const auto& block : m_Blocks) {
        y += block.marginTop;
        m_BlockYPositions.push_back(y);
        
        const float blockHeight = MeasureBlockHeight(context, block, contentWidth);
        m_BlockHeights.push_back(blockHeight);
        
        y += blockHeight;
        y += block.marginBottom;
    }
    
    m_TotalContentHeight = y;
    
    // Clamp scroll position
    const float maxScroll = std::max(0.0f, m_TotalContentHeight - m_ContentRect.height);
    m_TargetScrollOffset = std::clamp(m_TargetScrollOffset, 0.0f, maxScroll);
}
*/

/*
float FirstRunAgreementPopup::MeasureBlockHeight(we::runtime::kindui::PaintContext& context, const MarkdownBlock& block, float maxWidth) {
    const float fontSize = GetFontSizeForBlock(block.type) * m_DpiScale;
    const float lineHeight = GetLineHeightForBlock(block.type);
    
    switch (block.type) {
        case BlockType::CodeBlock: {
            const auto lines = WrapText(context, block.rawText, fontSize, maxWidth - kCodeBlockPadding * 2.0f * m_DpiScale);
            return static_cast<float>(lines.size()) * fontSize * lineHeight + kCodeBlockPadding * 2.0f * m_DpiScale;
        }
        case BlockType::HorizontalRule:
            return 4.0f * m_DpiScale;
        case BlockType::Spacer:
            return 0.0f;
        default: {
            // For text blocks, calculate height from wrapped text (after wrapping)
            std::string fullText;
            for (const auto& span : block.spans) {
                fullText += span.text;
            }
            const auto lines = WrapText(context, fullText, fontSize, maxWidth);
            return static_cast<float>(lines.size()) * fontSize * lineHeight;
        }
    }
}

std::vector<std::string> FirstRunAgreementPopup::WrapText(we::runtime::kindui::PaintContext& context, const std::string& text, float fontSize, float maxWidth) const {
    if (text.empty() || maxWidth <= 0.0f) return {};
    
    std::vector<std::string> lines;
    std::string currentLine;
    
    for (char c : text) {
        if (c == '\n') {
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
                currentLine.clear();
            }
            continue;
        }
        
        std::string testLine = currentLine + c;
        if (context.GetTextWidth(testLine, fontSize) > maxWidth && !currentLine.empty()) {
            lines.push_back(currentLine);
            currentLine = std::string(1, c);
        } else {
            currentLine = testLine;
        }
    }
    
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    return lines;
}
*/

// ============================================================================
// OLD RENDERING (REMOVED - replaced by RenderNode)
// ============================================================================

/*
void FirstRunAgreementPopup::PaintBlock(we::runtime::kindui::PaintContext& context, const MarkdownBlock& block, float y, float viewportTop, float viewportBottom) {
const float contentMargin = kContentMargin * m_DpiScale;
    
    switch (block.type) {
        case BlockType::Heading1:
        case BlockType::Heading2:
        case BlockType::Heading3:
        case BlockType::Paragraph:
            PaintSpans(context, block.spans, m_ContentRect.x + contentMargin, y, 
                GetFontSizeForBlock(block.type) * m_DpiScale, 
                GetColorForBlock(block.type), 
                m_ContentRect.width - contentMargin * 2.0f, 
                viewportTop, viewportBottom);
            break;
            
        case BlockType::CodeBlock:
            PaintCodeBlock(context, block, y, viewportTop, viewportBottom);
            break;
            
        case BlockType::Blockquote:
            PaintBlockquote(context, block, y, viewportTop, viewportBottom);
            break;
            
        case BlockType::UnorderedList:
        case BlockType::OrderedList:
            PaintList(context, block, y, viewportTop, viewportBottom);
            break;
            
        case BlockType::HorizontalRule: {
            const float ruleY = y + 2.0f * m_DpiScale;
            const float ruleWidth = m_ContentRect.width - contentMargin * 2.0f;
            context.DrawRect(we::runtime::kindui::Rect{ m_ContentRect.x + contentMargin, ruleY, ruleWidth, 1.0f * m_DpiScale },
                ThemeColor(ColorToken::BorderDefault));
            break;
        }
            
        case BlockType::Spacer:
            // Nothing to render
            break;
    }
}

void FirstRunAgreementPopup::PaintSpans(we::runtime::kindui::PaintContext& context, const std::vector<TextSpan>& spans, float x, float& y, float fontSize, const we::runtime::kindui::Color& baseColor, float maxWidth, float viewportTop, float viewportBottom) {
const float lineHeight = fontSize * kBaseLineHeight;
    
    // Combine all spans into a single string for wrapping, then render with formatting
    std::string fullText;
    for (const auto& span : spans) {
        fullText += span.text;
    }
    
    const auto lines = WrapText(context, fullText, fontSize, maxWidth);
    
    size_t charIndex = 0;
    for (const auto& line : lines) {
        if (y + lineHeight < viewportTop || y > viewportBottom) {
            y += lineHeight;
            charIndex += line.length();
            continue;
        }
        
        // Render line with inline formatting
        float lineX = x;
        size_t lineStart = charIndex;
        size_t lineEnd = charIndex + line.length();
        
        for (const auto& span : spans) {
            if (span.text.empty()) continue;
            
            we::runtime::kindui::Color color = baseColor;
            bool bold = false;
            
            switch (span.type) {
                case SpanType::Bold:
                    color = ThemeColor(ColorToken::TextPrimary);
                    bold = true;
                    break;
                case SpanType::Italic:
                    color = ThemeColor(ColorToken::TextPrimary);
                    break;
                case SpanType::Code:
                    color = we::runtime::kindui::Color{ 0.9f, 0.7f, 0.4f, 1.0f };
                    break;
                case SpanType::Link:
                    color = we::runtime::kindui::Color{ 0.4f, 0.7f, 1.0f, 1.0f };
                    break;
                default:
                    break;
            }
            
            // Find intersection of this span with current line
            size_t spanStart = charIndex;
            size_t spanEnd = charIndex + span.text.length();
            
            if (spanEnd <= lineStart || spanStart >= lineEnd) {
                charIndex += span.text.length();
                continue;
            }
            
            size_t intersectStart = std::max(lineStart, spanStart);
            size_t intersectEnd = std::min(lineEnd, spanEnd);
            
            if (intersectStart < intersectEnd) {
                std::string segment = span.text.substr(intersectStart - spanStart, intersectEnd - intersectStart);
                context.DrawText(segment, we::runtime::kindui::Point{ lineX, y }, color, fontSize, bold);
                lineX += context.GetTextWidth(segment, fontSize, bold);
            }
            
            charIndex += span.text.length();
        }
        
        y += lineHeight;
        charIndex = lineEnd;
    }
}

void FirstRunAgreementPopup::PaintCodeBlock(we::runtime::kindui::PaintContext& context, const MarkdownBlock& block, float y, float viewportTop, float viewportBottom) {
const float padding = kCodeBlockPadding * m_DpiScale;
    const float fontSize = kCodeFontSize * m_DpiScale;
    const float lineHeight = fontSize * kCodeLineHeight;
    const float contentMargin = kContentMargin * m_DpiScale;
    
    const we::runtime::kindui::Rect bgRect{
        m_ContentRect.x + contentMargin,
        y,
        m_ContentRect.width - contentMargin * 2.0f,
        MeasureBlockHeight(context, block, m_ContentRect.width - contentMargin * 2.0f)
    };
    
    // Code block background
    context.DrawRoundedRect(bgRect, we::runtime::kindui::Color{ 0.15f, 0.15f, 0.18f, 1.0f }, 4.0f);
    
    // Code text
    const auto lines = WrapText(context, block.rawText, fontSize, bgRect.width - padding * 2.0f);
    float textY = y + padding;
    
    for (const auto& line : lines) {
        if (textY + lineHeight < viewportTop || textY > viewportBottom) {
            textY += lineHeight;
            continue;
        }
        
        context.DrawText(line, we::runtime::kindui::Point{ bgRect.x + padding, textY },
            we::runtime::kindui::Color{ 0.9f, 0.9f, 0.85f, 1.0f }, fontSize, false);
        textY += lineHeight;
    }
}

void FirstRunAgreementPopup::PaintBlockquote(we::runtime::kindui::PaintContext& context, const MarkdownBlock& block, float y, float viewportTop, float viewportBottom) {
const float padding = kBlockquotePadding * m_DpiScale;
    const float fontSize = kBlockquoteFontSize * m_DpiScale;
    const float contentMargin = kContentMargin * m_DpiScale;
    
    // Left border
    const float borderX = m_ContentRect.x + contentMargin;
    context.DrawRect(we::runtime::kindui::Rect{ borderX, y, 3.0f * m_DpiScale, MeasureBlockHeight(context, block, m_ContentRect.width - contentMargin * 2.0f) },
        ThemeColor(ColorToken::AccentPrimary));
    
    // Quote text
    float textY = y;
    PaintSpans(context, block.spans, borderX + padding + 3.0f * m_DpiScale, textY,
        fontSize, ThemeColor(ColorToken::TextSecondary),
        m_ContentRect.width - contentMargin * 2.0f - padding - 6.0f * m_DpiScale,
        viewportTop, viewportBottom);
}

void FirstRunAgreementPopup::PaintList(we::runtime::kindui::PaintContext& context, const MarkdownBlock& block, float y, float viewportTop, float viewportBottom) {
const float fontSize = kBaseFontSize * m_DpiScale;
    const float lineHeight = fontSize * kBaseLineHeight;
    const float indent = 20.0f * m_DpiScale;
    const float contentMargin = kContentMargin * m_DpiScale;
    
    float textY = y;
    const float bulletX = m_ContentRect.x + contentMargin + indent * 0.5f;
    
    // Draw bullet or number
    if (block.type == BlockType::UnorderedList) {
        context.DrawText("ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¢", we::runtime::kindui::Point{ bulletX, textY }, ThemeColor(ColorToken::TextPrimary), fontSize, true);
    } else {
        std::string numStr = std::to_string(block.listNumber) + ".";
        context.DrawText(numStr, we::runtime::kindui::Point{ bulletX, textY }, ThemeColor(ColorToken::TextPrimary), fontSize, true);
    }
    
    // Draw text
    PaintSpans(context, block.spans, bulletX + indent, textY, fontSize, ThemeColor(ColorToken::TextPrimary),
        m_ContentRect.width - contentMargin * 2.0f - indent * 1.5f,
        viewportTop, viewportBottom);
}

void FirstRunAgreementPopup::PaintSelection(we::runtime::kindui::PaintContext& context) {
    // TODO: Implement selection highlighting
    // This would require tracking character positions and drawing selection rectangles
}
*/

// ============================================================================
// OLD SCROLLING (REMOVED - replaced by new scrolling methods)
// ============================================================================

/*
void FirstRunAgreementPopup::ScrollTo(float offset) {
    const float maxScroll = std::max(0.0f, m_TotalContentHeight - m_ContentRect.height);
    m_TargetScrollOffset = std::clamp(offset, 0.0f, maxScroll);
}

void FirstRunAgreementPopup::ScrollBy(float delta) {
    ScrollTo(m_TargetScrollOffset + delta);
}

void FirstRunAgreementPopup::ScrollPageUp() {
    ScrollBy(-m_ContentRect.height * 0.9f);
}

void FirstRunAgreementPopup::ScrollPageDown() {
    ScrollBy(m_ContentRect.height * 0.9f);
}

void FirstRunAgreementPopup::ScrollToTop() {
    ScrollTo(0.0f);
}

void FirstRunAgreementPopup::ScrollToBottom() {
    const float maxScroll = std::max(0.0f, m_TotalContentHeight - m_ContentRect.height);
    ScrollTo(maxScroll);
}

void FirstRunAgreementPopup::UpdateScrollbar() {
    const float scrollbarX = m_ContentRect.x + m_ContentRect.width;
    const float scrollbarWidth = kScrollbarWidth * m_DpiScale;
    
    m_Scrollbar.track = we::runtime::kindui::Rect{
        scrollbarX,
        m_ContentRect.y,
        scrollbarWidth,
        m_ContentRect.height
    };
    
    if (m_TotalContentHeight > m_ContentRect.height + 1.0f) {
        const float scrollRatio = m_ContentRect.height / m_TotalContentHeight;
        const float thumbHeight = std::max(kScrollbarMinThumbSize * m_DpiScale, m_ContentRect.height * scrollRatio);
        const float scrollRange = std::max(1.0f, m_TotalContentHeight - m_ContentRect.height);
        const float thumbY = m_Scrollbar.track.y + (m_Scrollbar.track.height - thumbHeight) * (m_ScrollOffset / scrollRange);
        
        m_Scrollbar.thumb = we::runtime::kindui::Rect{
            scrollbarX + 2.0f * m_DpiScale,
            thumbY,
            scrollbarWidth - 4.0f * m_DpiScale,
            thumbHeight
        };
    } else {
        m_Scrollbar.thumb = {};
    }
}

bool FirstRunAgreementPopup::IsPointInScrollbar(const we::runtime::kindui::Point& point) const {
    return m_Scrollbar.track.Contains(point);
}

bool FirstRunAgreementPopup::IsPointInThumb(const we::runtime::kindui::Point& point) const {
    return m_Scrollbar.thumb.Contains(point);
}
*/



// ============================================================================
// PUBLIC STATIC
// ============================================================================

std::string FirstRunAgreementPopup::BuildDefaultContent() {
    return
        "# WindEffects Editor Terms\n"
        "\n"
        "By using this editor, you agree to the license and third-party notices.\n"
        "\n"
        "## Summary\n"
        "- Use this software according to the project license.\n"
        "- Review third-party notices and obligations.\n"
        "- If you do not agree, choose Quit.\n";
}

// ============================================================================
// NEW LAYOUT ENGINE
// ============================================================================

void FirstRunAgreementPopup::ComputeLayout(we::runtime::kindui::PaintContext& context) {
    const float contentWidth = m_ContentRect.width;
    
    if (std::abs(contentWidth - m_LayoutWidth) < 1.0f && m_LayoutValid) {
        return;
    }
    
    m_LayoutWidth = contentWidth;
    m_LayoutValid = true;
    m_NodeYPositions.clear();
    m_NodeYPositions.reserve(m_DocumentNodes.size());
    
    float y = 0.0f;
    for (auto& node : m_DocumentNodes) {
        y += node.marginTop;
        m_NodeYPositions.push_back(y);
        
        const float nodeHeight = MeasureNode(context, node, contentWidth);
        node.cachedHeight = nodeHeight;
        node.cachedWidth = contentWidth;
        
        y += nodeHeight;
        y += node.marginBottom;
    }
    
    m_TotalDocumentHeight = y;
    
    const float maxScroll = std::max(0.0f, m_TotalDocumentHeight - m_ContentRect.height);
    m_ScrollOffset = std::clamp(m_ScrollOffset, 0.0f, maxScroll);
}

float FirstRunAgreementPopup::MeasureNode(we::runtime::kindui::PaintContext& context, DocumentNode& node, float maxWidth) {
    const float fontSize = GetFontSize(node.type) * m_DpiScale;
    const float lineHeight = GetLineHeight(node.type);
    
    switch (node.type) {
        case NodeType::CodeBlock: {
            node.wrappedLines = WrapWords(context, node.rawText, fontSize, maxWidth - kCodeBlockPadding * 2.0f * m_DpiScale);
            return static_cast<float>(node.wrappedLines.size()) * fontSize * lineHeight + kCodeBlockPadding * 2.0f * m_DpiScale;
        }
        case NodeType::HorizontalRule:
            return 4.0f * m_DpiScale;
        case NodeType::Spacer:
            return 0.0f;
        default: {
            std::string fullText;
            for (const auto& run : node.runs) {
                fullText += run.text;
            }
            node.wrappedLines = WrapWords(context, fullText, fontSize, maxWidth);
            return static_cast<float>(node.wrappedLines.size()) * fontSize * lineHeight;
        }
    }
}

std::vector<std::string> FirstRunAgreementPopup::WrapWords(we::runtime::kindui::PaintContext& context, const std::string& text, float fontSize, float maxWidth) {
    if (text.empty() || maxWidth <= 0.0f) return {};
    
    std::vector<std::string> lines;
    std::string currentLine;
    
    for (char c : text) {
        if (c == '\n') {
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
                currentLine.clear();
            }
            continue;
        }
        
        std::string testLine = currentLine + c;
        if (context.GetTextWidth(testLine, fontSize) > maxWidth && !currentLine.empty()) {
            lines.push_back(currentLine);
            currentLine = std::string(1, c);
        } else {
            currentLine = testLine;
        }
    }
    
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    return lines;
}

// ============================================================================
// NEW RENDERING
// ============================================================================

void FirstRunAgreementPopup::RenderNode(we::runtime::kindui::PaintContext& context, const DocumentNode& node, float y) {
const float margin = kContentMargin * m_DpiScale;
    
    switch (node.type) {
        case NodeType::Heading1:
        case NodeType::Heading2:
        case NodeType::Heading3:
        case NodeType::Paragraph:
            RenderTextRuns(context, node.runs, m_ContentRect.x + margin, y, 
                GetFontSize(node.type) * m_DpiScale, 
                GetTextColor(node.type), 
                m_ContentRect.width - margin * 2.0f);
            break;
        case NodeType::CodeBlock:
            RenderCodeBlock(context, node, y);
            break;
        case NodeType::Blockquote:
            RenderBlockquote(context, node, y);
            break;
        case NodeType::UnorderedList:
        case NodeType::OrderedList:
            RenderList(context, node, y);
            break;
        case NodeType::HorizontalRule: {
            const float ruleY = y + 2.0f * m_DpiScale;
            const float ruleWidth = m_ContentRect.width - margin * 2.0f;
            context.DrawRect(we::runtime::kindui::Rect{ m_ContentRect.x + margin, ruleY, ruleWidth, 1.0f * m_DpiScale },
                ThemeColor(ColorToken::BorderDefault));
            break;
        }
        case NodeType::Spacer:
            break;
    }
}

void FirstRunAgreementPopup::RenderTextRuns(we::runtime::kindui::PaintContext& context, const std::vector<TextRun>& runs, float x, float& y, float fontSize, const we::runtime::kindui::Color& baseColor, float maxWidth) {
const float lineHeight = fontSize * kBaseLineHeight;
    
    // Combine all runs for wrapping
    std::string fullText;
    for (const auto& run : runs) {
        fullText += run.text;
    }
    
    const auto lines = WrapWords(context, fullText, fontSize, maxWidth);
    
    size_t charIndex = 0;
    for (const auto& line : lines) {
        float lineX = x;
        size_t lineStart = charIndex;
        size_t lineEnd = charIndex + line.length();
        
        for (const auto& run : runs) {
            if (run.text.empty()) continue;
            
            we::runtime::kindui::Color color = baseColor;
            bool bold = false;
            
            switch (run.style) {
                case TextStyle::Bold:
                    color = ThemeColor(ColorToken::TextPrimary);
                    bold = true;
                    break;
                case TextStyle::Italic:
                    color = ThemeColor(ColorToken::TextPrimary);
                    break;
                case TextStyle::Code:
                    color = we::runtime::kindui::Color{ 0.9f, 0.7f, 0.4f, 1.0f };
                    break;
                case TextStyle::Link:
                    color = we::runtime::kindui::Color{ 0.4f, 0.7f, 1.0f, 1.0f };
                    break;
                default:
                    break;
            }
            
            size_t runStart = charIndex;
            size_t runEnd = charIndex + run.text.length();
            
            if (runEnd <= lineStart || runStart >= lineEnd) {
                charIndex += run.text.length();
                continue;
            }
            
            size_t intersectStart = std::max(lineStart, runStart);
            size_t intersectEnd = std::min(lineEnd, runEnd);
            
            if (intersectStart < intersectEnd) {
                std::string segment = run.text.substr(intersectStart - runStart, intersectEnd - intersectStart);
                context.DrawText(segment, we::runtime::kindui::Point{ lineX, y }, color, fontSize, bold);
                lineX += context.GetTextWidth(segment, fontSize, bold);
            }
            
            charIndex += run.text.length();
        }
        
        y += lineHeight;
        charIndex = lineEnd;
    }
}

void FirstRunAgreementPopup::RenderCodeBlock(we::runtime::kindui::PaintContext& context, const DocumentNode& node, float y) {
const float padding = kCodeBlockPadding * m_DpiScale;
    const float fontSize = kCodeFontSize * m_DpiScale;
    const float lineHeight = fontSize * kCodeLineHeight;
    const float margin = kContentMargin * m_DpiScale;
    
    const we::runtime::kindui::Rect bgRect{
        m_ContentRect.x + margin,
        y,
        m_ContentRect.width - margin * 2.0f,
        node.cachedHeight
    };
    
    context.DrawRoundedRect(bgRect, we::runtime::kindui::Color{ 0.15f, 0.15f, 0.18f, 1.0f }, 4.0f);
    
    const auto lines = node.wrappedLines;
    float textY = y + padding;
    
    for (const auto& line : lines) {
        context.DrawText(line, we::runtime::kindui::Point{ bgRect.x + padding, textY },
            we::runtime::kindui::Color{ 0.9f, 0.9f, 0.85f, 1.0f }, fontSize, false);
        textY += lineHeight;
    }
}

void FirstRunAgreementPopup::RenderBlockquote(we::runtime::kindui::PaintContext& context, const DocumentNode& node, float y) {
const float padding = kBlockquotePadding * m_DpiScale;
    const float fontSize = kBlockquoteFontSize * m_DpiScale;
    const float margin = kContentMargin * m_DpiScale;
    
    const float borderX = m_ContentRect.x + margin;
    context.DrawRect(we::runtime::kindui::Rect{ borderX, y, 3.0f * m_DpiScale, node.cachedHeight },
        ThemeColor(ColorToken::AccentPrimary));
    
    float textY = y;
    RenderTextRuns(context, node.runs, borderX + padding + 3.0f * m_DpiScale, textY,
        fontSize, ThemeColor(ColorToken::TextSecondary),
        m_ContentRect.width - margin * 2.0f - padding - 6.0f * m_DpiScale);
}

void FirstRunAgreementPopup::RenderList(we::runtime::kindui::PaintContext& context, const DocumentNode& node, float y) {
const float fontSize = kBaseFontSize * m_DpiScale;
    const float lineHeight = fontSize * kBaseLineHeight;
    const float indent = 20.0f * m_DpiScale;
    const float margin = kContentMargin * m_DpiScale;
    
    float textY = y;
    const float bulletX = m_ContentRect.x + margin + indent * 0.5f;
    
    if (node.type == NodeType::UnorderedList) {
        context.DrawText("ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¢", we::runtime::kindui::Point{ bulletX, textY }, ThemeColor(ColorToken::TextPrimary), fontSize, true);
    } else {
        std::string numStr = std::to_string(node.listNumber) + ".";
        context.DrawText(numStr, we::runtime::kindui::Point{ bulletX, textY }, ThemeColor(ColorToken::TextPrimary), fontSize, true);
    }
    
    RenderTextRuns(context, node.runs, bulletX + indent, textY, fontSize, ThemeColor(ColorToken::TextPrimary),
        m_ContentRect.width - margin * 2.0f - indent * 1.5f);
}

// ============================================================================
// NEW SCROLLING
// ============================================================================

void FirstRunAgreementPopup::SetScrollOffset(float offset) {
    const float maxScroll = std::max(0.0f, m_TotalDocumentHeight - m_ContentRect.height);
    m_ScrollOffset = std::clamp(offset, 0.0f, maxScroll);
}

void FirstRunAgreementPopup::ScrollBy(float delta) {
    SetScrollOffset(m_ScrollOffset + delta);
}

void FirstRunAgreementPopup::ScrollPageUp() {
    ScrollBy(-m_ContentRect.height * 0.9f);
}

void FirstRunAgreementPopup::ScrollPageDown() {
    ScrollBy(m_ContentRect.height * 0.9f);
}

void FirstRunAgreementPopup::ScrollToTop() {
    SetScrollOffset(0.0f);
}

void FirstRunAgreementPopup::ScrollToBottom() {
    const float maxScroll = std::max(0.0f, m_TotalDocumentHeight - m_ContentRect.height);
    SetScrollOffset(maxScroll);
}

void FirstRunAgreementPopup::UpdateScrollbarGeometry() {
    const float scrollbarX = m_ContentRect.x + m_ContentRect.width;
    const float scrollbarWidth = kScrollbarWidth * m_DpiScale;
    
    m_Scrollbar.track = we::runtime::kindui::Rect{
        scrollbarX,
        m_ContentRect.y,
        scrollbarWidth,
        m_ContentRect.height
    };
    
    if (m_TotalDocumentHeight > m_ContentRect.height + 1.0f) {
        const float scrollRatio = m_ContentRect.height / m_TotalDocumentHeight;
        const float thumbHeight = std::max(kScrollbarMinThumbSize * m_DpiScale, m_ContentRect.height * scrollRatio);
        const float scrollRange = std::max(1.0f, m_TotalDocumentHeight - m_ContentRect.height);
        const float thumbY = m_Scrollbar.track.y + (m_Scrollbar.track.height - thumbHeight) * (m_ScrollOffset / scrollRange);
        
        m_Scrollbar.thumb = we::runtime::kindui::Rect{
            scrollbarX + 2.0f * m_DpiScale,
            thumbY,
            scrollbarWidth - 4.0f * m_DpiScale,
            thumbHeight
        };
    } else {
        m_Scrollbar.thumb = {};
    }
}

bool FirstRunAgreementPopup::IsOverScrollbar(const we::runtime::kindui::Point& point) const {
    return m_Scrollbar.track.Contains(point);
}

bool FirstRunAgreementPopup::IsOverThumb(const we::runtime::kindui::Point& point) const {
    return m_Scrollbar.thumb.Contains(point);
}

// ============================================================================
// NEW UTILITIES
// ============================================================================

float FirstRunAgreementPopup::GetFontSize(NodeType type) const {
    switch (type) {
        case NodeType::Heading1: return kHeading1FontSize;
        case NodeType::Heading2: return kHeading2FontSize;
        case NodeType::Heading3: return kHeading3FontSize;
        case NodeType::CodeBlock: return kCodeFontSize;
        case NodeType::Blockquote: return kBlockquoteFontSize;
        default: return kBaseFontSize;
    }
}

float FirstRunAgreementPopup::GetLineHeight(NodeType type) const {
    switch (type) {
        case NodeType::CodeBlock: return kCodeLineHeight;
        default: return kBaseLineHeight;
    }
}

we::runtime::kindui::Color FirstRunAgreementPopup::GetTextColor(NodeType type) const {
switch (type) {
        case NodeType::Heading1:
        case NodeType::Heading2:
        case NodeType::Heading3:
            return ThemeColor(ColorToken::TextPrimary);
        case NodeType::CodeBlock:
            return we::runtime::kindui::Color{ 0.9f, 0.9f, 0.85f, 1.0f };
        case NodeType::Blockquote:
            return ThemeColor(ColorToken::TextSecondary);
        default:
            return ThemeColor(ColorToken::TextPrimary);
    }
}

// ============================================================================
// FREE FUNCTIONS
// ============================================================================

bool HasAcceptedFirstRunAgreement() {
    const auto path = GetAgreementStatePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    
    // Auto-generate default file if it doesn't exist
    if (!std::filesystem::exists(path, ec)) {
        std::ofstream file(path);
        if (file.is_open()) {
            file << "# WindEffects First Run Agreement State\n";
            file << "# This file tracks whether the user has accepted the license agreement\n";
            file << "accepted=0\n";
        }
        return false;
    }
    
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        if (line == "accepted=1" || line == "accepted=true") return true;
    }
    return false;
}

void SetAcceptedFirstRunAgreement(bool accepted) {
    const auto path = GetAgreementStatePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) return;
    file << "accepted=" << (accepted ? "1" : "0") << "\n";
}

std::string LoadFirstRunAgreementContent() {
    const std::string eula = LoadFirstRunAgreementEulaContent();
    const std::string thirdParty = LoadFirstRunAgreementThirdPartyNoticesContent();

    if (eula.empty() && thirdParty.empty()) {
        return FirstRunAgreementPopup::BuildDefaultContent();
    }

    if (eula.empty()) return thirdParty;
    if (thirdParty.empty()) return eula;

    return eula + "\n\n" + thirdParty;
}

std::string LoadFirstRunAgreementEulaContent() {
    const auto eulaPath = ResolveRuntimeFile("Legal/EULA.md");
    std::string eula = eulaPath.empty() ? std::string{} : ReadTextFile(eulaPath);
    if (eula.empty()) {
        return
            "# End User License Agreement\n\n"
            "License text was not found. Please review the project documentation.\n";
    }
    return std::string("# End User License Agreement\n\n") + eula;
}

std::string LoadFirstRunAgreementThirdPartyNoticesContent() {
    const auto noticesPath = ResolveRuntimeFile("Legal/THIRD_PARTY_NOTICES.md");
    std::string notices = noticesPath.empty() ? std::string{} : ReadTextFile(noticesPath);
    if (notices.empty()) {
        return
            "# Third-Party Notices\n\n"
            "Third-party notice text was not found. Please review the project documentation.\n";
    }
    return std::string("# Third-Party Notices\n\n") + notices;
}

} // namespace we::programs::editor
