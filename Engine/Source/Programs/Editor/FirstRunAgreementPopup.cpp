#include "Platform/Platform.h"
#include "FirstRunAgreementPopup.h"
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

using we::runtime::kindui::ColorToken;

namespace {

constexpr float kDialogPadding = 16.0f;
constexpr float kButtonHeight = 32.0f;
constexpr float kButtonWidth = 110.0f;
constexpr float kButtonGap = 10.0f;
constexpr float kCopyButtonWidth = 110.0f;
constexpr float kScrollbarWidth = 12.0f;
constexpr float kScrollbarMinThumbSize = 30.0f;
constexpr float kSmoothScrollSpeed = 0.15f;

// Typography constants (scaled by DPI)
constexpr float kBaseFontSize = 13.0f;
constexpr float kHeading1FontSize = 18.0f;
constexpr float kHeading2FontSize = 15.0f;
constexpr float kHeading3FontSize = 13.0f;
constexpr float kCodeFontSize = 12.0f;
constexpr float kBlockquoteFontSize = 13.0f;

constexpr float kBaseLineHeight = 1.45f;
constexpr float kCodeLineHeight = 1.4f;

// Compact spacing similar to GitHub/UE documentation
constexpr float kParagraphSpacing = 8.0f;
constexpr float kHeadingSpacing = 10.0f;
constexpr float kListSpacing = 4.0f;
constexpr float kCodeBlockPadding = 10.0f;
constexpr float kBlockquotePadding = 10.0f;
constexpr float kContentMargin = 16.0f;

std::filesystem::path GetAgreementStatePath() {
    return we::core::ResolveEditorConfigPath("first_run_agreement.ini");
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return {};
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::filesystem::path ResolveRuntimeFile(const std::string& relativePath) {
    const std::filesystem::path p(relativePath);
    if (std::filesystem::exists(p)) return p;
    const std::filesystem::path candidates[] = {
        std::filesystem::path("..") / relativePath,
        std::filesystem::path("../..") / relativePath
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) return candidate;
    }
    return {};
}

bool StartsWith(const std::string& str, const std::string& prefix) {
    return str.rfind(prefix, 0) == 0;
}

std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::string TrimLeft(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    return str.substr(start);
}

} // namespace

FirstRunAgreementPopup::FirstRunAgreementPopup(std::string markdownAndTextContent)
    : m_RawContent(std::move(markdownAndTextContent))
{
    m_DpiScale = we::runtime::kindui::DPIContext::GetScale();
    
    if (m_RawContent.empty()) {
        m_RawContent = BuildDefaultContent();
    }
    m_Title = ExtractTitle(m_RawContent);
    ParseDocument();
    
    m_AgreeButton.label = "I Agree";
    m_AgreeButton.iconName = we::runtime::kindui::Icons::CheckName;
    m_DeclineButton.label = "Quit";
    m_DeclineButton.iconName = we::runtime::kindui::Icons::XName;
    m_CopyButton.label = "Copy";
    m_CopyButton.iconName = we::runtime::kindui::Icons::CopyName;
}

FirstRunAgreementPopup::~FirstRunAgreementPopup() = default;

we::runtime::kindui::Size FirstRunAgreementPopup::Measure(const we::runtime::kindui::Size& availableSize) {
    // Dialog should be a reasonable size but not fill the entire viewport
    const float dialogW = std::clamp(availableSize.width * 0.7f, 600.0f * m_DpiScale, 900.0f * m_DpiScale);
    const float dialogH = std::clamp(availableSize.height * 0.8f, 450.0f * m_DpiScale, 700.0f * m_DpiScale);
    
    m_DesiredSize = we::runtime::kindui::Size{ dialogW, dialogH };
    return m_DesiredSize;
}

