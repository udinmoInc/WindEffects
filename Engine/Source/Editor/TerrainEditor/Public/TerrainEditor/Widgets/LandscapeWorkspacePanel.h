#pragma once

#include "TerrainEditor/Export.h"
#include "TerrainEditor/ILandscapeEditor.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/ScrollViewport.h"
#include "KindUI/Layout/Flex.h"

#include <cstdint>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::editor::terrain {

class TERRAINEDITOR_API LandscapeWorkspacePanel : public we::runtime::kindui::Widget {
public:
    explicit LandscapeWorkspacePanel(ILandscapeEditor* editor);
    ~LandscapeWorkspacePanel() override;

    using ExtensionFactory = std::function<std::shared_ptr<we::runtime::kindui::Widget>(LandscapeWorkspaceTab tab)>;
    void RegisterExtension(LandscapeWorkspaceTab tab, ExtensionFactory factory);

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override {
        if (!m_ContentContainer) {
            return availableSize;
        }
        we::runtime::kindui::Size desired = m_ContentContainer->Measure(availableSize);
        if (availableSize.width > 0.0f) {
            desired.width = (std::min)(desired.width, availableSize.width);
        }
        if (availableSize.height > 0.0f) {
            desired.height = (std::min)(desired.height, availableSize.height);
        }
        return desired;
    }
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override { m_Geometry = allottedRect; if (m_ContentContainer) m_ContentContainer->Arrange(allottedRect); }
    void Paint(we::runtime::kindui::PaintContext& context) override { if (m_ContentContainer) m_ContentContainer->Paint(context); }
    bool IsFocusable() const override { return true; }

private:
    void RebuildLayout();
    void SyncDefaultTab();
    void SetActiveTab(LandscapeWorkspaceTab tab);
    void ActivateSculptTool(runtime_terrain::TerrainBrushOp op);

    ILandscapeEditor* m_Editor = nullptr;
    LandscapeWorkspaceTab m_ActiveTab = LandscapeWorkspaceTab::Create;
    bool m_UserSelectedTab = false;

    std::shared_ptr<we::runtime::kindui::Column> m_ContentContainer;

    std::string m_ImportPath;
    std::string m_ExportPath = "Landscape.r16";
    int m_ResizeX = 505;
    int m_ResizeY = 505;

    std::unordered_map<int, ExtensionFactory> m_Extensions;
};

} // namespace we::editor::terrain
