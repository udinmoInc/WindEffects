#include "Widgets/WindowsPanelMenuButton.h"

#include "Widgets/DropdownMenu.h"
#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"

#include "KindUI/Core/Animator.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

#include <algorithm>

namespace we::editor::toolbar {
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::Animator;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::MetricToken;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace ToolbarButtonChrome = ::we::runtime::kindui::ToolbarButtonChrome;

std::shared_ptr<WindowsPanelMenuButton> WindowsPanelMenuButton::Create(
    const ::we::editor::extensions::UIExtensionRegistry& extensions,
    std::function<void(const std::string& panelId)> onTogglePanel,
    std::function<bool(const std::string& panelId)> isPanelVisible)
{
    auto button = std::shared_ptr<WindowsPanelMenuButton>(new WindowsPanelMenuButton());
    button->m_OnTogglePanel = std::move(onTogglePanel);
    button->m_IsPanelVisible = std::move(isPanelVisible);

    std::vector<const ::we::editor::extensions::PanelRegistration*> panels;
    for (const auto& [panelId, reg] : extensions.GetPanels()) {
        if (reg.descriptor.showInWindowMenu) {
            panels.push_back(&reg);
        }
    }
    std::sort(panels.begin(), panels.end(), [](const auto* a, const auto* b) {
        if (a->descriptor.sortOrder != b->descriptor.sortOrder) {
            return a->descriptor.sortOrder < b->descriptor.sortOrder;
        }
        return a->descriptor.windowMenuLabel < b->descriptor.windowMenuLabel;
    });

    for (const auto* reg : panels) {
        WindowsPanelMenuEntry entry{};
        entry.panelId = reg->descriptor.id;
        entry.label = reg->descriptor.windowMenuLabel.empty()
            ? reg->descriptor.title
            : reg->descriptor.windowMenuLabel;
        if (button->m_IsPanelVisible) {
            entry.visible = button->m_IsPanelVisible(entry.panelId);
        }
        button->m_Entries.push_back(std::move(entry));
    }

    return button;
}

void WindowsPanelMenuButton::RefreshVisibilityState() {
    if (!m_IsPanelVisible) {
        return;
    }
    for (auto& entry : m_Entries) {
        entry.visible = m_IsPanelVisible(entry.panelId);
    }
}

::we::runtime::kindui::Size WindowsPanelMenuButton::Measure(const ::we::runtime::kindui::Size& availableSize) {
    (void)availableSize;
    const float uiScale = std::max(1.0f, DPIContext::GetScale());
    const float padH = ToolbarButtonChrome::ChipHorizontalPad(uiScale);
    const float iconSz = ToolbarButtonChrome::IconSize(uiScale);
    const float iconGap = ToolbarButtonChrome::IconGapPx(uiScale);
    const float chevW = static_cast<float>(IconMetrics::CompactGlyphTierPx());
    const float textSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeToolbar) * uiScale;
    const float textW = textSize * 3.8f;
    const float controlH = ToolbarButtonChrome::RowContentHeight(uiScale);
    m_DesiredSize = ::we::runtime::kindui::Size{
        padH + iconSz + iconGap + textW + iconGap + chevW + padH,
        controlH
    };
    return m_DesiredSize;
}

void WindowsPanelMenuButton::Arrange(const ::we::runtime::kindui::Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void WindowsPanelMenuButton::Paint(::we::runtime::kindui::PaintContext& context) {
    const float uiScale = std::max(1.0f, DPIContext::GetScale());
    m_HoverAnim = Animator::Damp(
        m_HoverAnim,
        m_Hovered ? 1.0f : 0.0f,
        we::runtime::kindui::ResolveMetric(MetricToken::HoverAnimationDamping));

    const float pressStrength = m_Pressed ? 1.0f : 0.0f;
    ToolbarButtonChrome::PaintInlineDropdown(context, m_Geometry, m_HoverAnim, pressStrength, uiScale);

    const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
    const float padH = ToolbarButtonChrome::ChipHorizontalPad(uiScale);
    const float iconSize = ToolbarButtonChrome::IconSize(uiScale);
    const float iconGap = ToolbarButtonChrome::IconGapPx(uiScale);
    const auto iconColor = ToolbarButtonChrome::ResolveIconColor(m_HoverAnim, pressStrength, false);

    IconPainter::DrawIcon(
        context,
        Icons::MonitorName,
        ToolbarButtonChrome::PlaceIconInControl(
            ::we::runtime::kindui::Rect{ m_Geometry.x + padH, centerY - iconSize * 0.5f, iconSize, iconSize },
            iconSize),
        iconColor);

    const float textSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeToolbar) * uiScale;
    const float textX = m_Geometry.x + padH + iconSize + iconGap;
    context.DrawText(
        "Windows",
        ::we::runtime::kindui::Point{ textX, centerY - textSize * 0.5f },
        we::runtime::kindui::ResolveTextForState(m_HoverAnim > 0.01f, false),
        textSize);

    const float tier = static_cast<float>(IconMetrics::CompactGlyphTierPx());
    const float chevronX = m_Geometry.x + m_Geometry.width - padH - tier;
    IconPainter::DrawCompactIcon(
        context,
        Icons::ChevronDownName,
        IconMetrics::CompactGlyphBand(m_Geometry, chevronX),
        iconColor);
}

void WindowsPanelMenuButton::OnMouseDown(const ::we::runtime::kindui::MouseEvent& event) {
    if (event.button == MouseButton::Left) {
        m_Pressed = true;
        ShowMenu();
    }
}

void WindowsPanelMenuButton::OnMouseMove(const ::we::runtime::kindui::MouseEvent& event) {
    m_Hovered = m_Geometry.Contains(event.position);
}

void WindowsPanelMenuButton::OnMouseUp(const ::we::runtime::kindui::MouseEvent& event) {
    (void)event;
    m_Pressed = false;
}

bool WindowsPanelMenuButton::ShowsPointerCursor(const ::we::runtime::kindui::Point& position) const {
    return m_Geometry.Contains(position);
}

void WindowsPanelMenuButton::ShowMenu() {
    RefreshVisibilityState();

    std::vector<std::shared_ptr<::we::editor::menus::MenuItem>> items;
    items.reserve(m_Entries.size());
    for (const auto& entry : m_Entries) {
        auto item = std::make_shared<::we::editor::menus::MenuItem>();
        item->label = entry.label;
        item->checked = entry.visible;
        item->onClick = [this, panelId = entry.panelId]() {
            if (m_OnTogglePanel) {
                m_OnTogglePanel(panelId);
            }
            RefreshVisibilityState();
        };
        items.push_back(std::move(item));
    }

    auto menu = std::make_shared<::we::editor::menus::DropdownMenu>(items);
    auto* overlay = ::we::programs::editor::GetEditorPopupHost();
    if (!overlay) {
        return;
    }
    overlay->CloseAllPopups();
    overlay->ShowPopup(
        menu,
        ::we::runtime::kindui::Point{ m_Geometry.x, m_Geometry.y + m_Geometry.height + 2.0f });
}

} // namespace we::editor::toolbar
