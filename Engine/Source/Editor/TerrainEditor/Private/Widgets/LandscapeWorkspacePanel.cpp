#include "TerrainEditor/Widgets/LandscapeWorkspacePanel.h"
#include "LandscapeWorkspaceInternal.h"
#include "KindUI/Widgets/Components.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "WindEffects/Editor/UI/Panel/PanelBodyLayout.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"

#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::editor::terrain {
namespace {
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::ResolveMetric;
using we::editor::panels::PanelBodyRegion;
} // namespace

LandscapeWorkspacePanel::LandscapeWorkspacePanel(ILandscapeEditor* editor) : m_Editor(editor) {
    SetFlexGrow(1.0f);
    SetFlexShrink(1.0f);
    SetMinSize({0.0f, 0.0f});

    m_BodyLayout = std::make_shared<we::editor::panels::PanelBodyLayout>();

    m_TabBar = std::make_shared<we::runtime::kindui::Row>();
    m_TabBar->Gap(ResolveMetric(MetricToken::Space1));
    m_TabBar->SetFlexShrink(0.0f);

    m_ScrollArea = std::make_shared<we::runtime::kindui::ScrollLayout>();
    m_ScrollArea->SetFlexGrow(1.0f);
    m_ScrollArea->SetFlexShrink(1.0f);
    m_ScrollArea->SetMinSize({0.0f, 0.0f});

    m_TabContent = std::make_shared<we::runtime::kindui::Column>();
    ConfigureLandscapeFormColumn(m_TabContent);
    m_TabContent->SetMinSize({0.0f, 0.0f});
    m_ScrollArea->SetContent(m_TabContent);

    m_FooterButton = we::runtime::kindui::MakePrimaryAction("Create Landscape");
    m_FooterButton->SetFlexShrink(0.0f);

    SyncDefaultTab();
    RebuildLayout();
}

LandscapeWorkspacePanel::~LandscapeWorkspacePanel() {
    if (m_FooterButton) {
        m_FooterButton->SetOnClicked({});
    }
    if (m_TabBar) {
        m_TabBar->ClearChildren();
    }
    if (m_BodyLayout) {
        m_BodyLayout->ClearRegions();
    }
    m_FooterButton.reset();
    m_TabContent.reset();
    m_ScrollArea.reset();
    m_TabBar.reset();
    m_BodyLayout.reset();
}

void LandscapeWorkspacePanel::RegisterExtension(LandscapeWorkspaceTab tab, ExtensionFactory factory) {
    m_Extensions[static_cast<int>(tab)] = std::move(factory);
    RebuildLayout();
}

void LandscapeWorkspacePanel::SyncDefaultTab() {
    if (!m_Editor || m_UserSelectedTab) return;
    m_ActiveTab = m_Editor->HasLandscape() ? LandscapeWorkspaceTab::Sculpt : LandscapeWorkspaceTab::Create;
}

void LandscapeWorkspacePanel::SetActiveTab(LandscapeWorkspaceTab tab) {
    if (m_ActiveTab == tab) return;
    m_ActiveTab = tab;
    m_UserSelectedTab = true;
    RebuildLayout();
}

void LandscapeWorkspacePanel::ActivateSculptTool(runtime_terrain::TerrainBrushOp op) {
    (void)op;
}

