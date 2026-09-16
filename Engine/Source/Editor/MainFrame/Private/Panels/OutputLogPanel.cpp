// ==============================================================================
// WindEffects — MainFrame — OutputLogPanel
// Internal implementation for the MainFrame module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include <KindUI/EditorUI.h>
#include <KindUI/UI/Flex.h>
#include "WindEffects/Editor/EditorSDK.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "Widgets/OutputLogWidget.h"
#include "Widgets/DropdownMenu.h"
#include "Core/Paths.h"
#include <algorithm>
#include <filesystem>
#include <cstdlib>

#ifdef DrawText
#undef DrawText
#endif

namespace we::programs::editor {
namespace WindIcons = ::we::runtime::kindui::WindIcons;
using ::we::runtime::kindui::kWindIconNone;
using namespace ::we::runtime::kindui;
using ::we::runtime::kindui::panels::Panel;
using ::we::editor::panels::OutputLogWidget;
using ::we::editor::docking::DockZone;
using ::we::editor::menus::MenuItem;
using ::we::editor::menus::DropdownMenu;

namespace {

class OutputLogFooterWidget : public Widget {
public:
    explicit OutputLogFooterWidget(std::shared_ptr<OutputLogWidget> logWidget)
        : m_LogWidget(std::move(logWidget)) {}

    Size Measure(const Size& availableSize) override {
        (void)availableSize;
        const float scale = (std::max)(1.0f, DPIContext::GetScale());
        return Size{ availableSize.width, 24.0f * scale };
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
    }

    void Paint(PaintContext& context) override {
        const float scale = (std::max)(1.0f, DPIContext::GetScale());

        // Thin footer bar background
        context.DrawSurface(m_Geometry, SurfaceRole::Toolbar, 0.0f, "OutputLogFooter");

        // Top 1px border line
        const Color borderColor = ResolveColor(ColorToken::Separator);
        context.DrawRect(Rect{ m_Geometry.x, m_Geometry.y, m_Geometry.width, 1.0f * scale }, borderColor);

        const float iconSize = 14.0f * scale;
        const float fontSize = 11.0f * scale;
        const float btnH = 20.0f * scale;

        if (!m_LogWidget) return;

        // Shared vertical positions for right status counters & Filters button
        const float counterIconY = m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f;
        const float counterTextY = m_Geometry.y + (m_Geometry.height - fontSize) * 0.5f - 1.0f * scale;
        const float btnTextY = m_Geometry.y + (m_Geometry.height - fontSize) * 0.5f - 1.0f * scale;
        const float btnIconY = m_Geometry.y + (m_Geometry.height - iconSize) * 0.5f;

        const float gap = 4.0f * scale;
        const float groupGap = 14.0f * scale;

        // 1. Filters ▾ Button on the LEFT
        const float filterBtnY = m_Geometry.y + (m_Geometry.height - btnH) * 0.5f;
        const float filterBtnWidth = 84.0f * scale;
        m_FilterBtnRect = Rect{ m_Geometry.x + 6.0f * scale, filterBtnY, filterBtnWidth, btnH };
        if (m_FilterBtnHovered) {
            context.DrawSurface(m_FilterBtnRect, SurfaceRole::ControlHover, 3.0f * scale, "FooterFiltersBtn");
        }
        IconPainter::Draw(context, WindIcons::ListFilter16, Point{ m_FilterBtnRect.x + 4.0f * scale, btnIconY });
        context.DrawText("Filters", Point{ m_FilterBtnRect.x + 4.0f * scale + iconSize + 3.0f * scale, btnTextY }, ResolveColor(ColorToken::TextPrimary), fontSize);
        IconPainter::Draw(context, WindIcons::ChevronDown16, Point{ m_FilterBtnRect.x + m_FilterBtnRect.width - iconSize - 2.0f * scale, btnIconY });

        // 2. Right Status Counters (Warning & Error only)
        const std::string warnStr = std::to_string(m_LogWidget->GetWarningCount());
        const std::string errStr = std::to_string(m_LogWidget->GetErrorCount());

        const float warnWidth = iconSize + gap + (static_cast<float>(warnStr.size()) * 7.5f * scale);
        const float errWidth = iconSize + gap + (static_cast<float>(errStr.size()) * 7.5f * scale);

        const float totalRightWidth = warnWidth + errWidth + groupGap;
        float currentX = m_Geometry.x + m_Geometry.width - totalRightWidth - 10.0f * scale;

        const Color textPrimary = ResolveColor(ColorToken::TextPrimary);

        // Warning Counter (Untinted native colored icon + primary text)
        IconPainter::Draw(context, WindIcons::Alert16, Point{ currentX, counterIconY });
        currentX += iconSize + gap;
        context.DrawText(warnStr, Point{ currentX, counterTextY }, textPrimary, fontSize);
        currentX += (static_cast<float>(warnStr.size()) * 7.5f * scale) + groupGap;

        // Error Counter (Untinted native colored icon + primary text)
        IconPainter::Draw(context, WindIcons::Error16, Point{ currentX, counterIconY });
        currentX += iconSize + gap;
        context.DrawText(errStr, Point{ currentX, counterTextY }, textPrimary, fontSize);
    }

