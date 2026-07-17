#include "Widgets/EditorModeSelector.h"
#include "WindEffects/Editor/UI/Shell/EditorModeController.h"
#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Layout/OverlayManager.h"
#include "Widgets/MenuBar.h"

#include <algorithm>
#include <memory>
#include <vector>

namespace we::programs::editor {
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::KeyEventType;
using ::we::runtime::kindui::IconPainter;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;


using ::we::runtime::kindui::Color;
using ::we::editor::shell::EditorModeController;
using ::we::editor::toolspanel::EditorToolsRegistry;
namespace ToolbarButtonChrome = ::we::runtime::kindui::ToolbarButtonChrome;
using ::we::editor::menus::MenuItem;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

class EditorModeMenu : public we::runtime::kindui::Widget {
public:
    explicit EditorModeMenu(std::vector<std::shared_ptr<MenuItem>> items)
        : m_Items(std::move(items)) {}

    Size Measure(const Size& availableSize) override {
        const float rowH = ThemeMetric(MetricToken::ListRowHeight);
        const float padY = ThemeMetric(MetricToken::Space1);
        const float textSize = ThemeMetric(MetricToken::TextSizeSmall);
        float maxWidth = 160.0f;
        for (const auto& item : m_Items) {
            maxWidth = (std::max)(maxWidth, ThemeMetric(MetricToken::Space6) + item->label.length() * textSize * 0.52f + 28.0f);
        }
        m_DesiredSize = Size{ maxWidth, padY * 2.0f + m_Items.size() * rowH };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(PaintContext& context) override {
        context.DrawShadow(m_Geometry, ThemeColor(ColorToken::ShadowPopup), 3.0f, 8.0f);
        context.DrawRoundedRect(m_Geometry, ThemeColor(ColorToken::PopupBackground), ThemeMetric(MetricToken::CornerRadiusSmall));

        const float rowH = ThemeMetric(MetricToken::ListRowHeight);
        const float padY = ThemeMetric(MetricToken::Space1);
        const float padX = ThemeMetric(MetricToken::Space2);
        const float textSize = ThemeMetric(MetricToken::TextSizeSmall);
        const float iconSize = static_cast<float>(we::runtime::kindui::IconMetrics::NativeIconTierPx(ThemeMetric(MetricToken::IconSizeTree)));

        float y = m_Geometry.y + padY;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            const auto& item = m_Items[i];
            Rect row{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, rowH };
            if (static_cast<int>(i) == m_Hovered) {
                context.DrawRect(row, ThemeColor(ColorToken::HoverBackground));
            }

            we::runtime::kindui::IconPainter::DrawIcon(context, item->shortcut,
                Rect{ row.x + padX, row.y + (rowH - iconSize) * 0.5f, iconSize, iconSize }, ThemeColor(ColorToken::TextPrimary));

            context.DrawText(item->label, Point{ row.x + padX + iconSize + ThemeMetric(MetricToken::Space1), row.y + (rowH - textSize) * 0.5f },
                ThemeColor(ColorToken::TextPrimary), textSize);

            if (item->checked) {
                we::runtime::kindui::IconPainter::DrawIcon(context, we::runtime::kindui::Icons::CheckName,
                    Rect{ row.x + row.width - padX - iconSize, row.y + (rowH - iconSize) * 0.5f, iconSize, iconSize },
                    ThemeColor(ColorToken::AccentPrimary));
            }

            y += rowH;
        }
    }

    void OnMouseMove(const MouseEvent& event) override {
        m_Hovered = -1;
        const float rowH = ThemeMetric(MetricToken::ListRowHeight);
        const float padY = ThemeMetric(MetricToken::Space1);
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

        const float rowH = ThemeMetric(MetricToken::ListRowHeight);
        const float padY = ThemeMetric(MetricToken::Space1);
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
        if (!we::runtime::kindui::Icons::IsKnownIcon(m_IconName)) {
            m_IconName = we::runtime::kindui::Icons::CursorName;
        }
    } else {
        m_Label = "Select";
        m_IconName = we::runtime::kindui::Icons::CursorName;
    }
}

Size EditorModeSelector::Measure(const Size& availableSize) {
    (void)availableSize;
    const float uiScale = (std::max)(1.0f, we::runtime::kindui::DPIContext::GetScale());
    const float padH = ToolbarButtonChrome::HorizontalPad(uiScale);
    const float iconSz = ToolbarButtonChrome::IconSize(uiScale);
    const float iconGap = ToolbarButtonChrome::IconGapPx(uiScale);
    const float chevW = we::runtime::kindui::IconMetrics::CompactDisplayPx();
    const float controlH = ThemeMetric(MetricToken::IconButtonSize) * uiScale;
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
    const float uiScale = (std::max)(1.0f, we::runtime::kindui::DPIContext::GetScale());
    m_HoverAnim = we::runtime::kindui::Animator::Damp(
        m_HoverAnim, m_Hovered ? 1.0f : 0.0f, ThemeMetric(MetricToken::HoverAnimationDamping));

    const float pressStrength = m_Pressed ? 1.0f : 0.0f;
    ToolbarButtonChrome::PaintIconButton(
        context, m_Geometry, m_HoverAnim, pressStrength, false, 0.0f, uiScale);

    const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
    const float padH = ToolbarButtonChrome::HorizontalPad(uiScale);
    const float iconSize = ToolbarButtonChrome::IconSize(uiScale);
    const float iconGap = ToolbarButtonChrome::IconGapPx(uiScale);
    const float chevSize = we::runtime::kindui::IconMetrics::CompactDisplayPx();
    Color iconColor = ToolbarButtonChrome::ResolveIconColor(
        m_HoverAnim, pressStrength, false);

    const Rect iconBand{
        m_Geometry.x + padH,
        m_Geometry.y,
        (std::max)(iconSize, m_Geometry.width - padH * 2.0f - iconGap - chevSize),
        m_Geometry.height
    };
    we::runtime::kindui::IconPainter::DrawIcon(context, m_IconName,
        ToolbarButtonChrome::PlaceIconInControl(iconBand, iconSize),
        iconColor);

    const float chevX = m_Geometry.x + m_Geometry.width - padH - chevSize;
    we::runtime::kindui::IconPainter::DrawCompactIcon(context, we::runtime::kindui::Icons::ChevronDownName,
        Rect{ chevX, centerY - chevSize * 0.5f, chevSize, chevSize },
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