void LandscapeWorkspacePanel::RebuildLayout() {
    if (!m_BodyLayout || !m_TabContent || !m_Editor) return;

    m_TabBar->ClearChildren();
    auto addTab = [&](const char* label, LandscapeWorkspaceTab tab) {
        auto btn = we::runtime::kindui::MakePanelTab(label);
        btn->SetActive(m_ActiveTab == tab);
        btn->SetFlexGrow(1.0f);
        btn->SetFlexShrink(1.0f);
        btn->SetMinWidth(we::runtime::kindui::LayoutMetrics::FormChipButtonMinWidth());
        btn->SetOnClicked([this, tab]() { SetActiveTab(tab); });
        m_TabBar->AddChild(btn);
    };

    addTab("Create", LandscapeWorkspaceTab::Create);
    addTab("Sculpt", LandscapeWorkspaceTab::Sculpt);
    addTab("Paint", LandscapeWorkspaceTab::Paint);
    addTab("Manage", LandscapeWorkspaceTab::Manage);

    m_TabContent->ClearChildren();
    switch (m_ActiveTab) {
    case LandscapeWorkspaceTab::Create: BuildCreateTab(m_TabContent, *m_Editor); break;
    case LandscapeWorkspaceTab::Sculpt: BuildSculptTab(m_TabContent, *m_Editor); break;
    case LandscapeWorkspaceTab::Paint: BuildPaintTab(m_TabContent, *m_Editor); break;
    case LandscapeWorkspaceTab::Manage:
        BuildManageTab(m_TabContent, *m_Editor, m_ImportPath, m_ExportPath, m_ResizeX, m_ResizeY);
        break;
    }

    m_BodyLayout->SetRegion(PanelBodyRegion::ColumnHeader, m_TabBar);
    m_BodyLayout->SetRegion(PanelBodyRegion::Content, m_ScrollArea);

    const bool showFooter = m_ActiveTab == LandscapeWorkspaceTab::Create;
    if (showFooter) {
        m_FooterButton->SetOnClicked([this]() {
            m_Editor->Wizard().State() = m_Editor->Dialog();
            if (m_Editor->CreateFromDialog()) {
                m_UserSelectedTab = true;
                m_ActiveTab = LandscapeWorkspaceTab::Sculpt;
                RebuildLayout();
            }
        });
        m_BodyLayout->SetRegion(PanelBodyRegion::Footer, m_FooterButton);
    } else {
        m_BodyLayout->SetRegion(PanelBodyRegion::Footer, nullptr);
    }

    InvalidateLayout();
}

we::runtime::kindui::Size LandscapeWorkspacePanel::Measure(const we::runtime::kindui::Size& availableSize) {
    if (!m_BodyLayout) {
        return availableSize;
    }
    m_DesiredSize = m_BodyLayout->Measure(availableSize);
    return m_DesiredSize;
}

void LandscapeWorkspacePanel::Arrange(const we::runtime::kindui::Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (m_BodyLayout) {
        m_BodyLayout->Arrange(allottedRect);
    }
}

void LandscapeWorkspacePanel::Paint(we::runtime::kindui::PaintContext& context) {
    if (m_BodyLayout) {
        m_BodyLayout->Paint(context);
    }
}

void LandscapeWorkspacePanel::OnMouseDown(const we::runtime::kindui::MouseEvent& event) {
    if (m_BodyLayout) {
        m_BodyLayout->OnMouseDown(event);
    }
}

void LandscapeWorkspacePanel::OnMouseMove(const we::runtime::kindui::MouseEvent& event) {
    if (m_BodyLayout) {
        m_BodyLayout->OnMouseMove(event);
    }
}

void LandscapeWorkspacePanel::OnMouseUp(const we::runtime::kindui::MouseEvent& event) {
    if (m_BodyLayout) {
        m_BodyLayout->OnMouseUp(event);
    }
}

void LandscapeWorkspacePanel::OnMouseWheel(const we::runtime::kindui::MouseEvent& event) {
    if (m_BodyLayout) {
        m_BodyLayout->OnMouseWheel(event);
    }
}

bool LandscapeWorkspacePanel::CanReceiveMouseWheelAt(const we::runtime::kindui::Point& pos) const {
    if (!m_BodyLayout) {
        return false;
    }
    const auto contentRect = m_BodyLayout->GetRegionRect(PanelBodyRegion::Content);
    return !contentRect.IsEmpty() && contentRect.Contains(pos);
}

std::shared_ptr<we::runtime::kindui::Widget> LandscapeWorkspacePanel::HitTestPoint(
    const we::runtime::kindui::Point& pos,
    const we::runtime::kindui::Rect* clip) {
    if (!IsVisible() || IsPointerTransparent() || !IsEnabled()) {
        return nullptr;
    }
    if ((clip != nullptr && !clip->Contains(pos)) || !m_Geometry.Contains(pos)) {
        return nullptr;
    }
    if (m_BodyLayout) {
        if (auto hit = m_BodyLayout->HitTestPoint(pos, clip)) {
            return hit;
        }
    }
    return nullptr;
}

} // namespace we::editor::terrain