void FirstRunAgreementPopup::Arrange(const we::runtime::kindui::Rect& allottedRect) {
    m_Geometry = allottedRect;
    
    // Center dialog in viewport every frame
    const float dialogW = m_DesiredSize.width;
    const float dialogH = m_DesiredSize.height;
    
    m_DialogRect = we::runtime::kindui::Rect{
        allottedRect.x + (allottedRect.width - dialogW) * 0.5f,
        allottedRect.y + (allottedRect.height - dialogH) * 0.5f,
        dialogW,
        dialogH
    };
    
    // Content area
    const float margin = kContentMargin * m_DpiScale;
    const float headerHeight = 50.0f * m_DpiScale;
    const float footerHeight = 60.0f * m_DpiScale;
    
    m_ContentRect = we::runtime::kindui::Rect{
        m_DialogRect.x + margin,
        m_DialogRect.y + headerHeight,
        m_DialogRect.width - margin * 2.0f - kScrollbarWidth * m_DpiScale,
        m_DialogRect.height - headerHeight - footerHeight
    };
    
    // Buttons
    const float buttonY = m_DialogRect.y + m_DialogRect.height - footerHeight + 10.0f * m_DpiScale;
    const float scaledButtonWidth = kButtonWidth * m_DpiScale;
    const float scaledButtonHeight = kButtonHeight * m_DpiScale;
    const float scaledButtonGap = kButtonGap * m_DpiScale;
    
    m_AgreeButton.rect = we::runtime::kindui::Rect{
        m_DialogRect.x + m_DialogRect.width - kDialogPadding * m_DpiScale - scaledButtonWidth,
        buttonY,
        scaledButtonWidth,
        scaledButtonHeight
    };
    m_DeclineButton.rect = we::runtime::kindui::Rect{
        m_AgreeButton.rect.x - scaledButtonGap - scaledButtonWidth,
        buttonY,
        scaledButtonWidth,
        scaledButtonHeight
    };
    m_CopyButton.rect = we::runtime::kindui::Rect{
        m_DialogRect.x + m_DialogRect.width - kDialogPadding * m_DpiScale - kButtonWidth * m_DpiScale,
        m_DialogRect.y + kDialogPadding * m_DpiScale,
        kButtonWidth * m_DpiScale,
        26.0f * m_DpiScale
    };
    
    // Invalidate layout
    m_LayoutValid = false;
}

