// ==============================================================================
// WindEffects — ToolsPanel — EditorModeSelector
// UI widget used by the ToolsPanel module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Widgets/EditorModeSelector.h"
#include "Widgets/DropdownMenu.h"
#include "WindEffects/Editor/UI/Shell/EditorModeController.h"
#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"
#include <KindUI/EditorUI.h>
#include "Widgets/MenuBar.h"

#include <algorithm>
#include <memory>
#include <vector>

namespace we::programs::editor {
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::Animator;
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace LayoutMetrics = ::we::runtime::kindui::LayoutMetrics;


using ::we::runtime::kindui::Color;
using ::we::editor::shell::EditorModeController;
using ::we::editor::toolspanel::EditorToolsRegistry;
namespace ToolbarButtonChrome = ::we::runtime::kindui::ToolbarButtonChrome;
using ::we::editor::menus::MenuItem;
using ::we::editor::menus::DropdownMenu;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

EditorModeSelector::EditorModeSelector() {
    Refresh();
}

EditorModeSelector::~EditorModeSelector() = default;

void EditorModeSelector::InitializeCallbacks(const std::shared_ptr<EditorModeSelector>& self) {
    std::weak_ptr<EditorModeSelector> weak = self;
    EditorModeController::Get().AddModeChangedListener([weak](const std::string&) {
        if (auto self = weak.lock()) {
            self->Refresh();
        }
    });
}

void EditorModeSelector::Refresh() {
    const auto& modeId = EditorModeController::Get().GetActiveModeId();
    if (const auto* mode = EditorToolsRegistry::Get().FindMode(modeId)) {
        m_Label = mode->label;
        m_Icon = mode->icon;
    } else {
        m_Label = "Select";
        m_Icon = we::runtime::kindui::kWindIconNone;
    }
}

Size EditorModeSelector::Measure(const Size& availableSize) {
    (void)availableSize;
    const float uiScale = (std::max)(1.0f, we::runtime::kindui::DPIContext::GetScale());
    const float padH = ToolbarButtonChrome::ChipHorizontalPad(uiScale);
    const float iconSz = ToolbarButtonChrome::IconSize(uiScale);
    const float iconGap = ToolbarButtonChrome::IconGapPx(uiScale);
    const float chevW = static_cast<float>(14u);
    const float textSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeToolbar) * uiScale;
    const float labelW = m_Label.empty() ? 0.0f : (m_Label.length() * (7.2f * uiScale));
    const float controlH = ToolbarButtonChrome::RowContentHeight(uiScale);

    float width = padH;
    if (m_Icon.IsValid()) {
        width += iconSz;
        if (labelW > 0.0f) {
            width += iconGap;
        }
    }
    if (labelW > 0.0f) {
        width += labelW;
    }
    width += iconGap + chevW + padH;

    m_DesiredSize = Size{ width, controlH };
    return m_DesiredSize;
}

void EditorModeSelector::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void EditorModeSelector::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, we::runtime::kindui::DPIContext::GetScale());
    m_HoverAnim = we::runtime::kindui::Animator::Damp(
        m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ThemeMetric(MetricToken::HoverAnimationDamping));

    const float pressStrength = m_Pressed ? 1.0f : 0.0f;
    ToolbarButtonChrome::PaintInlineDropdown(
        context, m_Geometry, m_HoverAnim, pressStrength, uiScale);

    const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
    const float padH = ToolbarButtonChrome::ChipHorizontalPad(uiScale);
    const float iconSize = ToolbarButtonChrome::IconSize(uiScale);
    const float iconGap = ToolbarButtonChrome::IconGapPx(uiScale);
    const float chevSize = static_cast<float>(14u);
    const float textSize = we::runtime::kindui::ResolveMetric(MetricToken::TextSizeToolbar) * uiScale;

    float currentX = m_Geometry.x + padH;
    if (m_Icon.IsValid()) {
        const Rect iconBand{ currentX, centerY - iconSize * 0.5f, iconSize, iconSize };
        ToolbarButtonChrome::PaintFloatingIcon(
            context, m_Icon, iconBand, iconSize, m_HoverAnim, pressStrength, false);
        currentX += iconSize + iconGap;
    }

    if (!m_Label.empty()) {
        const Color textColor = we::runtime::kindui::ResolveTextForState(m_HoverAnim > 0.01f, false);
        context.DrawText(
            m_Label,
            we::runtime::kindui::Point{ currentX, we::runtime::kindui::LayoutMetrics::AlignTextTopAtCenterY(centerY,
                textSize) },
            textColor,
            textSize,
            we::runtime::text::layout::FontWeight::Regular);
        currentX += m_Label.length() * (7.2f * uiScale);
    }

    const float chevX = m_Geometry.x + m_Geometry.width - padH - chevSize;
    ToolbarButtonChrome::PaintFloatingIcon(
        context,
        we::runtime::kindui::WindIcons::ChevronDownV212,
        we::runtime::kindui::IconMetrics::CompactGlyphBand(m_Geometry, chevX),
        chevSize,
        m_HoverAnim,
        pressStrength,
        false);
}

void EditorModeSelector::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) return;
    m_Pressed = true;
    OpenModeMenu();
    m_Pressed = false;
}

void EditorModeSelector::OpenModeMenu() {
    auto* overlay = GetPopupHost();
    if (!overlay) return;

    overlay->CloseAllPopups();

    std::vector<std::shared_ptr<MenuItem>> items;
    const std::string& activeId = EditorModeController::Get().GetActiveModeId();

    for (const auto* mode : EditorToolsRegistry::Get().GetModesSorted()) {
        if (!mode || mode->id.empty() || mode->label.empty()) continue;
        auto item = std::make_shared<MenuItem>();
        item->label = mode->label;
        item->icon = mode->icon;
        item->tooltip = mode->tooltip;
        item->checked = mode->id == activeId;
        const std::string modeId = mode->id;
        item->onClick = [modeId]() {
            EditorModeController::Get().SetActiveMode(modeId);
        };
        items.push_back(item);
    }

    auto menu = std::make_shared<DropdownMenu>(std::move(items));
    Point popupPos{ m_Geometry.x, m_Geometry.y + m_Geometry.height + 2.0f };
    overlay->ShowPopup(menu, popupPos);
}

} // namespace we::programs::editor
