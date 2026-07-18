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
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::MouseButton;
namespace Icons = ::we::runtime::kindui::Icons;

using we::runtime::kindui::ColorToken;
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::PaddingToken;

using namespace first_run_detail;

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


} // namespace we::programs::editor
