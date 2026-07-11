#include "Widgets/EditorModeSelector.h"
#include "EditorModeController.h"
#include "EditorToolsRegistry.h"
#include "Core/PaintContext.h"
#include "Core/Theme.h"
#include "Core/Icon.h"
#include "Core/DPIContext.h"
#include "Layout/OverlayManager.h"
#include "Widgets/MenuBar.h"

#include <memory>
#include <vector>

namespace we::programs::editor {

namespace {
constexpr float kIconSize     = 18.0f;  // Matches ToolbarInline icon size
constexpr float kHeight       = 22.0f;  // Matches ProjectSelector height
constexpr float kPadding      = 8.0f;
constexpr float kIconGap      = 5.0f;   // Gap between icon and label
constexpr float kChevronWidth = 10.0f;
} // namespace

using WindEffects::Editor::UI::Color;
using WindEffects::Editor::UI::MenuItem;
using WindEffects::Editor::UI::MouseButton;
using WindEffects::Editor::UI::MouseEvent;
using WindEffects::Editor::UI::OverlayManager;
using WindEffects::Editor::UI::PaintContext;
using WindEffects::Editor::UI::Point;
using WindEffects::Editor::UI::Rect;
using WindEffects::Editor::UI::Size;
using WindEffects::Editor::UI::Theme;

class EditorModeMenu : public WindEffects::Editor::UI::Widget {
public:
    explicit EditorModeMenu(std::vector<std::shared_ptr<MenuItem>> items)
        : m_Items(std::move(items)) {}

    Size Measure(const Size& availableSize) override {
        float maxWidth = 180.0f;
        for (const auto& item : m_Items) {
            maxWidth = (std::max)(maxWidth, 24.0f + item->label.length() * 7.0f + 40.0f);
        }
        m_DesiredSize = Size{ maxWidth, 6.0f + m_Items.size() * 28.0f };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(PaintContext& context) override {
        context.DrawShadow(m_Geometry, Color{ 0.0f, 0.0f, 0.0f, 0.15f }, 4.0f, 12.0f);
        context.DrawRoundedRect(m_Geometry, Theme::Get().PanelBackground, 4.0f);
        context.DrawRoundedRectOutline(m_Geometry, Theme::Get().BorderDefault, 1.0f, 4.0f);

        float y = m_Geometry.y + 4.0f;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            const auto& item = m_Items[i];
            Rect row{ m_Geometry.x + 2.0f, y, m_Geometry.width - 4.0f, 26.0f };
            if (static_cast<int>(i) == m_Hovered) {
                context.DrawRoundedRect(row, Theme::Get().HoverOverlay, 3.0f);
            }

            const auto* mode = EditorToolsRegistry::Get().FindMode(item->label);
            (void)mode;

            WindEffects::Editor::UI::IconPainter::DrawIcon(context, item->shortcut,
                Rect{ row.x + 8.0f, row.y + 4.0f, 18.0f, 18.0f }, Theme::Get().TextPrimary);

            context.DrawText(item->label, Point{ row.x + 32.0f, row.y + 6.0f },
                Theme::Get().TextPrimary, 11.0f);

            if (item->checked) {
                WindEffects::Editor::UI::IconPainter::DrawIcon(context, WindEffects::Editor::UI::Icons::CheckName,
                    Rect{ row.x + row.width - 22.0f, row.y + 5.0f, 16.0f, 16.0f },
                    Theme::Get().SelectedAccent);
            }

            y += 28.0f;
        }
    }

    void OnMouseMove(const MouseEvent& event) override {
        m_Hovered = -1;
        float y = m_Geometry.y + 4.0f;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            Rect row{ m_Geometry.x + 2.0f, y, m_Geometry.width - 4.0f, 26.0f };
            if (row.Contains(event.position)) {
                m_Hovered = static_cast<int>(i);
                break;
            }
            y += 28.0f;
        }
    }

    void OnMouseDown(const MouseEvent& event) override {
        if (event.button != MouseButton::Left) return;

        float y = m_Geometry.y + 4.0f;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            Rect row{ m_Geometry.x + 2.0f, y, m_Geometry.width - 4.0f, 26.0f };
            if (row.Contains(event.position)) {
                if (m_Items[i]->onClick) m_Items[i]->onClick();
                break;
            }
            y += 28.0f;
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
    const float labelWidth = (std::max)(48.0f, m_Label.length() * 7.4f);
    m_DesiredSize = Size{ kPadding + kIconSize + kIconGap + labelWidth + 8.0f + kChevronWidth + kPadding, kHeight };
    return m_DesiredSize;
}

void EditorModeSelector::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
}

void EditorModeSelector::Paint(PaintContext& context) {
    const auto& theme = Theme::Get();
    Color bg = m_Pressed ? Color{ 0.14f, 0.14f, 0.14f, 1.0f }
               : (m_Hovered ? theme.HoverOverlay : Color{ 0.18f, 0.18f, 0.18f, 1.0f });
    context.DrawRoundedRect(m_Geometry, bg, 4.0f);
    context.DrawRoundedRectOutline(m_Geometry, theme.BorderDefault, 1.0f, 4.0f);

    const float uiScale = (std::max)(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;

    // Mode icon — 16px, vertically centered
    const float iconSize = kIconSize * uiScale;
    const float iconY = centerY - iconSize * 0.5f;
    WindEffects::Editor::UI::IconPainter::DrawIcon(context, m_IconName.c_str(),
        Point{ m_Geometry.x + kPadding * uiScale, iconY }, iconSize, theme.TextPrimary);

    // Label — 12px text, vertically centered
    const float textX = m_Geometry.x + (kPadding + kIconSize + kIconGap) * uiScale;
    const float textSize = 12.0f * uiScale;
    context.DrawText(m_Label, Point{ textX, centerY - textSize * 0.5f },
        theme.TextPrimary, textSize, true);

    // Chevron — 10px, vertically centered
    const float chevX = m_Geometry.x + m_Geometry.width - (kPadding + kChevronWidth) * uiScale;
    const float chevSize = kChevronWidth * uiScale;
    WindEffects::Editor::UI::IconPainter::DrawIcon(context, WindEffects::Editor::UI::Icons::ChevronDownName,
        Rect{ chevX, centerY - chevSize * 0.5f, chevSize, chevSize },
        theme.TextSecondary);
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
