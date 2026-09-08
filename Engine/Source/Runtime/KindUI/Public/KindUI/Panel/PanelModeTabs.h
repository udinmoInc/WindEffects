#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Panel/PanelChrome.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Input/InputEvents.h"

#include <functional>
#include <string>
#include <vector>

namespace we::runtime::kindui::panels {

struct PanelModeTabDescriptor {
    std::string id;
    std::string label;
    we::runtime::kindui::WindIconRef icon = we::runtime::kindui::kWindIconNone;
};

/// Horizontal mode tab strip for drawer panels (Actors, Landscape, Tools, etc.).
class KINDUI_API PanelModeTabs : public we::runtime::kindui::Widget {
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
    void OnHoverLost() override;
    bool ShowsPointerCursor(const we::runtime::kindui::Point& position) const override;

private:
    [[nodiscard]] size_t ActiveTabIndex() const;
    void RebuildTabGeometries();

    std::vector<PanelModeTabDescriptor> m_Tabs;
    std::vector<PanelChrome::DockTabDescriptor> m_Descriptors;
    PanelChrome::DockTabStripLayout m_StripLayout;
    std::string m_ActiveTabId;
    int m_HoveredIndex = -1;
    std::function<void(const std::string& tabId)> m_OnTabChanged;
};

} // namespace we::runtime::kindui::panels
