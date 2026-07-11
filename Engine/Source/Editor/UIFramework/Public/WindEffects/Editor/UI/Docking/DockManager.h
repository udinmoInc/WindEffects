#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"

#include <mutex>
#include <unordered_map>

namespace WindEffects::Editor::UI {

class DockManager final : public IDockManager {
public:
    void RegisterPanel(const DockPanelDescriptor& descriptor, PanelFactory factory) override;
    void UnregisterPanel(std::string_view panelId) override;

    [[nodiscard]] const WorkspaceLayout& GetLayout() const override { return m_Layout; }
    void SetLayout(WorkspaceLayout layout) override;

    [[nodiscard]] std::shared_ptr<void> GetPanelWidget(std::string_view panelId) const override;
    void SetPanelVisible(std::string_view panelId, bool visible) override;
    [[nodiscard]] bool IsPanelVisible(std::string_view panelId) const override;

    void FloatPanel(std::string_view panelId) override;
    void DockPanel(std::string_view panelId, DockZone zone) override;

private:
    struct PanelEntry {
        DockPanelDescriptor descriptor;
        PanelFactory factory;
        mutable std::shared_ptr<void> instance;
        bool visible = true;
        DockZone currentZone = DockZone::Floating;
    };

    mutable std::mutex m_Mutex;
    WorkspaceLayout m_Layout;
    std::unordered_map<std::string, PanelEntry> m_Panels;
};

class UIFRAMEWORK_API DockLayoutSerializer final : public IDockLayoutSerializer {
public:
    [[nodiscard]] bool Save(const WorkspaceLayout& layout, std::string_view path) const override;
    [[nodiscard]] bool Load(WorkspaceLayout& layout, std::string_view path) const override;
};

UIFRAMEWORK_API WorkspaceLayout CreateDefaultEditorWorkspaceLayout();

} // namespace WindEffects::Editor::UI
