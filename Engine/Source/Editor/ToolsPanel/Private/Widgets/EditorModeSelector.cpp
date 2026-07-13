#include "Widgets/EditorModeSelector.h"
#include "EditorModeController.h"
#include "EditorToolsRegistry.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "Core/ToolbarButtonChrome.h"
#include "Core/Animator.h"
#include "Core/DPIContext.h"
#include "Rendering/IconMetrics.h"
#include "Layout/OverlayManager.h"
#include "Widgets/MenuBar.h"

#include <memory>
#include <vector>

namespace we::programs::editor {

using WindEffects::Editor::UI::Color;
using WindEffects::Editor::UI::MenuItem;
using WindEffects::Editor::UI::MouseButton;
using WindEffects::Editor::UI::MouseEvent;
using WindEffects::Editor::UI::PaintContext;
using WindEffects::Editor::UI::Point;
using WindEffects::Editor::UI::Rect;
using WindEffects::Editor::UI::Size;
using WindEffects::Editor::UI::ThemeToken;

class EditorModeMenu : public WindEffects::Editor::UI::Widget {
public:
    explicit EditorModeMenu(std::vector<std::shared_ptr<MenuItem>> items)
        : m_Items(std::move(items)) {}

    Size Measure(const Size& availableSize) override {
        const float rowH = ThemeMetric(ThemeToken::ListRowHeight);
        const float padY = ThemeMetric(ThemeToken::Space1);
        const float textSize = ThemeMetric(ThemeToken::TextSizeSmall);
        float maxWidth = 160.0f;
        for (const auto& item : m_Items) {
            maxWidth = (std::max)(maxWidth, ThemeMetric(ThemeToken::Space6) + item->label.length() * textSize * 0.52f + 28.0f);
        }
        m_DesiredSize = Size{ maxWidth, padY * 2.0f + m_Items.size() * rowH };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(PaintContext& context) override {
        context.DrawShadow(m_Geometry, ThemeColor(ThemeToken::ShadowPopup), 3.0f, 8.0f);
        context.DrawRoundedRect(m_Geometry, ThemeColor(ThemeToken::PopupBackground), ThemeMetric(ThemeToken::CornerRadiusSmall));

        const float rowH = ThemeMetric(ThemeToken::ListRowHeight);
        const float padY = ThemeMetric(ThemeToken::Space1);
        const float padX = ThemeMetric(ThemeToken::Space2);
        const float textSize = ThemeMetric(ThemeToken::TextSizeSmall);
        const float iconSize = static_cast<float>(WindEffects::Editor::UI::IconMetrics::NativeIconTierPx(ThemeMetric(ThemeToken::IconSizeTree)));

        float y = m_Geometry.y + padY;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            const auto& item = m_Items[i];
            Rect row{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, rowH };
            if (static_cast<int>(i) == m_Hovered) {
                context.DrawRect(row, ThemeColor(ThemeToken::HoverBackground));
            }

            WindEffects::Editor::UI::IconPainter::DrawIcon(context, item->shortcut,
                Rect{ row.x + padX, row.y + (rowH - iconSize) * 0.5f, iconSize, iconSize }, ThemeColor(ThemeToken::TextPrimary));

            context.DrawText(item->label, Point{ row.x + padX + iconSize + ThemeMetric(ThemeToken::Space1), row.y + (rowH - textSize) * 0.5f },
                ThemeColor(ThemeToken::TextPrimary), textSize);

            if (item->checked) {
                WindEffects::Editor::UI::IconPainter::DrawIcon(context, WindEffects::Editor::UI::Icons::CheckName,
                    Rect{ row.x + row.width - padX - iconSize, row.y + (rowH - iconSize) * 0.5f, iconSize, iconSize },
                    ThemeColor(ThemeToken::AccentPrimary));
            }

            y += rowH;
        }
    }

    void OnMouseMove(const MouseEvent& event) override {
        m_Hovered = -1;
        const float rowH = ThemeMetric(ThemeToken::ListRowHeight);
        const float padY = ThemeMetric(ThemeToken::Space1);
        float y = m_Geometry.y + padY;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            Rect row{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, rowH };
            if (row.Contains(event.position)) {
                m_Hovered = static_cast<int>(i);
                break;
            }
            y += rowH;
        }
    }

    void OnMouseDown(const MouseEvent& event) override {
        if (event.button != MouseButton::Left) return;

        const float rowH = ThemeMetric(ThemeToken::ListRowHeight);
        const float padY = ThemeMetric(ThemeToken::Space1);
        float y = m_Geometry.y + padY;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            Rect row{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, rowH };
            if (row.Contains(event.position)) {
                if (m_Items[i]->onClick) m_Items[i]->onClick();
                break;
            }
            y += rowH;
        }