    void OnMouseDown(const MouseEvent& event) override {
        if (m_FilterBtnRect.Contains(event.position)) {
            auto popupHost = EditorWorkspaceController::Get().GetPopupHost();
            if (!popupHost || !m_LogWidget) return;

            std::vector<std::shared_ptr<MenuItem>> items;
            auto addFilterItem = [&](const char* label, we::Logger::Level level) {
                auto item = std::make_shared<MenuItem>();
                item->label = label;
                item->onClick = [this, level]() {
                    m_LogWidget->SetMinimumLevel(level);
                };
                items.push_back(item);
            };

            addFilterItem("All Levels (Trace+)", we::Logger::Level::Trace);
            addFilterItem("Info & Higher", we::Logger::Level::Info);
            addFilterItem("Warnings & Errors", we::Logger::Level::Warning);
            addFilterItem("Errors Only", we::Logger::Level::Error);

            auto menu = std::make_shared<DropdownMenu>(items);
            popupHost->CloseTransientPopups();
            popupHost->ShowAnchoredPopup(
                menu,
                Rect{ m_FilterBtnRect.x, m_FilterBtnRect.y - 120.0f, 150.0f, 24.0f },
                PopupPlacementMode::TopPreferred);
        }
    }

    void OnMouseMove(const MouseEvent& event) override {
        const bool hoveredFilter = m_FilterBtnRect.Contains(event.position);
        if (hoveredFilter != m_FilterBtnHovered) {
            m_FilterBtnHovered = hoveredFilter;
            InvalidatePaint();
        }
    }

    void OnHoverLost() override {
        if (m_FilterBtnHovered) {
            m_FilterBtnHovered = false;
            InvalidatePaint();
        }
    }

    bool ShowsPointerCursor(const Point& position) const override {
        return m_FilterBtnRect.Contains(position);
    }

    void Tick(float /*deltaTime*/) override {
        if (m_LogWidget) {
            const size_t total = m_LogWidget->GetTotalCount();
            if (total != m_LastTotal) {
                m_LastTotal = total;
                InvalidatePaint();
            }
        }
    }

private:
    std::shared_ptr<OutputLogWidget> m_LogWidget;
    Rect m_FilterBtnRect;
    bool m_FilterBtnHovered = false;
    size_t m_LastTotal = 0;
};

} // namespace

std::shared_ptr<Panel> CreateOutputLogPanel() {
    auto outputWidget = std::make_shared<OutputLogWidget>();
    auto footerWidget = std::make_shared<OutputLogFooterWidget>(outputWidget);

    return we::editor::dsl::Panel("Output Log", [&](we::editor::dsl::PanelContext& p) {
        p.TabIcon(WindIcons::Console16)
         .WithCloseButton([]() {
             EditorWorkspaceController::Get().SetPanelVisible("OutputLog", false);
         })
         .Content(outputWidget)
         .Footer(footerWidget);

        p.Toolbar([&](we::editor::dsl::ToolbarContext& t) {
            // 1. Search Box ("Search log...") (Left side)
            t.Search("Search log...", [outputWidget](const std::string& text) {
                outputWidget->SetSearchQuery(text);
            });

            // 3. Spacer (Pushes remaining controls to the right)
            t.Spacer();

            // 4. Category Filter Dropdown ("All Categories ▾") (Right side)
            t.Button("All Categories", WindIcons::ChevronDown16, [outputWidget](std::shared_ptr<Widget> btn) {
                auto popupHost = EditorWorkspaceController::Get().GetPopupHost();
                if (!popupHost) return;

                std::vector<std::shared_ptr<MenuItem>> items;
                auto addCatItem = [&](const char* label, const std::string& cat) {
                    auto item = std::make_shared<MenuItem>();
                    item->label = label;
                    item->onClick = [outputWidget, cat]() {
                        outputWidget->SetCategoryFilter(cat);
                    };
                    items.push_back(item);
                };

                addCatItem("All Categories", "");
                addCatItem("Startup", "Startup");
                addCatItem("ApplicationFramework", "ApplicationFramework");
                addCatItem("EditorLoopComponents", "EditorLoopComponents");
                addCatItem("PlatformInputSubsystem", "PlatformInputSubsystem");
                addCatItem("SwapchainSubsystem", "SwapchainSubsystem");
                addCatItem("EditorLayout", "EditorLayout");

                auto menu = std::make_shared<DropdownMenu>(items);
                popupHost->CloseTransientPopups();
                popupHost->ShowAnchoredPopup(
                    menu,
                    btn,
                    PopupPlacementMode::BottomPreferred);
            });

            // 5. Clear Button (Trash icon + "Clear") (Right side)
            t.Button("Clear", WindIcons::Trash16, [outputWidget]() { outputWidget->Clear(); });

            // 6. Pause Button (Pause icon + "Pause") (Right side)
            t.Button("Pause", WindIcons::Pause16, [outputWidget]() { outputWidget->SetPaused(!outputWidget->IsPaused()); });

            // 7. Settings Button (Right side)
            t.IconButton(WindIcons::Settings16, [outputWidget]() {
                outputWidget->SetAutoScroll(!outputWidget->IsAutoScroll());
            });
        });
    });
}

REGISTER_UI_PANEL(OutputLog,
    WE_PANEL(OutputLog).Title("Output Log").Icon("output-log").Zone(DockZone::Bottom).WindowMenu("Output Log").SortOrder(5),
    CreateOutputLogPanel)

}
