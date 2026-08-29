#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Input/InputEvents.h"

#include <functional>
#include <string>
#include <vector>

namespace we::editor::panels {

struct PanelModeTabDescriptor {
    std::string id;
    std::string label;
    std::string iconName;
};

/// Horizontal mode tab strip for editor tool drawers (Actors, Landscape, etc.).
class UIFRAMEWORK_API PanelModeTabs : public we::runtime::kindui::Widget {
public:
    void SetTabs(std::vector<PanelModeTabDescriptor> tabs);
    void SetActiveTabId(const std::string& tabId);
    [[nodiscard]] const std::string& GetActiveTabId() const { return m_ActiveTabId; }

    void SetOnTabChanged(std::function<void(const std::string& tabId)> callback) {
        m_OnTabChanged = std::move(callback);
    }

    we::runtime::kindui::Size Measure(const we::runtime::kindui::Size& availableSize) override;
    void Arrange(const we::runtime::kindui::Rect& allottedRect) override;
    void Paint(we::runtime::kindui::PaintContext& context) override;

    void OnMouseDown(const we::runtime::kindui::MouseEvent& event) override;
    void OnMouseMove(const we::runtime::kindui::MouseEvent& event) override;

private:
    struct TabLayout {
        PanelModeTabDescriptor descriptor;
        we::runtime::kindui::Rect geometry;
    };

    void RebuildTabGeometries();

    std::vector<PanelModeTabDescriptor> m_Tabs;
    std::vector<TabLayout> m_Layout;
    std::string m_ActiveTabId;
    int m_HoveredIndex = -1;
    std::function<void(const std::string& tabId)> m_OnTabChanged;
};

} // namespace we::editor::panels