void FirstRunAgreementPopup::Paint(we::runtime::kindui::PaintContext& context) {
    // Background overlay - cover entire viewport
    context.DrawRect(m_Geometry, we::runtime::kindui::Color{ 0.0f, 0.0f, 0.0f, 0.65f });
    
    // Dialog shadow and background
    context.DrawShadow(m_DialogRect, we::runtime::kindui::Color{ 0.0f, 0.0f, 0.0f, 0.40f }, 16.0f, 24.0f);
    context.DrawRoundedRect(m_DialogRect, ThemeColor(ColorToken::PanelBackground), 8.0f);
    context.DrawRoundedRectOutline(m_DialogRect, ThemeColor(ColorToken::BorderDefault), 1.0f, 8.0f);
    
    // Header
    const float titleFontSize = 17.0f * m_DpiScale;
    const float subtitleFontSize = 11.0f * m_DpiScale;
    context.DrawText(m_Title.empty() ? "License Agreement" : m_Title,
        we::runtime::kindui::Point{ m_DialogRect.x + kDialogPadding * m_DpiScale, m_DialogRect.y + kDialogPadding * m_DpiScale + 6.0f * m_DpiScale },
        ThemeColor(ColorToken::TextPrimary), titleFontSize, true);
    context.DrawText("Please review and accept before using the editor.",
        we::runtime::kindui::Point{ m_DialogRect.x + kDialogPadding * m_DpiScale, m_DialogRect.y + kDialogPadding * m_DpiScale + 28.0f * m_DpiScale },
        ThemeColor(ColorToken::TextSecondary), subtitleFontSize);
    
    // Content area background
    context.DrawRoundedRect(m_ContentRect, ThemeColor(ColorToken::WorkspaceBackground), 6.0f);
    context.DrawRoundedRectOutline(m_ContentRect, ThemeColor(ColorToken::BorderDefault), 1.0f, 6.0f);
    context.PushClipRect(m_ContentRect);
    
    // Compute layout if invalid
    ComputeLayout(context);
    
    // Render visible nodes (no culling - render all visible content every frame)
    const float viewportTop = m_ScrollOffset;
    const float viewportBottom = m_ScrollOffset + m_ContentRect.height;
    
    for (size_t i = 0; i < m_DocumentNodes.size(); ++i) {
        const float nodeY = m_NodeYPositions[i];
        const float nodeBottom = nodeY + m_DocumentNodes[i].cachedHeight;
        
        // Skip nodes completely outside viewport (but don't cull too aggressively)
        if (nodeBottom < viewportTop - 50.0f || nodeY > viewportBottom + 50.0f) {
            continue;
        }
        
        RenderNode(context, m_DocumentNodes[i], nodeY - m_ScrollOffset + m_ContentRect.y);
    }
    
    context.PopClipRect();
    
    // Scrollbar
    UpdateScrollbarGeometry();
    if (m_TotalDocumentHeight > m_ContentRect.height + 1.0f) {
        const float trackAlpha = m_Scrollbar.hovering ? 0.15f : 0.08f;
        const float thumbAlpha = (m_Scrollbar.hovering || m_Scrollbar.dragging) ? 0.50f : 0.30f;
        
        context.DrawRoundedRect(m_Scrollbar.track, we::runtime::kindui::Color{ 1.0f, 1.0f, 1.0f, trackAlpha }, 4.0f);
        context.DrawRoundedRect(m_Scrollbar.thumb, we::runtime::kindui::Color{ 1.0f, 1.0f, 1.0f, thumbAlpha }, 4.0f);
    }
    
    // Footer hint text
    const float hintFontSize = 10.0f * m_DpiScale;
    context.DrawText("Press Ctrl+C to copy the full agreement text.",
        we::runtime::kindui::Point{ m_DialogRect.x + kDialogPadding * m_DpiScale, m_DialogRect.y + m_DialogRect.height - kDialogPadding * m_DpiScale - 14.0f * m_DpiScale },
        ThemeColor(ColorToken::TextSecondary), hintFontSize);
    
    // Buttons
    auto paintButton = [&](const ButtonState& button, bool primary) {
        we::runtime::kindui::Color bg = primary ? ThemeColor(ColorToken::AccentPrimary) : ThemeColor(ColorToken::HoverBackground);
        if (!button.hovered) bg.a *= 0.85f;
        if (button.pressed) bg = we::runtime::kindui::Color::Lerp(bg, ThemeColor(ColorToken::PressedBackground), 0.45f);
        context.DrawRoundedRect(button.rect, bg, 4.0f);
        context.DrawRoundedRectOutline(button.rect, ThemeColor(ColorToken::BorderDefault), 1.0f, 4.0f);
        
        const float iconSize = 13.0f * m_DpiScale;
        const float textGap = button.iconName.empty() ? 0.0f : 6.0f * m_DpiScale;
        const float fontSize = 12.0f * m_DpiScale;
        const float tw = context.GetTextWidth(button.label, fontSize);
        const float contentW = tw + (button.iconName.empty() ? 0.0f : iconSize + textGap);
        float contentX = button.rect.x + (button.rect.width - contentW) * 0.5f;
        const we::runtime::kindui::Color textColor = primary ? we::runtime::kindui::Color{ 0.10f, 0.10f, 0.10f, 1.0f } : ThemeColor(ColorToken::TextPrimary);
        
        if (!button.iconName.empty()) {
            we::runtime::kindui::IconPainter::DrawIcon(context, button.iconName,
                we::runtime::kindui::Rect{ contentX, button.rect.y + (button.rect.height - iconSize) * 0.5f, iconSize, iconSize }, textColor);
            contentX += iconSize + textGap;
        }
        context.DrawText(button.label,
            we::runtime::kindui::Point{ contentX, button.rect.y + (button.rect.height - fontSize) * 0.5f + 1.5f * m_DpiScale },
            textColor, fontSize, true);
    };
    
    paintButton(m_CopyButton, false);
    paintButton(m_DeclineButton, false);
    paintButton(m_AgreeButton, true);
}

