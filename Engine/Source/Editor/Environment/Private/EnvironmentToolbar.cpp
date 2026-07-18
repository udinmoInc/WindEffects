#include "Environment/EnvironmentEditorApi.h"

#include "Environment/EnvironmentSystem.h"
#include "Environment/EnvironmentTypes.h"
#include "Environment/EnvironmentTypes.h"
#include "Scene/Entity.h"
#include "Scene/Scene.h"
#include "Widgets/PropertyEditor.h"
#include "ContentBrowser/Widgets/TreeView.h"
#include "Widgets/MenuBar.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/ToolbarButtonChrome.h"
#include "KindUI/Rendering/IconMetrics.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <vector>
#include <cstdio>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::editor::environment {
using ::we::runtime::world::environment::EnvironmentSystem;

using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::Animator;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace ToolbarButtonChrome = ::we::runtime::kindui::ToolbarButtonChrome;
using ::we::runtime::kindui::DPIContext;



namespace {

class EnvironmentDropdownMenu : public we::runtime::kindui::Widget {
public:
    explicit EnvironmentDropdownMenu(std::vector<std::shared_ptr<::we::editor::menus::MenuItem>> items)
        : m_Items(std::move(items)) {}

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override {
        const float rowH = ThemeMetric(we::runtime::kindui::MetricToken::ListRowHeight);
        const float padY = ThemeMetric(we::runtime::kindui::MetricToken::Space1);
        const float textSize = ThemeMetric(we::runtime::kindui::MetricToken::TextSizeSmall);
        float maxWidth = 200.0f;
        for (const auto& item : m_Items) {
            maxWidth = std::max(maxWidth, ThemeMetric(we::runtime::kindui::MetricToken::Space6) + item->label.length() * textSize * 0.52f + 28.0f);
        }
        m_DesiredSize = we::runtime::kindui::Size{ maxWidth, padY * 2.0f + m_Items.size() * rowH };
        return m_DesiredSize;
    }

    void Arrange(const we::runtime::kindui::Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(we::runtime::kindui::PaintContext& context) override {
        context.DrawShadow(m_Geometry, ThemeColor(we::runtime::kindui::ColorToken::ShadowPopup), 3.0f, 8.0f);
        context.DrawRoundedRect(m_Geometry, ThemeColor(we::runtime::kindui::ColorToken::PopupBackground), ThemeMetric(we::runtime::kindui::MetricToken::CornerRadiusSmall));

        const float rowH = ThemeMetric(we::runtime::kindui::MetricToken::ListRowHeight);
        const float padY = ThemeMetric(we::runtime::kindui::MetricToken::Space1);
        const float padX = ThemeMetric(we::runtime::kindui::MetricToken::Space2);
        const float textSize = ThemeMetric(we::runtime::kindui::MetricToken::TextSizeSmall);
        const float iconSize = ThemeMetric(we::runtime::kindui::MetricToken::IconSizeTree);

        float y = m_Geometry.y + padY;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            const auto& item = m_Items[i];
            we::runtime::kindui::Rect row{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, rowH };
            if (!item->enabled) {
                y += rowH;
                continue;
            }
            if (static_cast<int>(i) == m_Hovered) {
                context.DrawRect(row, ThemeColor(we::runtime::kindui::ColorToken::HoverBackground));
            }
            context.DrawText(item->label, we::runtime::kindui::Point{ row.x + padX, row.y + (rowH - textSize) * 0.5f },
                ThemeColor(we::runtime::kindui::ColorToken::TextPrimary), textSize);
            if (item->checked) {
                we::runtime::kindui::IconPainter::DrawIcon(context, we::runtime::kindui::Icons::CheckName,
                    we::runtime::kindui::Rect{ row.x + row.width - padX - iconSize, row.y + (rowH - iconSize) * 0.5f, iconSize, iconSize },
                    ThemeColor(we::runtime::kindui::ColorToken::AccentPrimary));
            }
            y += rowH;
        }
    }

    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override {
        m_Hovered = -1;
        const float rowH = ThemeMetric(we::runtime::kindui::MetricToken::ListRowHeight);
        const float padY = ThemeMetric(we::runtime::kindui::MetricToken::Space1);
        float y = m_Geometry.y + padY;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            we::runtime::kindui::Rect row{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, rowH };
            if (row.Contains(event.position)) {
                m_Hovered = static_cast<int>(i);
                break;
            }
            y += rowH;
        }
    }

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override {
        if (event.button != we::runtime::kindui::MouseButton::Left) {
            return;
        }

        const float rowH = ThemeMetric(we::runtime::kindui::MetricToken::ListRowHeight);
        const float padY = ThemeMetric(we::runtime::kindui::MetricToken::Space1);
        float y = m_Geometry.y + padY;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            we::runtime::kindui::Rect row{ m_Geometry.x + 1.0f, y, m_Geometry.width - 2.0f, rowH };
            if (row.Contains(event.position) && m_Items[i]->enabled && m_Items[i]->onClick) {
                m_Items[i]->onClick();
                we::programs::editor::GetEditorPopupHost()->CloseAllPopups();
                return;
            }
            y += rowH;
        }
    }

