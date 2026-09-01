#pragma once

#include "TerrainEditor/Export.h"
#include "TerrainEditor/ILandscapeEditor.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"

#include <cstdint>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::editor::panels {
class PanelBodyLayout;
}

namespace we::editor::terrain {

class TERRAINEDITOR_API LandscapeWorkspacePanel : public we::runtime::kindui::Widget {
public:
    explicit LandscapeWorkspacePanel(ILandscapeEditor* editor);
    ~LandscapeWorkspacePanel() override;

    using ExtensionFactory = std::function<std::shared_ptr<we::runtime::kindui::Widget>(LandscapeWorkspaceTab tab)>;
    void RegisterExtension(LandscapeWorkspaceTab tab, ExtensionFactory factory);

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    bool IsFocusable() const override { return true; }

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseWheel(const we::runtime::kindui::MouseEvent& event) override;
    [[nodiscard]] bool CanReceiveMouseWheelAt(const we::runtime::kindui::Point& pos) const override;
    [[nodiscard]] std::shared_ptr<we::runtime::kindui::Widget> HitTestPoint(
        const we::runtime::kindui::Point& pos,
        const we::runtime::kindui::Rect* clip = nullptr) override;

private:
    void RebuildLayout();
    void SyncDefaultTab();
    void SetActiveTab(LandscapeWorkspaceTab tab);
    void ActivateSculptTool(runtime_terrain::TerrainBrushOp op);

    ILandscapeEditor* m_Editor = nullptr;
    LandscapeWorkspaceTab m_ActiveTab = LandscapeWorkspaceTab::Create;
    bool m_UserSelectedTab = false;

    std::shared_ptr<we::editor::panels::PanelBodyLayout> m_BodyLayout;
    std::shared_ptr<we::runtime::kindui::Row> m_TabBar;
    std::shared_ptr<we::runtime::kindui::ScrollLayout> m_ScrollArea;
    std::shared_ptr<we::runtime::kindui::Column> m_TabContent;
    std::shared_ptr<we::runtime::kindui::DesignButton> m_FooterButton;

    std::string m_ImportPath;
    std::string m_ExportPath = "Landscape.r16";
    int m_ResizeX = 505;
    int m_ResizeY = 505;

    std::unordered_map<int, ExtensionFactory> m_Extensions;
};

} // namespace we::editor::terrain