void FirstRunAgreementPopup::OnMouseDown(const we::runtime::kindui::MouseEvent& event) {
    if (event.button != we::runtime::kindui::MouseButton::Left) return;
    
    m_AgreeButton.pressed = m_AgreeButton.rect.Contains(event.position);
    m_DeclineButton.pressed = m_DeclineButton.rect.Contains(event.position);
    m_CopyButton.pressed = m_CopyButton.rect.Contains(event.position);
    
    if (IsOverScrollbar(event.position)) {
        if (IsOverThumb(event.position)) {
            m_Scrollbar.dragging = true;
            m_Scrollbar.dragStartY = event.position.y;
            m_Scrollbar.dragStartScroll = m_ScrollOffset;
        } else {
            const float scrollRatio = (event.position.y - m_Scrollbar.track.y) / m_Scrollbar.track.height;
            const float maxScroll = std::max(0.0f, m_TotalDocumentHeight - m_ContentRect.height);
            SetScrollOffset(scrollRatio * maxScroll);
            m_Scrollbar.dragging = true;
            m_Scrollbar.dragStartY = event.position.y;
            m_Scrollbar.dragStartScroll = m_ScrollOffset;
        }
    }
}

void FirstRunAgreementPopup::OnMouseMove(const we::runtime::kindui::MouseEvent& event) {
    m_AgreeButton.hovered = m_AgreeButton.rect.Contains(event.position);
    m_DeclineButton.hovered = m_DeclineButton.rect.Contains(event.position);
    m_CopyButton.hovered = m_CopyButton.rect.Contains(event.position);
    m_Scrollbar.hovering = IsOverScrollbar(event.position);
    
    if (m_Scrollbar.dragging) {
        const float deltaY = event.position.y - m_Scrollbar.dragStartY;
        const float scrollRange = std::max(1.0f, m_TotalDocumentHeight - m_ContentRect.height);
        const float thumbRange = m_Scrollbar.track.height - m_Scrollbar.thumb.height;
        const float scrollDelta = (deltaY / thumbRange) * scrollRange;
        SetScrollOffset(m_Scrollbar.dragStartScroll + scrollDelta);
    }
}

void FirstRunAgreementPopup::OnMouseUp(const we::runtime::kindui::MouseEvent& event) {
    if (event.button != we::runtime::kindui::MouseButton::Left) return;
    
    const bool acceptClicked = m_AgreeButton.pressed && m_AgreeButton.rect.Contains(event.position);
    const bool declineClicked = m_DeclineButton.pressed && m_DeclineButton.rect.Contains(event.position);
    const bool copyClicked = m_CopyButton.pressed && m_CopyButton.rect.Contains(event.position);
    
    m_AgreeButton.pressed = false;
    m_DeclineButton.pressed = false;
    m_CopyButton.pressed = false;
    m_Scrollbar.dragging = false;
    
    if (copyClicked) {
        we::platform::Platform::Get().SetClipboardText(m_RawContent.c_str());
    }
    
    // Defer callback execution to avoid use-after-free during event processing
    if (acceptClicked && m_OnAccepted) {
        m_PendingCallback = m_OnAccepted;
    }
    if (declineClicked && m_OnDeclined) {
        m_PendingCallback = m_OnDeclined;
    }
}

void FirstRunAgreementPopup::ExecutePendingCallback() {
    if (m_PendingCallback) {
        try {
            auto callback = std::move(m_PendingCallback);
            m_PendingCallback = nullptr;
            callback();
        } catch (const std::exception& e) {
            HE_ERROR("Error executing pending callback: " + std::string(e.what()));
        } catch (...) {
            HE_ERROR("Unknown error executing pending callback");
        }
    }
}

void FirstRunAgreementPopup::OnMouseWheel(const we::runtime::kindui::MouseEvent& event) {
    if (!m_DialogRect.Contains(event.position)) return;
    
    if (m_ContentRect.Contains(event.position) || IsOverScrollbar(event.position)) {
        const float scrollAmount = event.wheelDeltaY * 40.0f * m_DpiScale;
        ScrollBy(-scrollAmount);
    }
}