        if (auto* overlay = GetPopupHost()) {
            overlay->CloseAllPopups();
        }
    }

private:
    std::vector<std::shared_ptr<MenuItem>> m_Items;
    int m_Hovered = -1;
};

EditorModeSelector::EditorModeSelector() {
    Refresh();
    EditorModeController::Get().AddModeChangedListener([this](const std::string&) {
        Refresh();
    });
}

void EditorModeSelector::Refresh() {
    const auto& modeId = EditorModeController::Get().GetActiveModeId();
    if (const auto* mode = EditorToolsRegistry::Get().FindMode(modeId)) {
        m_Label = mode->label;
        m_IconName = mode->iconName;
        if (!WindEffects::Editor::UI::Icons::IsKnownIcon(m_IconName)) {
            m_IconName = WindEffects::Editor::UI::Icons::CursorName;
        }
    } else {
        m_Label = "Select";
        m_IconName = WindEffects::Editor::UI::Icons::CursorName;
    }
}

Size EditorModeSelector::Measure(const Size& availableSize) {
    (void)availableSize;
    const float uiScale = (std::max)(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const float padH = WindEffects::Editor::UI::ToolbarButtonChrome::HorizontalPad(uiScale);
    const float iconSz = WindEffects::Editor::UI::ToolbarButtonChrome::IconSize(uiScale);
    const float iconGap = WindEffects::Editor::UI::ToolbarButtonChrome::IconGapPx(uiScale);
    const float chevW = WindEffects::Editor::UI::ToolbarButtonChrome::IconSize(uiScale);
    const float controlH = ThemeMetric(ThemeToken::IconButtonSize) * uiScale;
    m_DesiredSize = Size{
        padH + iconSz + iconGap + chevW + padH,
        controlH
    };
    return m_DesiredSize;
}

void EditorModeSelector::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void EditorModeSelector::Paint(PaintContext& context) {
    const float uiScale = (std::max)(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    m_HoverAnim = WindEffects::Editor::UI::Animator::Damp(
        m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ThemeMetric(ThemeToken::HoverAnimationDamping));

    const float pressStrength = m_Pressed ? 1.0f : 0.0f;
    WindEffects::Editor::UI::ToolbarButtonChrome::PaintIconButton(
        context, m_Geometry, m_HoverAnim, pressStrength, false, 0.0f, uiScale);

    const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
    const float padH = WindEffects::Editor::UI::ToolbarButtonChrome::HorizontalPad(uiScale);
    const float iconSize = WindEffects::Editor::UI::ToolbarButtonChrome::IconSize(uiScale);
    Color iconColor = WindEffects::Editor::UI::ToolbarButtonChrome::ResolveIconColor(
        m_HoverAnim, pressStrength, false);

    WindEffects::Editor::UI::IconPainter::DrawIcon(context, m_IconName,
        Rect{ m_Geometry.x + padH, centerY - iconSize * 0.5f, iconSize, iconSize }, iconColor);

    const float chevSize = WindEffects::Editor::UI::ToolbarButtonChrome::IconSize(uiScale);
    const float chevX = m_Geometry.x + m_Geometry.width - padH - chevSize;
    WindEffects::Editor::UI::IconPainter::DrawIcon(context, WindEffects::Editor::UI::Icons::ChevronDownName,
        WindEffects::Editor::UI::IconMetrics::PlaceGlyphCentered(
            Rect{ chevX, centerY - chevSize * 0.5f, chevSize, chevSize }, chevSize),
        iconColor);
}

void EditorModeSelector::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) return;
    m_Pressed = true;
    OpenModeMenu();
    m_Pressed = false;
}

void EditorModeSelector::OnMouseMove(const MouseEvent& event) {
    m_Hovered = m_Geometry.Contains(event.position);
}

void EditorModeSelector::OpenModeMenu() {
    auto* overlay = GetPopupHost();
    if (!overlay) return;

    overlay->CloseAllPopups();

    std::vector<std::shared_ptr<MenuItem>> items;
    const std::string& activeId = EditorModeController::Get().GetActiveModeId();

    for (const auto* mode : EditorToolsRegistry::Get().GetModesSorted()) {
        auto item = std::make_shared<MenuItem>();
        item->label = mode->label;
        item->shortcut = mode->iconName;
        item->checked = mode->id == activeId;
        const std::string modeId = mode->id;
        item->onClick = [modeId]() {
            EditorModeController::Get().SetActiveMode(modeId);
        };
        items.push_back(item);
    }

    auto menu = std::make_shared<EditorModeMenu>(std::move(items));
    Point popupPos{ m_Geometry.x, m_Geometry.y + m_Geometry.height + 2.0f };
    overlay->ShowPopup(menu, popupPos);
}

} // namespace we::programs::editor
