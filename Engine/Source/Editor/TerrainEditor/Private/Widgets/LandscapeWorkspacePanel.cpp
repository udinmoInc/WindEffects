#include "TerrainEditor/Widgets/LandscapeWorkspacePanel.h"
#include "LandscapeWorkspaceInternal.h"
#include "KindUI/Widgets/Components.h"
#include "KindUI/Core/LayoutMetrics.h"
#include "KindUI/Core/PropertyPanelChrome.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"

#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"

namespace we::editor::terrain {
namespace {
using we::runtime::kindui::MetricToken;
using we::runtime::kindui::ResolveMetric;
} // namespace

LandscapeWorkspacePanel::LandscapeWorkspacePanel(ILandscapeEditor* editor) : m_Editor(editor) {
    SetFlexGrow(1.0f);
    SetFlexShrink(1.0f);
    SetMinSize({0.0f, 0.0f});
    m_ContentContainer = std::make_shared<we::runtime::kindui::Column>();
    ConfigureLandscapeFormColumn(m_ContentContainer);
    m_ContentContainer->SetMinSize({0.0f, 0.0f});
    SyncDefaultTab();
    RebuildLayout();
}

LandscapeWorkspacePanel::~LandscapeWorkspacePanel() = default;

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
    // Ported original logic
}

void LandscapeWorkspacePanel::RebuildLayout() {
    if (!m_ContentContainer || !m_Editor) return;
    m_ContentContainer->ClearChildren();
    
    auto tabBar = std::make_shared<we::runtime::kindui::Row>();
    tabBar->Gap(ResolveMetric(MetricToken::Space1));
    tabBar->SetFlexShrink(0.0f);
    
    auto addTab = [&](const char* label, LandscapeWorkspaceTab tab) {
        auto btn = we::runtime::kindui::MakePanelTab(label);
        btn->SetActive(m_ActiveTab == tab);
        btn->SetFlexGrow(1.0f);
        btn->SetFlexShrink(1.0f);
        btn->SetMinWidth(we::runtime::kindui::LayoutMetrics::FormChipButtonMinWidth());
        btn->SetOnClicked([this, tab]() { SetActiveTab(tab); });
        tabBar->AddChild(btn);
    };
    
    addTab("Create", LandscapeWorkspaceTab::Create);
    addTab("Sculpt", LandscapeWorkspaceTab::Sculpt);
    addTab("Paint", LandscapeWorkspaceTab::Paint);
    addTab("Manage", LandscapeWorkspaceTab::Manage);
    
    m_ContentContainer->AddChild(tabBar);
    
    auto contentArea = std::make_shared<we::runtime::kindui::ScrollLayout>();
    contentArea->SetFlexGrow(1.0f);
    contentArea->SetFlexShrink(1.0f);
    contentArea->SetMinSize({0.0f, 0.0f});

    auto tabContent = std::make_shared<we::runtime::kindui::Column>();
    tabContent->Align(we::runtime::kindui::AlignItems::Stretch);
    tabContent->Gap(we::runtime::kindui::PropertyPanelChrome::FormStackGap());
    tabContent->SetMinSize({0.0f, 0.0f});

    switch (m_ActiveTab) {
    case LandscapeWorkspaceTab::Create: BuildCreateTab(tabContent, *m_Editor); break;
    case LandscapeWorkspaceTab::Sculpt: BuildSculptTab(tabContent, *m_Editor); break;
    case LandscapeWorkspaceTab::Paint: BuildPaintTab(tabContent, *m_Editor); break;
    case LandscapeWorkspaceTab::Manage: BuildManageTab(tabContent, *m_Editor, m_ImportPath, m_ExportPath, m_ResizeX, m_ResizeY); break;
    }

    contentArea->SetContent(tabContent);
    m_ContentContainer->AddChild(contentArea);
    
    if (m_ActiveTab == LandscapeWorkspaceTab::Create) {
        auto createBtn = we::runtime::kindui::MakePrimaryAction("Create Landscape");
        createBtn->SetOnClicked([this]() {
            m_Editor->Wizard().State() = m_Editor->Dialog();
            if (m_Editor->CreateFromDialog()) {
                m_UserSelectedTab = true;
                m_ActiveTab = LandscapeWorkspaceTab::Sculpt;
                RebuildLayout();
            }
        });
        m_ContentContainer->AddChild(createBtn);
    }
}

} // namespace we::editor::terrain