void FirstRunAgreementPopup::OnKeyDown(const we::runtime::kindui::KeyEvent& event) {
    if (event.ctrlDown && event.key == we::platform::KeyCode::C) {
        we::platform::Platform::Get().SetClipboardText(m_RawContent.c_str());
        return;
    }
    
    if (event.key == we::platform::KeyCode::Escape) {
        if (m_OnDeclined) m_OnDeclined();
        return;
    }
    
    const float lineHeight = GetFontSize(NodeType::Paragraph) * GetLineHeight(NodeType::Paragraph);
    
    switch (event.key) {
        case we::platform::KeyCode::PageUp:
            ScrollPageUp();
            break;
        case we::platform::KeyCode::PageDown:
            ScrollPageDown();
            break;
        case we::platform::KeyCode::Home:
            ScrollToTop();
            break;
        case we::platform::KeyCode::End:
            ScrollToBottom();
            break;
        case we::platform::KeyCode::Up:
            ScrollBy(-lineHeight * 3.0f);
            break;
        case we::platform::KeyCode::Down:
            ScrollBy(lineHeight * 3.0f);
            break;
        case we::platform::KeyCode::Space:
            if (event.shiftDown) ScrollPageUp();
            else ScrollPageDown();
            break;
        default:
            break;
    }
}

bool FirstRunAgreementPopup::ShowsPointerCursor(const we::runtime::kindui::Point& position) const {
    return m_AgreeButton.rect.Contains(position) || 
           m_DeclineButton.rect.Contains(position) || 
           m_CopyButton.rect.Contains(position) ||
           IsOverThumb(position);
}

// ============================================================================
// DOCUMENT PARSING
// ============================================================================

