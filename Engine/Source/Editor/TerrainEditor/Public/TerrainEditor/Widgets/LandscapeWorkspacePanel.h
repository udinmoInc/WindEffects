#pragma once

#include "TerrainEditor/Export.h"
#include "TerrainEditor/ILandscapeEditor.h"

#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/ScrollViewport.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::editor::terrain {

/// Landscape Tools-drawer workspace: Create / Sculpt / Paint / Manage.
/// Binds exclusively to ILandscapeEditor (injected). No Terrain Runtime calls.
class TERRAINEDITOR_API LandscapeWorkspacePanel : public we::runtime::kindui::Widget {
public:
    explicit LandscapeWorkspacePanel(ILandscapeEditor* editor);
    ~LandscapeWorkspacePanel() override;

    using ExtensionFactory = std::function<std::shared_ptr<Widget>(LandscapeWorkspaceTab tab)>;
    void RegisterExtension(LandscapeWorkspaceTab tab, ExtensionFactory factory);

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;
    void Tick(float deltaTime) override;

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseUp(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseWheel(const we::runtime::kindui::MouseEvent& event) override;
    void OnKeyDown(const we::runtime::kindui::KeyEvent& event) override;
    [[nodiscard]] bool IsFocusable() const override { return true; }
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;

private:
    void RebuildLayout();
    void SyncDefaultTab();
    void SetActiveTab(LandscapeWorkspaceTab tab);
    void ActivateSculptTool(runtime_terrain::TerrainBrushOp op);
    [[nodiscard]] int HitTest(const we::runtime::kindui::Point& p) const;

    ILandscapeEditor* m_Editor = nullptr;
    LandscapeWorkspaceTab m_ActiveTab = LandscapeWorkspaceTab::Create;
    bool m_UserSelectedTab = false;

    we::runtime::kindui::ScrollViewport m_Scroll;
    we::runtime::kindui::Rect m_TabBarRect{};
    we::runtime::kindui::Rect m_ContentRect{};
    we::runtime::kindui::Rect m_FooterRect{};
    we::runtime::kindui::Rect m_PrimaryButtonRect{};

    std::vector<we::runtime::kindui::Rect> m_TabRects;
    std::vector<float> m_TabHover;

    struct HitTarget {
        std::uint8_t kind = 0;
        we::runtime::kindui::Rect geometry{};
        int id = 0;
        std::function<void()> onClick;
        std::function<void(std::string_view)> onTextCommit;
        bool selected = false;
        bool toggled = false;
        bool isDanger = false;
        bool isPrimary = false;
        bool editable = false;
        std::string value;
        std::string label;
        const char* iconHook = nullptr;
        float hoverAnim = 0.f;
        float pressAnim = 0.f;
    };
    std::vector<HitTarget> m_Hits;
    int m_HoveredHit = -1;
    int m_PressedHit = -1;
    int m_FocusedField = -1;
    std::string m_EditBuffer;

    std::string m_ImportPath;
    std::string m_ExportPath = "Landscape.r16";
    int m_ResizeX = 505;
    int m_ResizeY = 505;

    float m_PrimaryHover = 0.f;
    float m_PrimaryPress = 0.f;
    bool m_PrimaryHovered = false;
    bool m_PrimaryPressed = false;

    float m_ContentHeight = 0.f;
    bool m_NeedsLayout = true;

    std::unordered_map<int, ExtensionFactory> m_Extensions;
};

} // namespace we::editor::terrain
