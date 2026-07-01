#include "Widgets/ExplorerPanelHeader.hpp"

#include "Explorer/ExplorerPanelAssets.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"
#include "Core/Icon.hpp"

#include <algorithm>
#include <cmath>

namespace we::UI {

namespace {
constexpr float kPadLeft = 10.0f;
constexpr float kPadRight = 8.0f;
constexpr float kLogoTitleGap = 8.0f;
constexpr float kButtonSize = 22.0f;
constexpr float kIconSize = 14.0f;
constexpr float kButtonGap = 2.0f;
} // namespace

ExplorerPanelHeader::ExplorerPanelHeader() {
    m_Buttons = {
        { Icons::PlusName, {} },
        { Icons::SearchName, {} },
        { Icons::SettingsName, {} },
        { Icons::MoreName, {} },
    };
}

Size ExplorerPanelHeader::Measure(const Size& availableSize) {
    m_DesiredSize = Size{ availableSize.width, m_Height };
    return m_DesiredSize;
}

void ExplorerPanelHeader::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;

    float x = allottedRect.x + allottedRect.width - kPadRight;
    for (int i = static_cast<int>(m_Buttons.size()) - 1; i >= 0; --i) {
        x -= kButtonSize;
        const float y = allottedRect.y + (m_Height - kButtonSize) * 0.5f;
        m_Buttons[static_cast<size_t>(i)].geometry = Rect{ x, y, kButtonSize, kButtonSize };
        x -= kButtonGap;
    }
}

void ExplorerPanelHeader::Paint(PaintContext& context) {
    const auto& theme = Theme::Get();

    context.DrawRect(m_Geometry, theme.HeaderBackground);
    context.DrawRect(
        Rect{ m_Geometry.x, m_Geometry.y + m_Geometry.height - 1.0f, m_Geometry.width, 1.0f },
        theme.Separator);

    const float logoSize = we::programs::editor::GetExplorerBrandLogoLogicalSize();
    const float logoY = m_Geometry.y + (m_Height - logoSize) * 0.5f;
    const auto snap = [](float v) { return std::floor(v + 0.5f); };
    Rect logoRect{ snap(m_Geometry.x + kPadLeft), snap(logoY), logoSize, logoSize };

    if (VkDescriptorSet logoSet = we::programs::editor::GetExplorerBrandLogo(); logoSet != VK_NULL_HANDLE) {
        context.DrawTexture(logoRect, logoSet, theme.TextPrimary);
    } else {
        IconPainter::DrawIcon(context, Icons::SunName, logoRect, theme.SelectedAccent);
    }

    const float titleX = logoRect.x + logoRect.width + kLogoTitleGap;
    const float titleSize = theme.TextSizeHeader;
    const float titleY = m_Geometry.y + (m_Height - titleSize) * 0.5f;
    context.DrawText(m_Title, Point{ titleX, titleY }, theme.TextPrimary, titleSize, true);

    for (size_t i = 0; i < m_Buttons.size(); ++i) {
        const auto& button = m_Buttons[i];
        Rect iconRect{
            button.geometry.x + (button.geometry.width - kIconSize) * 0.5f,
            button.geometry.y + (button.geometry.height - kIconSize) * 0.5f,
            kIconSize,
            kIconSize
        };

        if (static_cast<int>(i) == m_HoveredButton || static_cast<int>(i) == m_PressedButton) {
            context.DrawRoundedRect(button.geometry, theme.HoverOverlay, 4.0f);
        }

        Color iconColor = (static_cast<int>(i) == m_HoveredButton || static_cast<int>(i) == m_PressedButton)
            ? theme.TextPrimary
            : theme.TextSecondary;
        IconPainter::DrawIcon(context, button.iconName, iconRect, iconColor);
    }
}

int ExplorerPanelHeader::HitButtonIndex(const Point& position) const {
    for (size_t i = 0; i < m_Buttons.size(); ++i) {
        if (m_Buttons[i].geometry.Contains(position)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void ExplorerPanelHeader::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }

    m_PressedButton = HitButtonIndex(event.position);
    if (m_PressedButton < 0) {
        return;
    }

    std::function<void()> action;
    switch (m_PressedButton) {
    case 0: action = m_OnAdd; break;
    case 1: action = m_OnFilter; break;
    case 2: action = m_OnSettings; break;
    case 3: action = m_OnMore; break;
    default: break;
    }
    if (action) {
        action();
    }
}

void ExplorerPanelHeader::OnMouseMove(const MouseEvent& event) {
    m_HoveredButton = HitButtonIndex(event.position);
}

void ExplorerPanelHeader::OnMouseUp(const MouseEvent& event) {
    (void)event;
    m_PressedButton = -1;
}

bool ExplorerPanelHeader::ShowsPointerCursor(const Point& position) const {
    return HitButtonIndex(position) >= 0;
}

} // namespace we::UI