void FirstRunAgreementPopup::ParseDocument() {
    m_DocumentNodes.clear();
    
    std::istringstream input(m_RawContent);
    std::string line;
    bool inCodeBlock = false;
    std::string codeBlockContent;
    
    while (std::getline(input, line)) {
        // Code blocks
        if (StartsWith(line, "```")) {
            if (inCodeBlock) {
                DocumentNode node;
                node.type = NodeType::CodeBlock;
                node.rawText = codeBlockContent;
                node.marginTop = kParagraphSpacing * m_DpiScale;
                node.marginBottom = kParagraphSpacing * m_DpiScale;
                m_DocumentNodes.push_back(node);
                codeBlockContent.clear();
            }
            inCodeBlock = !inCodeBlock;
            continue;
        }
        
        if (inCodeBlock) {
            codeBlockContent += line + "\n";
            continue;
        }
        
        const std::string trimmed = Trim(line);
        
        // Horizontal rule
        if (trimmed == "---" || trimmed == "***" || StartsWith(trimmed, "---") || StartsWith(trimmed, "***")) {
            DocumentNode node;
            node.type = NodeType::HorizontalRule;
            node.marginTop = kParagraphSpacing * m_DpiScale;
            node.marginBottom = kParagraphSpacing * m_DpiScale;
            m_DocumentNodes.push_back(node);
            continue;
        }
        
        // Blockquotes
        if (StartsWith(trimmed, ">")) {
            DocumentNode node;
            node.type = NodeType::Blockquote;
            node.runs = ParseInlineText(TrimLeft(trimmed.substr(1)));
            node.marginTop = kParagraphSpacing * m_DpiScale;
            node.marginBottom = kParagraphSpacing * m_DpiScale;
            m_DocumentNodes.push_back(node);
            continue;
        }
        
        // Empty lines
        if (trimmed.empty()) {
            DocumentNode node;
            node.type = NodeType::Spacer;
            node.marginTop = kParagraphSpacing * m_DpiScale * 0.5f;
            node.marginBottom = kParagraphSpacing * m_DpiScale * 0.5f;
            m_DocumentNodes.push_back(node);
            continue;
        }
        
        // Headings
        if (StartsWith(trimmed, "# ")) {
            DocumentNode node;
            node.type = NodeType::Heading1;
            node.runs = ParseInlineText(trimmed.substr(2));
            node.marginTop = kHeadingSpacing * m_DpiScale;
            node.marginBottom = kParagraphSpacing * m_DpiScale * 0.5f;
            m_DocumentNodes.push_back(node);
            continue;
        }
        if (StartsWith(trimmed, "## ")) {
            DocumentNode node;
            node.type = NodeType::Heading2;
            node.runs = ParseInlineText(trimmed.substr(3));
            node.marginTop = kHeadingSpacing * m_DpiScale * 0.8f;
            node.marginBottom = kParagraphSpacing * m_DpiScale * 0.4f;
            m_DocumentNodes.push_back(node);
            continue;
        }
        if (StartsWith(trimmed, "### ")) {
            DocumentNode node;
            node.type = NodeType::Heading3;
            node.runs = ParseInlineText(trimmed.substr(4));
            node.marginTop = kHeadingSpacing * m_DpiScale * 0.6f;
            node.marginBottom = kParagraphSpacing * m_DpiScale * 0.3f;
            m_DocumentNodes.push_back(node);
            continue;
        }
        
        // Ordered lists
        static const std::regex orderedListRegex(R"(^(\d+)[.)]\s+(.*)$)");
        std::smatch orderedMatch;
        if (std::regex_match(trimmed, orderedMatch, orderedListRegex)) {
            DocumentNode node;
            node.type = NodeType::OrderedList;
            node.listNumber = std::stoi(orderedMatch[1].str());
            node.runs = ParseInlineText(orderedMatch[2].str());
            node.marginTop = kListSpacing * m_DpiScale;
            node.marginBottom = kListSpacing * m_DpiScale * 0.25f;
            m_DocumentNodes.push_back(node);
            continue;
        }
        
        // Unordered lists
        if (StartsWith(trimmed, "- ") || StartsWith(trimmed, "* ") || StartsWith(trimmed, "+ ")) {
            DocumentNode node;
            node.type = NodeType::UnorderedList;
            node.runs = ParseInlineText(trimmed.substr(2));
            node.marginTop = kListSpacing * m_DpiScale;
            node.marginBottom = kListSpacing * m_DpiScale * 0.25f;
            m_DocumentNodes.push_back(node);
            continue;
        }
        
        // Paragraph
        DocumentNode node;
        node.type = NodeType::Paragraph;
        node.runs = ParseInlineText(trimmed);
        node.marginTop = kParagraphSpacing * m_DpiScale;
        node.marginBottom = kParagraphSpacing * m_DpiScale;
        m_DocumentNodes.push_back(node);
    }
}

std::vector<FirstRunAgreementPopup::TextRun> FirstRunAgreementPopup::ParseInlineText(const std::string& text) {
    std::vector<TextRun> runs;
    std::string current;
    bool inBold = false;
    bool inItalic = false;
    bool inCode = false;
    
    for (size_t i = 0; i < text.length(); ++i) {
        // Bold: **text**
        if (i + 1 < text.length() && text[i] == '*' && text[i + 1] == '*') {
            if (!current.empty()) {
                TextRun run;
                run.text = current;
                run.style = inCode ? TextStyle::Code : (inBold ? TextStyle::Bold : (inItalic ? TextStyle::Italic : TextStyle::Normal));
                runs.push_back(run);
                current.clear();
            }
            inBold = !inBold;
            i += 1;
            continue;
        }
        
        // Italic: *text* or _text_
        if ((text[i] == '*' || text[i] == '_') && !inBold) {
            if (!current.empty()) {
                TextRun run;
                run.text = current;
                run.style = inCode ? TextStyle::Code : (inItalic ? TextStyle::Italic : TextStyle::Normal);
                runs.push_back(run);
                current.clear();
            }
            inItalic = !inItalic;
            continue;
        }
        
        // Code: `text`
        if (text[i] == '`') {
            if (!current.empty()) {
                TextRun run;
                run.text = current;
                run.style = inCode ? TextStyle::Code : (inBold ? TextStyle::Bold : (inItalic ? TextStyle::Italic : TextStyle::Normal));
                runs.push_back(run);
                current.clear();
            }
            inCode = !inCode;
            continue;
        }
        
        // Links: [text](url)
        if (text[i] == '[' && !inCode) {
            size_t linkEnd = text.find(']', i);
            size_t urlStart = text.find('(', linkEnd);
            size_t urlEnd = text.find(')', urlStart);
            
            if (linkEnd != std::string::npos && urlStart != std::string::npos && urlEnd != std::string::npos) {
                if (!current.empty()) {
                    TextRun run;
                    run.text = current;
                    run.style = TextStyle::Normal;
                    runs.push_back(run);
                    current.clear();
                }
                
                TextRun linkRun;
                linkRun.text = text.substr(linkEnd + 1, urlStart - linkEnd - 1);
                linkRun.style = TextStyle::Link;
                linkRun.linkUrl = text.substr(urlStart + 1, urlEnd - urlStart - 1);
                runs.push_back(linkRun);
                
                i = urlEnd;
                continue;
            }
        }
        
        current += text[i];
    }
    
    if (!current.empty()) {
        TextRun run;
        run.text = current;
        run.style = inCode ? TextStyle::Code : (inBold ? TextStyle::Bold : (inItalic ? TextStyle::Italic : TextStyle::Normal));
        runs.push_back(run);
    }
    
    return runs;
}