private:
    std::vector<std::shared_ptr<::we::editor::menus::MenuItem>> m_Items;
    int m_Hovered = -1;
};

class EnvironmentToolbarButton : public we::runtime::kindui::Widget {
public:
    EnvironmentToolbarButton() = default;

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override {
        (void)availableSize;
        const float uiScale = (std::max)(1.0f, we::runtime::kindui::DPIContext::GetScale());
        const float padH = we::runtime::kindui::ToolbarButtonChrome::HorizontalPad(uiScale);
        const float iconSz = we::runtime::kindui::ToolbarButtonChrome::IconSize(uiScale);
        const float iconGap = we::runtime::kindui::ToolbarButtonChrome::IconGapPx(uiScale);
        const float chevW = we::runtime::kindui::IconMetrics::CompactDisplayPx();
        const float controlH = ThemeMetric(we::runtime::kindui::MetricToken::IconButtonSize) * uiScale;
        m_DesiredSize = we::runtime::kindui::Size{
            padH + iconSz + iconGap + chevW + padH,
            controlH
        };
        return m_DesiredSize;
    }

    void Arrange(const we::runtime::kindui::Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(we::runtime::kindui::PaintContext& context) override {
        const float uiScale = (std::max)(1.0f, we::runtime::kindui::DPIContext::GetScale());
        m_HoverAnim = we::runtime::kindui::Animator::Damp(
            m_HoverAnim, m_Hovered ? 1.0f : 0.0f,
            ThemeMetric(we::runtime::kindui::MetricToken::HoverAnimationDamping));

        const float pressStrength = m_Pressed ? 1.0f : 0.0f;
        we::runtime::kindui::ToolbarButtonChrome::PaintIconButton(
            context, m_Geometry, m_HoverAnim, pressStrength, false, 0.0f, uiScale);

        const float centerY = m_Geometry.y + m_Geometry.height * 0.5f;
        const float padH = we::runtime::kindui::ToolbarButtonChrome::HorizontalPad(uiScale);
        const float iconSize = we::runtime::kindui::ToolbarButtonChrome::IconSize(uiScale);
        we::runtime::kindui::Color iconColor = we::runtime::kindui::ToolbarButtonChrome::ResolveIconColor(
            m_HoverAnim, pressStrength, false);

        we::runtime::kindui::IconPainter::DrawIcon(context, we::runtime::kindui::Icons::ToolbarEnvironmentName,
            we::runtime::kindui::ToolbarButtonChrome::PlaceIconInControl(
                we::runtime::kindui::Rect{ m_Geometry.x + padH, centerY - iconSize * 0.5f, iconSize, iconSize },
                iconSize),
            iconColor);

        const float display = we::runtime::kindui::IconMetrics::CompactDisplayPx();
        const float chevronX = m_Geometry.x + m_Geometry.width - padH - display;
        we::runtime::kindui::Rect chevronControl{ chevronX, centerY - display * 0.5f, display, display };
        we::runtime::kindui::IconPainter::DrawCompactIcon(context, we::runtime::kindui::Icons::ChevronDownName,
            chevronControl, iconColor);
    }

    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override {
        m_Hovered = m_Geometry.Contains(event.position);
    }

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override {
        if (event.button != we::runtime::kindui::MouseButton::Left) {
            return;
        }
        m_Pressed = true;
        ShowMenu();
    }

    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override {
        (void)event;
        m_Pressed = false;
    }

    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override {
        return m_Geometry.Contains(position);
    }

private:
    void ShowMenu() {
        auto makeItem = [](const std::string& label, std::function<void()> onClick, bool checked = false, bool enabled = true) {
            auto item = std::make_shared<::we::editor::menus::MenuItem>();
            item->label = label;
            item->onClick = std::move(onClick);
            item->checked = checked;
            item->enabled = enabled;
            return item;
        };

        EnvironmentSystem& system = EnvironmentSystem::Get();
        std::vector<std::shared_ptr<::we::editor::menus::MenuItem>> items;
        items.push_back(makeItem("Create Environment", []() { EnvironmentSystem::Get().CreateEnvironment(); }));
        items.push_back(makeItem("Reset Environment", []() { EnvironmentSystem::Get().ResetEnvironment(); }));
        items.push_back(makeItem("Remove Environment", []() { EnvironmentSystem::Get().RemoveEnvironment(); }));
        items.push_back(makeItem("Rebuild Environment", []() { EnvironmentSystem::Get().RebuildEnvironment(); }));
        items.push_back(makeItem("", []() {}, false, false));
        items.push_back(makeItem("Volumetric Fog", []() {
            EnvironmentSystem& env = EnvironmentSystem::Get();
            env.SetVolumetricFogEnabled(!env.IsVolumetricFogEnabled());
        }, system.IsVolumetricFogEnabled()));
        items.push_back(makeItem("Volumetric Clouds", []() {
            EnvironmentSystem& env = EnvironmentSystem::Get();
            env.SetVolumetricCloudsEnabled(!env.IsVolumetricCloudsEnabled());
        }, system.IsVolumetricCloudsEnabled()));
        items.push_back(makeItem("", []() {}, false, false));
        items.push_back(makeItem("Cloud: Clear Sky", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::ClearSky); }));
        items.push_back(makeItem("Cloud: Few Clouds", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::FewClouds); }));
        items.push_back(makeItem("Cloud: Scattered", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::ScatteredClouds); }));
        items.push_back(makeItem("Cloud: Broken", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::BrokenClouds); }));
        items.push_back(makeItem("Cloud: Overcast", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::Overcast); }));
        items.push_back(makeItem("Cloud: Storm", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::Storm); }));
        items.push_back(makeItem("Cloud: Heavy Storm", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::HeavyStorm); }));
        items.push_back(makeItem("Cloud: Sunset", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::SunsetClouds); }));
        items.push_back(makeItem("Cloud: Sunrise", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::SunriseClouds); }));
        items.push_back(makeItem("Cloud: High Cirrus", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::HighCirrus); }));
        items.push_back(makeItem("Cloud: Cumulus", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::Cumulus); }));
        items.push_back(makeItem("Cloud: Stratocumulus", []() { EnvironmentSystem::Get().ApplyCloudPreset(we::runtime::world::environment::CloudPreset::Stratocumulus); }));
        items.push_back(makeItem("", []() {}, false, false));
        items.push_back(makeItem("Preset: Sunny", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Sunny); }));
        items.push_back(makeItem("Preset: Sunset", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Sunset); }));
        items.push_back(makeItem("Preset: Night", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Night); }));
        items.push_back(makeItem("Preset: Overcast", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Overcast); }));
        items.push_back(makeItem("Preset: Foggy", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Foggy); }));
        items.push_back(makeItem("Preset: Studio", []() { EnvironmentSystem::Get().ApplyPreset(EnvironmentPreset::Studio); }));

        auto menu = std::make_shared<EnvironmentDropdownMenu>(items);
        auto* overlay = we::programs::editor::GetEditorPopupHost();
        if (!overlay) {
            return;
        }
        overlay->CloseAllPopups();
        overlay->ShowPopup(menu, we::runtime::kindui::Point{ m_Geometry.x, m_Geometry.y + m_Geometry.height + 2.0f });
    }

    bool m_Hovered = false;
    bool m_Pressed = false;
    float m_HoverAnim = 0.0f;
};


} // namespace

std::shared_ptr<we::runtime::kindui::Widget> CreateEnvironmentToolbarMenu() {
    return std::make_shared<EnvironmentToolbarButton>();
}

} // namespace we::editor::environment