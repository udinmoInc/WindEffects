#include "EditorCompositionProbes.h"

#include "KindUI/Profiling/UiColorCompositionDiagnostic.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "Widgets/Toolbar.h"
#include "Widgets/StatusBar.h"
#include "ContentBrowser/Widgets/ContentBrowser.h"
#include "ContentBrowser/Widgets/TreeView.h"
#include "WindEffects/Editor/UI/Widgets/DockContainer.h"
#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"

#include <algorithm>
#include <cmath>

namespace we::programs::editor {
namespace {

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Widget;
using ::we::editor::toolbar::Toolbar;
using ::we::editor::shell::StatusBar;
using ::we::editor::contentbrowser::ContentBrowser;
using ::we::editor::contentbrowser::TreeView;
using ::we::editor::docking::DockContainer;
using ::we::editor::panels::Panel;
using ::we::editor::panels::PanelBodyRegion;
using ::we::runtime::kindui::UiColorCompositionDiagnostic;

Rect InsetRect(const Rect& rect, float inset) {
    const float w = std::max(0.0f, rect.width - inset * 2.0f);
    const float h = std::max(0.0f, rect.height - inset * 2.0f);
    return Rect{ rect.x + inset, rect.y + inset, w, h };
}

Rect LeftEdgeStrip(const Rect& rect, float width = 3.0f) {
    return Rect{ rect.x, rect.y, std::min(width, rect.width), rect.height };
}

void AddProbe(const char* name,
              ColorToken token,
              const Rect& widgetRect,
              const Rect& centerRect,
              const Rect& edgeRect = {})
{
    if (centerRect.width < 4.0f || centerRect.height < 4.0f) {
        return;
    }
    UiColorCompositionDiagnostic::SurfaceProbe probe{};
    probe.name = name;
    probe.expectedToken = token;
    probe.widgetRect = widgetRect;
    probe.sampleCenterRect = centerRect;
    probe.sampleEdgeRect = edgeRect;
    UiColorCompositionDiagnostic::Get().RegisterProbe(std::move(probe));
}

struct ProbeFlags {
    bool toolbar = false;
    bool dockChrome = false;
    bool activeTab = false;
    bool inactiveTab = false;
    bool panelProduction = false;
    bool panelHeader = false;
    bool treeBackground = false;
    bool treeRow = false;
    bool searchInput = false;
    bool hoverRow = false;
    bool selectedRow = false;
    bool statusBar = false;
    bool cbHeader = false;
    bool cbTree = false;
    bool cbContent = false;
};

void WalkWidgets(const std::shared_ptr<Widget>& widget, ProbeFlags& flags) {
    if (!widget || !widget->IsVisible()) {
        return;
    }

    const Rect geo = widget->GetGeometry();
    if (geo.width < 8.0f || geo.height < 8.0f) {
        for (const auto& child : widget->GetChildren()) {
            WalkWidgets(child, flags);
        }
        return;
    }

    if (!flags.toolbar) {
        if (auto* toolbar = dynamic_cast<Toolbar*>(widget.get())) {
            flags.toolbar = true;
            const Rect center = InsetRect(geo, 12.0f);
            AddProbe("Toolbar", ColorToken::ToolbarBackground, geo, center, LeftEdgeStrip(center));
        }
    }

    if (!flags.statusBar) {
        if (dynamic_cast<StatusBar*>(widget.get())) {
            flags.statusBar = true;
            const Rect center = InsetRect(geo, 8.0f);
            AddProbe("Status bar", ColorToken::StatusBarBackground, geo, center, LeftEdgeStrip(center));
        }
    }

    if (auto* dock = dynamic_cast<DockContainer*>(widget.get())) {
        const float headerH = dock->GetHeaderHeightDevice();
        if (!flags.dockChrome && headerH > 0.0f) {
            flags.dockChrome = true;
            const Rect header{ geo.x, geo.y, geo.width, std::min(headerH, geo.height) };
            const Rect center = InsetRect(header, 6.0f);
            AddProbe("Dock chrome", ColorToken::DockChromeBackground, geo, center, LeftEdgeStrip(center));
        }

        const int active = dock->GetActiveTab();
        const int tabCount = dock->GetTabCount();
        if (tabCount > 0) {
            const float tabWidth = geo.width / static_cast<float>(std::max(tabCount, 1));
            if (!flags.activeTab && active >= 0) {
                flags.activeTab = true;
                const Rect tab{
                    geo.x + tabWidth * static_cast<float>(active),
                    geo.y,
                    tabWidth,
                    std::min(headerH, geo.height)
                };
                const Rect center = InsetRect(tab, 4.0f);
                AddProbe("Active tab", ColorToken::TabActiveBackground, geo, center, LeftEdgeStrip(center));
            }
            const int inactive = (active == 0 && tabCount > 1) ? 1 : 0;
            if (!flags.inactiveTab && tabCount > 1 && inactive != active) {
                flags.inactiveTab = true;
                const Rect tab{
                    geo.x + tabWidth * static_cast<float>(inactive),
                    geo.y,
                    tabWidth,
                    std::min(headerH, geo.height)
                };
                const Rect center = InsetRect(tab, 4.0f);
                AddProbe("Inactive tab", ColorToken::TabBackground, geo, center, LeftEdgeStrip(center));
            }
        }
    }

    if (auto* panel = dynamic_cast<Panel*>(widget.get())) {
        const float headerH = we::runtime::kindui::ResolveMetric(MetricToken::PanelTabHeight);
        const Rect header{
            geo.x,
            geo.y,
            geo.width,
            std::min(headerH, geo.height)
        };

        if (!flags.panelHeader) {
            flags.panelHeader = true;
            const Rect center = InsetRect(header, 4.0f);
            AddProbe("Panel header", ColorToken::HeaderBackground, geo, center, LeftEdgeStrip(center));
        }

        Rect body = panel->GetRegionRect(PanelBodyRegion::Content);
        if (body.width < 8.0f || body.height < 8.0f) {
            body = Rect{ geo.x, geo.y + headerH, geo.width, std::max(0.0f, geo.height - headerH) };
        }

        if (!flags.panelProduction && body.width >= 16.0f && body.height >= 16.0f) {
            flags.panelProduction = true;
            const Rect center = InsetRect(body, 16.0f);
            AddProbe("Panel (production)", ColorToken::PanelBackground, geo, center, LeftEdgeStrip(center));
            UiColorCompositionDiagnostic::Get().SetPanelAbTarget(body);
        }

        const Rect search = panel->GetRegionRect(PanelBodyRegion::Search);
        if (!flags.searchInput && search.width >= 16.0f && search.height >= 8.0f) {
            flags.searchInput = true;
            const Rect center = InsetRect(search, 4.0f);
            AddProbe("Search/input", ColorToken::InputBackground, geo, center, LeftEdgeStrip(center));
        }
    }

    if (auto* tree = dynamic_cast<TreeView*>(widget.get())) {
using ::we::runtime::kindui::UiColorCompositionDiagnostic;

        const float headerH = ::we::editor::panels::PanelChrome::ColumnHeaderRowHeight();
        const float rowH = we::runtime::kindui::ResolveMetric(MetricToken::ListRowHeight);

        if (!flags.treeBackground) {
            flags.treeBackground = true;
            const Rect bg{
                geo.x,
                geo.y + headerH,
                geo.width,
                std::max(0.0f, geo.height - headerH)
            };
            const Rect center = InsetRect(bg, 8.0f);
            AddProbe("Tree background", ColorToken::SecondarySurface, geo, center, LeftEdgeStrip(center));
        }

        if (!flags.treeRow) {
            flags.treeRow = true;
            const float rowY = geo.y + headerH + rowH * 1.5f;
            const Rect row{ geo.x + 24.0f, rowY - rowH * 0.5f, geo.width - 48.0f, rowH };
            const Rect center = InsetRect(row, 4.0f);
            AddProbe("Tree row", ColorToken::PanelBackground, geo, center, LeftEdgeStrip(center));
        }

        if (!flags.cbHeader && headerH > 0.0f) {
            flags.cbHeader = true;
            const Rect header{ geo.x, geo.y, geo.width, headerH };
            const Rect center = InsetRect(header, 4.0f);
            AddProbe("Content Browser header", ColorToken::HeaderBackground, geo, center, LeftEdgeStrip(center));
        }

        if (!flags.cbTree) {
            flags.cbTree = true;
            const Rect treeArea{
                geo.x,
                geo.y + headerH,
                geo.width * 0.35f,
                std::max(0.0f, geo.height - headerH)
            };
            const Rect center = InsetRect(treeArea, 8.0f);
            AddProbe("Content Browser tree", ColorToken::SecondarySurface, geo, center, LeftEdgeStrip(center));
        }
    }

    if (!flags.cbContent) {
        if (dynamic_cast<ContentBrowser*>(widget.get())) {
            flags.cbContent = true;
            const Rect center = InsetRect(geo, 24.0f);
            AddProbe("Content Browser content", ColorToken::PanelBackground, geo, center, LeftEdgeStrip(center));
        }
    }

    if (!flags.hoverRow && dynamic_cast<TreeView*>(widget.get())) {
        flags.hoverRow = true;
        const float rowH = we::runtime::kindui::ResolveMetric(MetricToken::ListRowHeight);
        const Rect row{ geo.x + 20.0f, geo.y + rowH * 2.0f, geo.width - 40.0f, rowH };
        const Rect center = InsetRect(row, 2.0f);
        AddProbe("Hover row (rest)", ColorToken::PanelBackground, geo, center, LeftEdgeStrip(center));
    }

    if (!flags.selectedRow && dynamic_cast<TreeView*>(widget.get())) {
        flags.selectedRow = true;
        const float rowH = we::runtime::kindui::ResolveMetric(MetricToken::ListRowHeight);
        const Rect row{ geo.x + 20.0f, geo.y + rowH * 3.0f, geo.width - 40.0f, rowH };
        const Rect center = InsetRect(row, 2.0f);
        AddProbe("Selected row (if any)", ColorToken::SelectedBackground, geo, center, LeftEdgeStrip(center));
    }

    for (const auto& child : widget->GetChildren()) {
        WalkWidgets(child, flags);
    }
}

} // namespace

void RegisterEditorCompositionProbes(const std::shared_ptr<Widget>& root) {
    if (!UiColorCompositionDiagnostic::IsEnabled() || !root) {
        return;
    }
    UiColorCompositionDiagnostic::Get().ClearProbes();
    ProbeFlags flags{};
    WalkWidgets(root, flags);
}

} // namespace we::programs::editor