std::string FirstRunAgreementPopup::StripMarkdown(const std::string& text) {
    std::string result = text;
    
    size_t pos = 0;
    while ((pos = result.find("**", pos)) != std::string::npos) {
        result.replace(pos, 2, "");
    }
    
    pos = 0;
    while ((pos = result.find("*", pos)) != std::string::npos) {
        result.replace(pos, 1, "");
    }
    pos = 0;
    while ((pos = result.find("_", pos)) != std::string::npos) {
        result.replace(pos, 1, "");
    }
    
    pos = 0;
    while ((pos = result.find("`", pos)) != std::string::npos) {
        result.replace(pos, 1, "");
    }
    
    std::regex linkRegex(R"(\[([^\]]+)\]\([^\)]+\))");
    result = std::regex_replace(result, linkRegex, "$1");
    
    return result;
}

std::string FirstRunAgreementPopup::ExtractTitle(const std::string& content) {
    std::istringstream input(content);
    std::string line;
    while (std::getline(input, line)) {
        const std::string trimmed = Trim(line);
        if (StartsWith(trimmed, "# ")) {
            return StripMarkdown(trimmed.substr(2));
        }
        if (StartsWith(trimmed, "## ")) {
            return StripMarkdown(trimmed.substr(3));
        }
        if (StartsWith(trimmed, "### ")) {
            return StripMarkdown(trimmed.substr(4));
        }
    }
    return "License Agreement";
}

// ============================================================================
// OLD PARSING (REMOVED - replaced by ParseInlineText)
// ============================================================================

/*
std::vector<FirstRunAgreementPopup::TextSpan> FirstRunAgreementPopup::ParseInlineFormatting(const std::string& text) {
    std::vector<TextSpan> spans;
    std::string current;
    bool inBold = false;
    bool inItalic = false;
    bool inCode = false;
    bool inLink = false;
    std::string linkUrl;
    
    for (size_t i = 0; i < text.length(); ++i) {
        // Bold: **text**
        if (i + 1 < text.length() && text[i] == '*' && text[i + 1] == '*') {
            if (!current.empty()) {
                TextSpan span;
                span.text = current;
                span.type = inCode ? SpanType::Code : (inBold ? SpanType::Bold : (inItalic ? SpanType::Italic : SpanType::Normal));
                span.linkUrl = inLink ? linkUrl : "";
                spans.push_back(span);
                current.clear();
            }
            inBold = !inBold;
            i += 1;
            continue;
        }
        
        // Italic: *text* or _text_
        if ((text[i] == '*' || text[i] == '_') && !inBold) {
            if (!current.empty()) {
                TextSpan span;
                span.text = current;
                span.type = inCode ? SpanType::Code : (inItalic ? SpanType::Italic : SpanType::Normal);
                span.linkUrl = inLink ? linkUrl : "";
                spans.push_back(span);
                current.clear();
            }
            inItalic = !inItalic;
            continue;
        }
        
        // Code: `text`
        if (text[i] == '`') {
            if (!current.empty()) {
                TextSpan span;
                span.text = current;
                span.type = inCode ? SpanType::Code : (inBold ? SpanType::Bold : (inItalic ? SpanType::Italic : SpanType::Normal));
                span.linkUrl = inLink ? linkUrl : "";
                spans.push_back(span);
                current.clear();
            }
            inCode = !inCode;
            continue;
        }
        
        // Links: [text](url)
        if (text[i] == '[' && !inCode) {
            size_t linkEnd = text.find(']', i);
            size_t urlStart = text.find('(', linkEnd);
            size_t urlEnd = text.find(')', urlStart);
            
            if (linkEnd != std::string::npos && urlStart != std::string::npos && urlEnd != std::string::npos) {
                if (!current.empty()) {
                    TextSpan span;
                    span.text = current;
                    span.type = SpanType::Normal;
                    spans.push_back(span);
                    current.clear();
                }
                
                TextSpan linkSpan;
                linkSpan.text = text.substr(linkEnd + 1, urlStart - linkEnd - 1);
                linkSpan.type = SpanType::Link;
                linkSpan.linkUrl = text.substr(urlStart + 1, urlEnd - urlStart - 1);
                spans.push_back(linkSpan);
                
                i = urlEnd;
                continue;
            }
        }
        
        current += text[i];
    }
    
    if (!current.empty()) {
        TextSpan span;
        span.text = current;
        span.type = inCode ? SpanType::Code : (inBold ? SpanType::Bold : (inItalic ? SpanType::Italic : SpanType::Normal));
        span.linkUrl = inLink ? linkUrl : "";
        spans.push_back(span);
    }
    
    return spans;
}

std::string FirstRunAgreementPopup::StripMarkdownFormatting(const std::string& text) {
    std::string result = text;
    
    // Remove bold
    size_t pos = 0;
    while ((pos = result.find("**", pos)) != std::string::npos) {
        result.replace(pos, 2, "");
    }
    
    // Remove italic
    pos = 0;
    while ((pos = result.find("*", pos)) != std::string::npos) {
        result.replace(pos, 1, "");
    }
    pos = 0;
    while ((pos = result.find("_", pos)) != std::string::npos) {
        result.replace(pos, 1, "");
    }
    
    // Remove code
    pos = 0;
    while ((pos = result.find("`", pos)) != std::string::npos) {
        result.replace(pos, 1, "");
    }
    
    // Remove links [text](url) -> text
    std::regex linkRegex(R"(\[([^\]]+)\]\([^\)]+\))");
    result = std::regex_replace(result, linkRegex, "$1");
    
    return result;
}

std::string FirstRunAgreementPopup::ExtractTitleFromMarkdown(const std::string& content) {
    std::istringstream input(content);
    std::string line;
    while (std::getline(input, line)) {
        const std::string trimmed = Trim(line);
        if (StartsWith(trimmed, "# ")) {
            return StripMarkdownFormatting(trimmed.substr(2));
        }
        if (StartsWith(trimmed, "## ")) {
            return StripMarkdownFormatting(trimmed.substr(3));
        }
        if (StartsWith(trimmed, "### ")) {
            return StripMarkdownFormatting(trimmed.substr(4));
        }
    }
    return "License Agreement";
}
*/

// ============================================================================
// OLD LAYOUT (REMOVED - replaced by ComputeLayout)
// ============================================================================

/*
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
        context.DrawText("•", we::runtime::kindui::Point{ bulletX, textY }, ThemeColor(ColorToken::TextPrimary), fontSize, true);
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
        context.DrawText("•", we::runtime::kindui::Point{ bulletX, textY }, ThemeColor(ColorToken::TextPrimary), fontSize, true);
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

